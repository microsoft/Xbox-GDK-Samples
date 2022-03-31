//--------------------------------------------------------------------------------------
// MeshletInstancing.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MeshletInstancing.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "Meshlet.h"

#pragma warning( disable : 4324 4365 )

extern void ExitSample();

using namespace ATG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;
using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

namespace 
{
    //--------------------------------------
    // Definitions

    const wchar_t* s_sampleTitle = L"Meshlet Instancing";

    const wchar_t* s_meshShaderFilename = L"InstancedMeshletMS.cso";
    const wchar_t* s_pixelShaderFilename = L"BasicMeshletPS.cso";

    static const wchar_t* s_lodFilenames[] =
    {
#if _GAMING_DESKTOP
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
    const uint32_t c_lodCount = _countof(s_lodFilenames);

#ifdef _GAMING_XBOX_SCARLETT
    const uint32_t c_textureDataPitchAlign = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
#else
    const uint32_t c_textureDataPitchAlign = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
#endif

    const HelpButtonAssignment c_buttonAssignment[] =
    {
        { HelpID::A_BUTTON, L"Switch Instance Mode" },
        { HelpID::X_BUTTON, L"Switch Render Mode" },
        { HelpID::LEFT_STICK, L"Camera Zoom" },
        { HelpID::RIGHT_STICK, L"Camera Rotate" },
        { HelpID::DPAD_LEFT, L"Decrease LOD" },
        { HelpID::DPAD_RIGHT, L"Increase LOD" },
        { HelpID::LEFT_SHOULDER, L"Decrease Instance Level" },
        { HelpID::RIGHT_SHOULDER, L"Increase Instance Level" },
        { HelpID::MENU_BUTTON, L"Show Help Screen" },
        { HelpID::VIEW_BUTTON, L"Exit Sample" },
    };

    enum DescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_WhiteTexture,
        SRV_Count
    };

    // Helper functions
    template <typename T>
    size_t GetAlignedSize(T size)
    {
        const size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        const size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
        return alignedSize;
    }

    template <typename T>
    constexpr T DivRoundUp(T num, T denom)
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
    : m_deviceResources(std::make_unique<DX::DeviceResources>())
    , m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
    , m_instMode(InstanceMode::IM_Line)
    , m_renderMode(RM_Meshlets)
    , m_lodIndex(0)
    , m_instanceLevel(0)
    , m_updateInstances(false)
    , m_renderHelp(false)
{
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

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // Input-agnostic app controls
    struct AppControls
    {
        bool CycleViz;
        bool CycleInst;
        int  InstLevelChange;
        int  LodChange;
        bool ToggleHelp;
        bool Exit;
    } controls;

    // Populate our app controls using per-input logic
    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        // Grab Controls
        controls.Exit = m_gamePadButtons.view == ButtonState::PRESSED;
        controls.ToggleHelp = m_gamePadButtons.menu == ButtonState::PRESSED;
        controls.CycleViz = m_gamePadButtons.x == ButtonState::PRESSED;
        controls.CycleInst = m_gamePadButtons.a == ButtonState::PRESSED;
        controls.InstLevelChange = m_gamePadButtons.rightShoulder == ButtonState::PRESSED ? 1 : (m_gamePadButtons.leftShoulder == ButtonState::PRESSED ? -1 : 0);
        controls.LodChange = m_gamePadButtons.dpadLeft == ButtonState::PRESSED ? 1 : (m_gamePadButtons.dpadRight == ButtonState::PRESSED ? -1 : 0);

        // Update Camera
        m_camera.Update(elapsedTime, pad);
    }
    else
    {
        m_gamePadButtons.Reset();
        m_keyboardButtons.Update(m_keyboard->GetState());

        // Grab Controls
        controls.Exit = m_keyboardButtons.IsKeyPressed(Keyboard::Escape);
        controls.ToggleHelp = m_keyboardButtons.IsKeyPressed(Keyboard::T);
        controls.CycleViz = m_keyboardButtons.IsKeyPressed(Keyboard::Space);
        controls.CycleInst = m_keyboardButtons.IsKeyPressed(Keyboard::Tab);
        controls.InstLevelChange = m_keyboardButtons.IsKeyPressed(Keyboard::OemPlus) ? 1 : (m_keyboardButtons.IsKeyPressed(Keyboard::OemMinus) ? -1 : 0);
        controls.LodChange = m_keyboardButtons.IsKeyPressed(Keyboard::OemCloseBrackets) ? 1 : (m_keyboardButtons.IsKeyPressed(Keyboard::OemOpenBrackets) ? -1 : 0);

        // Update Camera
        m_camera.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());
    }

    // Apply controls to the app settings
    m_lodIndex = (m_lodIndex + c_lodCount + controls.LodChange) % c_lodCount;

    if (controls.InstLevelChange != 0)
    {
        m_instanceLevel = std::max(m_instanceLevel + controls.InstLevelChange, 0);
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

        // Render the instances
        m_gpuTimer->Start(commandList);

        auto& model = m_lods[m_lodIndex].Model;
        auto& meshletData = m_lods[m_lodIndex].MeshletData;

        // Set up pipeline state
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_meshShaderPSO.Get());

        commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
        commandList->SetGraphicsRootShaderResourceView(7, m_instanceBuffer->GetGPUVirtualAddress());

        // Draw each mesh in the model
        for (uint32_t j = 0; j < static_cast<uint32_t>(meshletData.size()); ++j)
        {
            commandList->SetGraphicsRootConstantBufferView(2, meshletData[j].GetMeshInfoResource()->GetGPUVirtualAddress());

            commandList->SetGraphicsRootShaderResourceView(3, model->meshes[j]->opaqueMeshParts[0]->vertexBuffer.Resource()->GetGPUVirtualAddress());
            commandList->SetGraphicsRootShaderResourceView(4, meshletData[j].GetMeshletResource()->GetGPUVirtualAddress());
            commandList->SetGraphicsRootShaderResourceView(5, meshletData[j].GetUniqueIndexResource()->GetGPUVirtualAddress());
            commandList->SetGraphicsRootShaderResourceView(6, meshletData[j].GetPrimitiveResource()->GetGPUVirtualAddress());

            // We have to break up the instances into multiple DispatchMesh calls due to the 65536 API limit.
            const uint32_t instanceCount = static_cast<uint32_t>(m_instances.size());

            uint32_t instancesPerDispatch = meshletData[j].InstancesPerDispatch(MAX_MESHLET_SIZE);
            uint32_t dispatchCount = DivRoundUp(instanceCount, instancesPerDispatch);

            for (uint32_t k = 0; k < dispatchCount; ++k)
            {
                auto subOffset = k * instancesPerDispatch;
                auto subCount = std::min(instanceCount - subOffset, instancesPerDispatch);

                commandList->SetGraphicsRoot32BitConstant(1, subCount, 0);
                commandList->SetGraphicsRoot32BitConstant(1, subOffset, 1);

                uint32_t count = meshletData[j].CalcThreadGroupCount(MAX_MESHLET_SIZE, subCount);
                commandList->DispatchMesh(count, 1, 1);
            }
        }
        m_gpuTimer->Stop(commandList);

        DrawHUD(commandList);
    }

    PIXEndEvent(commandList);

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
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::UpdateConstants(ID3D12GraphicsCommandList* commandList)
{
    // Upload constant data to the GPU
    XMMATRIX view = m_camera.GetView();
    XMMATRIX proj = m_camera.GetProjection();

    Constants constants;
    XMStoreFloat4x4(&constants.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&constants.ViewProj, XMMatrixTranspose(view * proj));
    constants.RenderMode = m_renderMode;

    auto cbMem = m_graphicsMemory->AllocateConstant(constants);

    TransitionResource(commandList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->CopyBufferRegion(m_constantBuffer.Get(), 0, cbMem.Resource(), cbMem.ResourceOffset(), cbMem.Size());
    TransitionResource(commandList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

    // Only upload instance data if we've had a change
    if (m_updateInstances)
    {
        auto instanceMem = m_graphicsMemory->Allocate(m_instances.size() * sizeof(Instance));
        std::memcpy(instanceMem.Memory(), m_instances.data(), m_instances.size() * sizeof(Instance));

        TransitionResource(commandList, m_instanceBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->CopyBufferRegion(m_instanceBuffer.Get(), 0, instanceMem.Resource(), instanceMem.ResourceOffset(), instanceMem.Size());
        TransitionResource(commandList, m_instanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

        m_updateInstances = false;
    }
}

void Sample::DrawHUD(ID3D12GraphicsCommandList* commandList)
{
    const wchar_t* c_renderModeNames[] =
    {
        L"Flat",
        L"Meshlets"
    };

    const uint32_t instanceCount = static_cast<uint32_t>(m_instances.size());

    m_hudBatch->Begin(commandList);

    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(m_displayWidth, m_displayHeight);

    // Draw a text underlay box
    m_hudBatch->Draw(m_srvPile->GetGpuHandle(SRV_WhiteTexture), { 1, 1 }, RECT{ safe.left - 10, safe.top - 10, safe.left + 335, safe.top + 270 });

    wchar_t textBuffer[128] = {};
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = DirectX::Colors::DarkKhaki;

    // Draw title.
    m_smallFont->DrawString(m_hudBatch.get(), s_sampleTitle, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    // Draw app state
    swprintf_s(textBuffer, L"Render Mode: %s", c_renderModeNames[m_renderMode]);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Instance Mode: %s", m_instMode == InstanceMode::IM_Circles ? L"Circles" : L"Cube");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Instance Level: %d", m_instanceLevel + 1);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();
    textPos.y += m_smallFont->GetLineSpacing();

    // Draw app stats
    swprintf_s(textBuffer, L"Instance Count: %d", instanceCount);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    auto primCountStr = FormatWithCommas(m_primCounts[m_lodIndex] * instanceCount);

    swprintf_s(textBuffer, L"Primitive Count: %s", primCountStr.c_str());
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Frame Time: %f", m_gpuTimer->GetAverageMS());
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    // Draw our controls at the bottom of the screen
    textPos = XMFLOAT2(float(safe.left), float(safe.bottom));
    textPos.y -= m_smallFont->GetLineSpacing();

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        // Gamepad controls
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[Menu] Show Help Screen", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();
    }
    else
    {
        // Keyboard controls
        m_hudBatch->Draw(m_srvPile->GetGpuHandle(SRV_WhiteTexture), { 1, 1 }, RECT{ safe.left - 10, safe.bottom - 230, safe.left + 400, safe.bottom + 10 });

        m_smallFont->DrawString(m_hudBatch.get(), L"Esc -        Exit Sample", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"+- -         Change Instancing Level", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"[] -           Change LOD Level", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"Space -    Cycle Render Mode", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"Tab -       Cycle Instancing Mode", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"LMouse - Rotate Camera (Hold)", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"ASWD -   Move Camera", textPos, textColor);
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
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const
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

    // Instantiate manager objects
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
    m_controlHelp = std::make_unique<Help>(L"Meshlet Instancing", nullptr, c_buttonAssignment, _countof(c_buttonAssignment));

    m_srvPile = std::make_unique<DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        128,
        DescriptorHeapIndex::SRV_Count);

    // Create the Mesh Shader Pipeline State Object
    {
        auto meshShader  = DX::ReadData(s_meshShaderFilename);
        auto pixelShader = DX::ReadData(s_pixelShaderFilename);

        // Pull the root signature directly from the mesh shader bytecode
        DX::ThrowIfFailed(device->CreateRootSignature(0, meshShader.data(), meshShader.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature        = m_rootSignature.Get();
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
        DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_meshShaderPSO.ReleaseAndGetAddressOf())));
    }

    // Create GPU resources for various purposes
    {
        // Create constant resources
        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(GetAlignedSize(sizeof(Constants)));

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_constantBuffer.ReleaseAndGetAddressOf()))
        );

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
    m_primCounts.resize(_countof(s_lodFilenames));

    for (size_t i = 0; i < m_lods.size(); ++i)
    {
        wchar_t filepath[256];
        DX::FindMediaFile(filepath, _countof(filepath), s_lodFilenames[i]);

        m_lods[i].Model = Model::CreateFromSDKMESH(device, filepath);

        wchar_t meshletFilepath[256] = {};
        swprintf_s(meshletFilepath, L"%s.bin", filepath);

        m_lods[i].MeshletData = MeshletSet::Read(meshletFilepath);
    }

    // Upload graphics resources to the GPU
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        // Optimize meshes for rendering
        for (size_t i = 0; i < m_lods.size(); ++i)
        {
            m_lods[i].Model->LoadStaticBuffers(device, resourceUpload, true);
            m_primCounts[i] = m_lods[i].Model->meshes[0]->opaqueMeshParts[0]->indexCount / 3;

            for (auto& m : m_lods[i].MeshletData)
            {
                m.CreateResources(device, resourceUpload);
            }
        }

        // Upload textures to GPU.
        m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvPile->Heap());

        auto texOffsets = std::vector<size_t>(m_lods.size());
        for (size_t i = 0; i < m_lods.size(); ++i)
        {
            if (!m_lods[i].Model->textureNames.empty())
            {
                size_t _;
                m_srvPile->AllocateRange(m_lods[i].Model->textureNames.size(), texOffsets[i], _);

                m_lods[i].Model->LoadTextures(*m_textureFactory, int(texOffsets[i]));
            }
        }

        // Create our HUD objects
        auto backBufferRts = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        m_controlHelp->RestoreDevice(device, resourceUpload, backBufferRts);

        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

        // Load and upload our UI fonts
        wchar_t strFilePath[MAX_PATH] = {};
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_Font),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_Font));

        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_CtrlFont),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_CtrlFont));

        // Upload a single grey pixel to the underlay resource
        uint8_t color[4] = { 5, 5, 5, 196 };

        D3D12_SUBRESOURCE_DATA data = {};
        data.pData      = color;
        data.RowPitch   = c_textureDataPitchAlign;
        data.SlicePitch = 0;

        resourceUpload.Upload(m_whiteTexture.Get(), 0, &data, 1);
        resourceUpload.Transition(m_whiteTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        resourceUpload.End(m_deviceResources->GetCommandQueue());
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
    m_camera.SetProjectionParameters(XM_PIDIV4, 1.0f, 10000.0f, true);

    m_camera.SetLookAt(Vector3(-200.0f, 200.0f, -200.0f), Vector3::Up * 40.0f);
    m_camera.SetSensitivity(500.0f, 100.0f, 1000.0f, 10.0f);
}

void Sample::OnDeviceLost()
{
    m_gpuTimer.reset();
    m_graphicsMemory.reset();

    m_rootSignature.Reset();
    m_meshShaderPSO.Reset();
    m_constantBuffer.Reset();
    m_instanceBuffer.Reset();
    m_whiteTexture.Reset();

    m_hudBatch.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();

    m_lods.resize(0);
    m_primCounts.resize(0);
    m_textureFactory.reset();
    m_srvPile.reset();

    m_instances.resize(0);
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion

void Sample::RegenerateInstances()
{
    m_updateInstances = true;

    const float radius = m_lods[m_lodIndex].Model->meshes[0]->boundingSphere.Radius;
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

            Instance& inst = m_instances[i];
            inst.World = world.Transpose();
            inst.WorldInvTranspose = world.Transpose().Invert().Transpose();

            dist += spacing;
        }
    }
    else if (m_instMode == InstanceMode::IM_Circles)
    {
        // Create the instances in growing concentric circles
        m_instances.clear();

        float dist = 0.0f;
        for (uint32_t i = 0; i <= static_cast<uint32_t>(m_instanceLevel); ++i)
        {
            float totalCircum = dist * XM_2PI;

            uint32_t count = static_cast<uint32_t>(std::max(1.0f, totalCircum / spacing));
            float anglePerDiv = XM_2PI / count;

            for (uint32_t j = 0; j < count; ++j)
            {
                float angle = j * anglePerDiv;

                Matrix world = Matrix::CreateTranslation(dist, 0.0f, 0.0f) * Matrix::CreateRotationY(angle);

                Instance inst;
                inst.World = world.Transpose();
                inst.WorldInvTranspose = world.Transpose().Invert().Transpose();

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
        }
    }

    const uint32_t instanceCount = static_cast<uint32_t>(m_instances.size());
    const uint32_t instanceBufferSize = (uint32_t)GetAlignedSize(instanceCount * sizeof(Instance));

    // Only recreate instance-sized buffers if necessary.
    if (!m_instanceBuffer || m_instanceBuffer->GetDesc().Width < instanceBufferSize)
    {
        m_deviceResources->WaitForGpu();

        // Create/re-create the instance buffer
        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(GetAlignedSize(m_instances.size() * sizeof(Instance)));

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateCommittedResource(
                &defaultHeap,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_instanceBuffer.ReleaseAndGetAddressOf())));
    }
}
