//--------------------------------------------------------------------------------------
// DynamicLOD.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DynamicLOD.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "Meshlet.h"

#pragma warning( disable : 4324 4365 )

extern void ExitSample() noexcept;

using namespace ATG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace 
{
    //--------------------------------------
    // Definitions

    const wchar_t* s_sampleTitle = L"Dynamic LOD";

    const wchar_t* s_ampShaderFilename   = L"DynamicLodAS.cso";
    const wchar_t* s_meshShaderFilename  = L"InstancedLodMS.cso";
    const wchar_t* s_pixelShaderFilename = L"BasicMeshletPS.cso";

    const wchar_t* s_lodFilenames[] =
    {
#ifdef _GAMING_DESKTOP
        L"ATGDragon\\Dragon_LOD0.sdkmesh",
        L"ATGDragon\\Dragon_LOD1.sdkmesh",
        L"ATGDragon\\Dragon_LOD2.sdkmesh",
        L"ATGDragon\\Dragon_LOD3.sdkmesh",
        L"ATGDragon\\Dragon_LOD4.sdkmesh",
        L"ATGDragon\\Dragon_LOD5.sdkmesh",
#else
        L"Dragon_LOD0.sdkmesh",
        L"Dragon_LOD1.sdkmesh",
        L"Dragon_LOD2.sdkmesh",
        L"Dragon_LOD3.sdkmesh",
        L"Dragon_LOD4.sdkmesh",
        L"Dragon_LOD5.sdkmesh",
#endif
    };

#ifdef _GAMING_XBOX_SCARLETT
    constexpr uint32_t c_textureDataPitchAlign = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
#else
    constexpr uint32_t c_textureDataPitchAlign = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
#endif

    const HelpButtonAssignment c_buttonAssignment[] =
    {
        { HelpID::A_BUTTON, L"Change Instance Mode" },
        { HelpID::B_BUTTON, L"Force LOD 0" },
        { HelpID::Y_BUTTON, L"Toggle Culling" },
        { HelpID::X_BUTTON, L"Cycle Render Mode" },
        { HelpID::LEFT_STICK, L"Camera Zoom" },
        { HelpID::RIGHT_STICK, L"Camera Rotate" },
        { HelpID::LEFT_SHOULDER, L"Decrease Instance Level" },
        { HelpID::RIGHT_SHOULDER, L"Increase Instance Level" },
        { HelpID::MENU_BUTTON, L"Show Help Screen" },
        { HelpID::VIEW_BUTTON, L"Exit Sample" },
    };

    constexpr float c_fovy = XM_PIDIV4;
    constexpr uint32_t c_maxGroupsPerDispatch = 65536;
    constexpr uint32_t c_maxInstances = AS_GROUP_SIZE * c_maxGroupsPerDispatch;

    // Resource view offsets into the descriptor heap
    // The mesh & meshlet SRVs are laid out as they are referenced in shader code
    enum DescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_WhiteTexture,
        SRV_MeshInfoLODs,
        SRV_VertexLODs = SRV_MeshInfoLODs + MAX_LOD_LEVELS,
        SRV_MeshletLODs = SRV_VertexLODs + MAX_LOD_LEVELS,
        SRV_UniqueVertexIndexLODs = SRV_MeshletLODs + MAX_LOD_LEVELS,
        SRV_PrimitiveIndexLODs = SRV_UniqueVertexIndexLODs + MAX_LOD_LEVELS,
        SRV_Count = SRV_PrimitiveIndexLODs + MAX_LOD_LEVELS,
    };

    template <typename T>
    T GetAlignedSize(T size)
    {
        const T alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        return (size + alignment - 1) & ~(alignment - 1);
    }

    template <typename T, typename U>
    constexpr T DivRoundUp(T num, U denom)
    {
        return (num + denom - 1) / denom;
    }

    template<class T>
    std::wstring FormatWithCommas(T value)
    {
        static std::locale l("");

        std::wstringstream ss;
        ss.imbue(l);
        ss << std::fixed << value;
        return ss.str();
    }
}

Sample::Sample() noexcept(false) 
    : m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
    , m_uploadInstances(false)
    , m_instMode(InstanceMode::IM_Line)
    , m_renderMode(RenderMode::RM_Meshlets)
    , m_instanceLevel(0)
    , m_forceVisible(false)
    , m_forceLod0(false)
    , m_renderHelp(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_AmplificationShaders);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    // Input-agnostic app controls
    struct AppControls
    {
        bool CycleViz;
        bool CycleInst;
        int  InstLevelChange;
        bool ForceLod0;
        bool ForceVisible;
        bool ToggleHelp;
        bool Exit;
    } controls;

    // Populate our app controls using per-input logic
    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

        m_gamePadButtons.Update(pad);
        m_camera.Update(elapsedTime, pad);

        // Grab Controls
        controls.Exit = m_gamePadButtons.view == ButtonState::PRESSED;
        controls.ToggleHelp = m_gamePadButtons.menu == ButtonState::PRESSED;
        controls.CycleViz = m_gamePadButtons.x == ButtonState::PRESSED;
        controls.CycleInst = m_gamePadButtons.a == ButtonState::PRESSED;
        controls.ForceLod0 = m_gamePadButtons.b == ButtonState::PRESSED;
        controls.ForceVisible = m_gamePadButtons.y == ButtonState::PRESSED;
        controls.InstLevelChange = m_gamePadButtons.rightShoulder == ButtonState::PRESSED ? 1 : (m_gamePadButtons.leftShoulder == ButtonState::PRESSED ? -1 : 0);
    }
    else
    {
        m_gamePadButtons.Reset();

        m_keyboardButtons.Update(m_keyboard->GetState());
        m_camera.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());

        // Grab Controls
        controls.Exit = m_keyboardButtons.IsKeyPressed(Keyboard::Escape);
        controls.ToggleHelp = m_keyboardButtons.IsKeyPressed(Keyboard::T);
        controls.CycleViz = m_keyboardButtons.IsKeyPressed(Keyboard::Space);
        controls.CycleInst = m_keyboardButtons.IsKeyPressed(Keyboard::Tab);
        controls.ForceLod0 = m_keyboardButtons.IsKeyPressed(Keyboard::LeftShift);
        controls.ForceVisible = m_keyboardButtons.IsKeyPressed(Keyboard::LeftControl);
        controls.InstLevelChange = m_keyboardButtons.IsKeyPressed(Keyboard::OemPlus) ? 1 : (m_keyboardButtons.IsKeyPressed(Keyboard::OemMinus) ? -1 : 0);
    }

    // Apply controls to the app settings
    if (controls.InstLevelChange != 0)
    {
        m_instanceLevel = static_cast<uint32_t>(std::max(int(m_instanceLevel) + controls.InstLevelChange, 0));
        RegenerateInstances();
    }

    if (controls.CycleInst)
    {
        m_instMode = InstanceMode((m_instMode + 1) % IM_Count);
        RegenerateInstances();
    }

    if (controls.CycleViz)
    {
        m_renderMode = RenderMode((m_renderMode + 1) % RM_Count);
    }

    if (controls.ForceLod0)
    {
        m_forceLod0 = !m_forceLod0;
        m_gpuTimer->Reset();
    }

    if (controls.ForceVisible)
    {
        m_forceVisible = !m_forceVisible;
        m_gpuTimer->Reset();
    }

    if (controls.ToggleHelp)
    {
        m_renderHelp = !m_renderHelp;
    }

    if (controls.Exit)
    {
        ExitSample();
    }

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");
    m_gpuTimer->BeginFrame(commandList);

    // Render the help menu if specified
    if (m_renderHelp)
    {
        m_controlHelp->Render(commandList);
    }
    else
    {
        // Begin object rendering
        UpdateConstants(commandList);

        ID3D12DescriptorHeap* heap[] = { m_srvPile->Heap() };
        commandList->SetDescriptorHeaps(1, heap);

        // Set up pipeline state
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_msPso.Get());

        uint32_t instanceCount = static_cast<uint32_t>(m_instances.size());
        DrawParams params = { 0, instanceCount };

        commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
        commandList->SetGraphicsRoot32BitConstants(1, 2, &params, 0);
        commandList->SetGraphicsRootShaderResourceView(2, m_instanceBuffer->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(3, m_srvPile->GetGpuHandle(SRV_MeshInfoLODs));
        commandList->SetGraphicsRootUnorderedAccessView(4, m_lodCountsBuffer->GetGPUVirtualAddress());

        // Calculate our final threadgroup dispatch count
        const uint32_t groupCount = DivRoundUp(instanceCount, AS_GROUP_SIZE);

        m_gpuTimer->Start(commandList);
        commandList->DispatchMesh(groupCount, 1, 1);
        m_gpuTimer->Stop(commandList);

        DrawHUD(commandList);
    }

    PIXEndEvent(commandList);

    TransitionResource(commandList, m_lodCountsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->CopyResource(m_lodCountsReadback.Get(), m_lodCountsBuffer.Get());
    TransitionResource(commandList, m_lodCountsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_gpuTimer->EndFrame(commandList);
    m_deviceResources->Present();

    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::UpdateConstants(ID3D12GraphicsCommandList* commandList)
{
    // View constants
    {
        XMMATRIX view = m_camera.GetView();
        XMMATRIX viewProj = view * m_camera.GetProjection();

        auto cbMem = m_graphicsMemory->AllocateConstant<Constants>();
        Constants& constants = *static_cast<Constants*>(cbMem.Memory());

        XMStoreFloat3(&constants.ViewPosition, m_camera.GetPosition());
        XMStoreFloat4x4(&constants.View, XMMatrixTranspose(view));
        XMStoreFloat4x4(&constants.ViewProj, XMMatrixTranspose(viewProj));
        constants.RenderMode = m_renderMode;
        constants.RecipTanHalfFovy = 1.0f / std::tanf(c_fovy * 0.5f);
        constants.LODCount = static_cast<uint32_t>(m_lods.size());
        constants.ForceLOD0 = m_forceLod0;
        constants.ForceVisible = m_forceVisible;

        // Update camera frustum planes
        // http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

        XMMATRIX vp = XMMatrixTranspose(viewProj);

        XMVECTOR planes[6] =
        {
            XMPlaneNormalize(XMVectorAdd(vp.r[3], vp.r[0])),      // Left
            XMPlaneNormalize(XMVectorSubtract(vp.r[3], vp.r[0])), // Right
            XMPlaneNormalize(XMVectorAdd(vp.r[3], vp.r[1])),      // Bottom
            XMPlaneNormalize(XMVectorSubtract(vp.r[3], vp.r[1])),      // Top
            XMPlaneNormalize(vp.r[2]),                            // Near
            XMPlaneNormalize(XMVectorSubtract(vp.r[3], vp.r[2])), // Far
        };

        for (uint32_t i = 0; i < 6; ++i)
        {
            XMStoreFloat4(&constants.Planes[i], planes[i]);
        }

        TransitionResource(commandList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->CopyBufferRegion(m_constantBuffer.Get(), 0, cbMem.Resource(), cbMem.ResourceOffset(), cbMem.Size());
        TransitionResource(commandList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    if (m_uploadInstances)
    {
        auto instanceMem = m_graphicsMemory->Allocate(m_instances.size() * sizeof(Instance));
        std::memcpy(instanceMem.Memory(), m_instances.data(), m_instances.size() * sizeof(Instance));

        TransitionResource(commandList, m_instanceBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->CopyBufferRegion(m_instanceBuffer.Get(), 0, instanceMem.Resource(), instanceMem.ResourceOffset(), instanceMem.Size());
        TransitionResource(commandList, m_instanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

        m_uploadInstances = false;
    }
}

void Sample::DrawHUD(ID3D12GraphicsCommandList* commandList)
{
    // Do some quick instance & primitive count calculations on memory read back from GPU.
    uint32_t lodCounts[MAX_LOD_LEVELS] = {};

    const uint32_t instanceCount = static_cast<uint32_t>(m_instances.size());
    const uint32_t asThreadGroups = DivRoundUp(instanceCount, AS_GROUP_SIZE);

    const uint64_t maxPrimCount = static_cast<uint64_t>(instanceCount) * m_primCounts[0];

    for (uint32_t i = 0; i < asThreadGroups; ++i)
    {
        for (uint32_t j = 0; j < MAX_LOD_LEVELS; ++j)
        {
            lodCounts[j] += m_lodCounts[i * MAX_LOD_LEVELS + j];
        }
    }

    uint32_t actualInstCount = 0;
    uint64_t actualPrimCount = 0;

    for (uint32_t i = 0; i < MAX_LOD_LEVELS; ++i)
    {
        actualInstCount += lodCounts[i];
        actualPrimCount += static_cast<uint64_t>(lodCounts[i]) * m_primCounts[i];
    }

    // Start rendering HUD
    m_hudBatch->Begin(commandList);

    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(m_displayWidth, m_displayHeight);

    // Draw our text underlay boxes
    m_hudBatch->Draw(m_srvPile->GetGpuHandle(SRV_WhiteTexture), { 1, 1 }, RECT{ safe.left - 10, safe.top - 10, safe.left + 310, safe.top + 325 });

    wchar_t textBuffer[128] = {};
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = DirectX::Colors::DarkKhaki;

    // Sample title
    m_smallFont->DrawString(m_hudBatch.get(), s_sampleTitle, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    // Sample modes
    const wchar_t* s_instModeNames[] = { L"Circles", L"Cube", L"Line" };
    const wchar_t* s_visModeNames[] = { L"Flat", L"Meshlets", L"LOD Level" };

    swprintf_s(textBuffer, L"Instance Mode: %s", s_instModeNames[m_instMode]);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Render Mode: %s", s_visModeNames[m_renderMode]);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Instance Level: %d", m_instanceLevel + 1);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Time: %f", m_gpuTimer->GetAverageMS());
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();
    textPos.y += m_smallFont->GetLineSpacing();

    // Instancing statistics
    auto instanceCountStr = FormatWithCommas(instanceCount);
    auto actualInstStr = FormatWithCommas(actualInstCount);

    swprintf_s(textBuffer, L"Instances -\n  Max:    %s\n  Actual: %s", instanceCountStr.c_str(), actualInstStr.c_str());
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing() * 3;

    // Primitive statistics
    auto maxPrimsStr = FormatWithCommas(maxPrimCount);
    auto actPrimsStr = FormatWithCommas(actualPrimCount);

    swprintf_s(textBuffer, L"Primitives -\n  Max:    %s\n  Actual: %s", maxPrimsStr.c_str(), actPrimsStr.c_str());
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing() * 3;

    // Culling & LOD0 flags
    if (m_forceVisible || m_forceLod0)
    {
        int offset = swprintf_s(textBuffer, L"Forcing - %s", m_forceVisible ? L"Visible " : L"");
        swprintf(textBuffer + offset, _countof(textBuffer), L"%s", m_forceLod0 ? L"LOD0" : L"");

        m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();
    }

    // Controls
    textPos = XMFLOAT2(float(safe.left), float(safe.bottom));
    textPos.y -= m_smallFont->GetLineSpacing();

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[Menu] Show Help", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();
    }
    else
    {
        m_hudBatch->Draw(m_srvPile->GetGpuHandle(SRV_WhiteTexture), { 1, 1 }, RECT{ safe.left - 10, safe.bottom - 180, safe.left + 465, safe.bottom + 10 });

        m_smallFont->DrawString(m_hudBatch.get(), L"LControl - Toggle Culling", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"LShift -   Force LOD 0", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"Tab -      Change Instancing Mode", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"+- -       Change Instancing Level", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"Space -    Cycle Render Mode", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"LMouse -   Rotate Camera (Hold)", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"ASWD -     Move Camera", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();
    }

    m_hudBatch->End();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Check for Shader Model 6.5 and Mesh Shader feature support
#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_5))
    {
        OutputDebugStringA("ERROR: Shader Model 6.5 is not supported\n");
        throw std::exception("Shader Model 6.5 is not supported");
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features)))
        || (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
    {
        OutputDebugStringA("ERROR: Mesh Shaders aren't supported!\n");
        throw std::exception("Mesh Shaders aren't supported!");
    }
#endif

    // Instantiate our manager objects
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
    m_srvPile = std::make_unique<DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        128,
        DescriptorHeapIndex::SRV_Count);

    m_controlHelp = std::make_unique<Help>(L"Dynamic LOD", nullptr, c_buttonAssignment, _countof(c_buttonAssignment));

    // Create the Mesh Shader Pipeline State Object
    {
        auto ampShader   = DX::ReadData(s_ampShaderFilename);
        auto meshShader  = DX::ReadData(s_meshShaderFilename);
        auto pixelShader = DX::ReadData(s_pixelShaderFilename);

        // Pull the root signature directly from the mesh shader bytecode
        DX::ThrowIfFailed(device->CreateRootSignature(0, ampShader.data(), ampShader.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        // Populate the Mesh Shader PSO descriptor
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature        = m_rootSignature.Get();
        psoDesc.AS                    = { ampShader.data(), ampShader.size() };
        psoDesc.MS                    = { meshShader.data(), meshShader.size() };
        psoDesc.PS                    = { pixelShader.data(), pixelShader.size() };
        psoDesc.NumRenderTargets      = 1;
        psoDesc.RTVFormats[0]         = m_deviceResources->GetBackBufferFormat();
        psoDesc.DSVFormat             = m_deviceResources->GetDepthBufferFormat();
        psoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
        psoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
        psoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
        psoDesc.SampleMask            = UINT_MAX;
        psoDesc.SampleDesc            = DefaultSampleDesc();

        auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

        // Point to our populated stream desc
        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
        streamDesc.SizeInBytes                   = sizeof(meshStreamDesc);
        streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

        // Create the PSO using the stream desc CreatePipelineState API
        DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_msPso.ReleaseAndGetAddressOf())));
    }

    // Create GPU resources for various purposes
    {
        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto constantsDesc = CD3DX12_RESOURCE_DESC::Buffer(GetAlignedSize(sizeof(Constants)));

        // Create constant buffer
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &constantsDesc,
            c_initialReadTargetState,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_constantBuffer.ReleaseAndGetAddressOf())));

        // Create single pixel texture for text underlay box
        auto pixelDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &pixelDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_whiteTexture.ReleaseAndGetAddressOf())));

        device->CreateShaderResourceView(m_whiteTexture.Get(), nullptr, m_srvPile->GetCpuHandle(SRV_WhiteTexture));
    }

    // Load the model LODs and mesh data
    m_lods.resize(_countof(s_lodFilenames));

    for (size_t i = 0; i < m_lods.size(); ++i)
    {
        wchar_t filepath[256];
        DX::FindMediaFile(filepath, _countof(filepath), s_lodFilenames[i]);

        // Load the model's mesh data from the .sdkmesh file.
        m_lods[i].Model = Model::CreateFromSDKMESH(device, filepath);

        // Load the model's meshlet data from the companion .bin file for the .sdkmesh file.
        wchar_t meshletFilepath[256] = {};
        swprintf_s(meshletFilepath, L"%s.bin", filepath);

        m_lods[i].MeshletData = MeshletSet::Read(meshletFilepath);
    }

    // Upload graphics resources to the GPU
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        // Create the D3D12 resources for the model and meshlet data for each LOD
        for (auto& lod : m_lods)
        {
            lod.Model->LoadStaticBuffers(device, resourceUpload, true);

            for (auto& m : lod.MeshletData)
            {
                m.CreateResources(device, resourceUpload);
            }
        }

        // Create our HUD objects
        const RenderTargetState backBufferRts(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        m_controlHelp->RestoreDevice(device, resourceUpload, backBufferRts);

        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

        // Load and upload our UI fonts
        wchar_t strFilePath[MAX_PATH] = {};
        DX::FindMediaFile(strFilePath, MAX_PATH, L"Courier_16.spritefont");
        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload, strFilePath,
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_Font),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_Font));

        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload, strFilePath,
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_CtrlFont),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_CtrlFont));

        // Upload a single grey pixel to the underlay resource
        uint8_t color[4] = { 5, 5, 5, 196 };

        D3D12_SUBRESOURCE_DATA data = {};
        data.pData = color;
        data.RowPitch = c_textureDataPitchAlign;
        data.SlicePitch = 0;

        resourceUpload.Upload(m_whiteTexture.Get(), 0, &data, 1);
        resourceUpload.Transition(m_whiteTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        resourceUpload.End(m_deviceResources->GetCommandQueue());
    }


    // Populate descriptor table with arrays of SRVs for each LOD
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_lods.size()); ++i)
    {
        auto& m = m_lods[i].MeshletData[0];
        auto& vb = m_lods[i].Model->meshes[0]->opaqueMeshParts[0];

        m_primCounts[i] = m.GetPrimitiveCount();

        // Mesh Info Buffers
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m.GetMeshInfoResource()->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (UINT)GetAlignedSize(sizeof(MeshInfo));

        device->CreateConstantBufferView(&cbvDesc, m_srvPile->GetCpuHandle(SRV_MeshInfoLODs + i));

        // Populate common shader resource view desc with shared settings.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        // Vertices
        srvDesc.Buffer.StructureByteStride = vb->vertexStride;
        srvDesc.Buffer.NumElements = vb->vertexCount;
        device->CreateShaderResourceView(m_lods[i].Model->meshes[0]->opaqueMeshParts[0]->vertexBuffer.Resource(), &srvDesc, m_srvPile->GetCpuHandle(SRV_VertexLODs + i));

        // Meshlets
        srvDesc.Buffer.StructureByteStride = sizeof(Meshlet);
        srvDesc.Buffer.NumElements = m.GetMeshletCount();
        device->CreateShaderResourceView(m.GetMeshletResource(), &srvDesc, m_srvPile->GetCpuHandle(SRV_MeshletLODs + i));

        // Primitive Indices
        srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
        srvDesc.Buffer.NumElements = m.GetPrimitiveCount();
        device->CreateShaderResourceView(m.GetPrimitiveResource(), &srvDesc, m_srvPile->GetCpuHandle(SRV_PrimitiveIndexLODs + i));

        // Unique Vertex Indices
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Buffer.StructureByteStride = 0;
        srvDesc.Buffer.NumElements = DivRoundUp(m.GetIndexCount() * m.BytesPerIndex(), 4);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

        device->CreateShaderResourceView(m.GetUniqueIndexResource(), &srvDesc, m_srvPile->GetCpuHandle(SRV_UniqueVertexIndexLODs + i));
    }

    // Null-out remaining LOD slots in the descriptor table.
    for (uint32_t i = static_cast<uint32_t>(m_lods.size()); i < MAX_LOD_LEVELS; ++i)
    {
        device->CreateConstantBufferView(nullptr, m_srvPile->GetCpuHandle(SRV_MeshInfoLODs + i));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        srvDesc.Buffer.StructureByteStride = 24;
        device->CreateShaderResourceView(nullptr, &srvDesc, m_srvPile->GetCpuHandle(SRV_VertexLODs + i));

        srvDesc.Buffer.StructureByteStride = sizeof(Meshlet);
        device->CreateShaderResourceView(nullptr, &srvDesc, m_srvPile->GetCpuHandle(SRV_MeshletLODs + i));

        srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
        device->CreateShaderResourceView(nullptr, &srvDesc, m_srvPile->GetCpuHandle(SRV_PrimitiveIndexLODs + i));

        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Buffer.StructureByteStride = 0;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        device->CreateShaderResourceView(nullptr, &srvDesc, m_srvPile->GetCpuHandle(SRV_UniqueVertexIndexLODs + i));
    }

    RegenerateInstances();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT size = m_deviceResources->GetOutputSize();
    m_displayWidth = size.right - size.left;
    m_displayHeight = size.bottom - size.top;

    m_controlHelp->SetWindow(size);

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    m_camera.SetWindow(m_displayWidth, m_displayHeight);
    m_camera.SetProjectionParameters(c_fovy, 1.0f, 10000.0f, true);

    m_camera.SetLookAt(Vector3(-200.0f, 200.0f, -200.0f), Vector3::Up * 40.0f);
    m_camera.SetSensitivity(500.0f, 100.0f, 1000.0f, 10.0f);
}

void Sample::OnDeviceLost()
{
    m_gpuTimer.reset();
    m_graphicsMemory.reset();
    m_controlHelp.reset();

    m_rootSignature.Reset();
    m_msPso.Reset();
    m_constantBuffer.Reset();
    m_instanceBuffer.Reset();
    m_whiteTexture.Reset();

    m_lodCountsBuffer.Reset();
    m_lodCountsReadback.Reset();
    m_lodCounts = nullptr;

    m_hudBatch.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();
    m_camera.Reset();

    m_lods.clear();
    m_srvPile.reset();
    m_instances.clear();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion

void Sample::RegenerateInstances()
{
    m_uploadInstances = true;

    const float radius = m_lods[0].Model->meshes[0]->boundingSphere.Radius;
    const float padding = 0.5f;
    const float spacing = (1.0f + padding) * radius;

    // Determine our instancing mode
    if (m_instMode == InstanceMode::IM_Line)
    {
        // Create the instances in a growing straight line
        m_instances.resize(m_instanceLevel + 1);

        float dist = (m_instanceLevel / 2) * -spacing;
        for (uint32_t i = 0; i < m_instances.size(); ++i)
        {
            Matrix world = Matrix::CreateTranslation(dist, 0.0f, 0.0f);
            Vector3 location = world.Translation();

            Instance& inst = m_instances[i];
            inst.World = world.Transpose();
            inst.WorldInvTranspose = world.Transpose().Invert().Transpose();
            inst.BoundingSphere = Vector4(location.x, location.y, location.z, radius);

            dist += spacing;
        }
    }
    else if (m_instMode == InstanceMode::IM_Circles)
    {
        // Create the instances in growing concentric circles
        m_instances.clear();

        float dist = 0.0f;
        for (uint32_t i = 0; i < m_instanceLevel + 1; ++i)
        {
            float totalCircum = dist * XM_2PI;

            uint32_t count = static_cast<uint32_t>(std::max(1.0f, totalCircum / spacing));
            float anglePerDiv = XM_2PI / count;

            for (uint32_t j = 0; j < count; ++j)
            {
                float angle = j * anglePerDiv;

                Matrix world = Matrix::CreateTranslation(dist, 0.0f, 0.0f) * Matrix::CreateRotationY(angle);
                Vector3 location = world.Translation();

                Instance inst;
                inst.World = world.Transpose();
                inst.WorldInvTranspose = world.Transpose().Invert().Transpose();
                inst.BoundingSphere = Vector4(location.x, location.y, location.z, radius);

                m_instances.push_back(inst);
            }

            dist += spacing;
        }
    }
    else // InstanceMode::IM_Cube
    {
        // Create the instances in a growing cube volume
        const uint32_t width = m_instanceLevel * 2 + 1;
        const uint32_t instanceCount = width * width * width;

        m_instances.resize(instanceCount);
        float extents = spacing * m_instanceLevel;

        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            Vector3 index = Vector3(float(i % width), float((i / width) % width), float(i / (width * width)));
            Vector3 location = index * spacing - Vector3(extents);

            Matrix world = Matrix::CreateTranslation(location);

            auto& inst = m_instances[i];
            inst.World = world.Transpose();
            inst.WorldInvTranspose = world.Transpose().Invert().Transpose();
            inst.BoundingSphere = Vector4(location.x, location.y, location.z, radius);
        }
    }

    // Limit instance count to the number of instances
    if (m_instances.size() > c_maxInstances)
    {
        m_instances.resize(c_maxInstances);
    }

    // Only recreate instance-sized buffers if necessary.
    const uint32_t instanceBufferSize = (uint32_t)GetAlignedSize(static_cast<uint32_t>(m_instances.size()) * sizeof(Instance));

    if (!m_instanceBuffer || m_instanceBuffer->GetDesc().Width < instanceBufferSize)
    {
        m_deviceResources->WaitForGpu();

        auto device = m_deviceResources->GetD3DDevice();

        // Create/re-create the instance buffer
        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto instanceDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize);

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeap,
                D3D12_HEAP_FLAG_NONE,
                &instanceDesc,
                c_initialReadTargetState,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_instanceBuffer.ReleaseAndGetAddressOf()))
        );

        // Create our buffer to readback culling / LOD counts statistics.
        // Averages to one-byte per instance
        const uint32_t maxGroupCount = DivRoundUp(static_cast<uint32_t>(m_instances.size()), AS_GROUP_SIZE);

        // UAV resource for writing stats on GPU
        auto countsDesc = CD3DX12_RESOURCE_DESC::Buffer(maxGroupCount * MAX_LOD_LEVELS * sizeof(uint32_t), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &countsDesc,
            c_initialUAVTargetState,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_lodCountsBuffer.ReleaseAndGetAddressOf()))
        );

        auto readbackHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        countsDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &readbackHeap,
            D3D12_HEAP_FLAG_NONE,
            &countsDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_lodCountsReadback.ReleaseAndGetAddressOf()))
        );

        // Persistently map the readback heap for reads on CPU
        m_lodCountsReadback->Map(0, nullptr, (void**)&m_lodCounts);
    }
}
