//--------------------------------------------------------------------------------------
// MeshletCull.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MeshletCull.h"

#include "ATGColors.h"
#include "DDSTextureLoader.h"
#include "FindMedia.h"

#include "Shared.h"

extern void ExitSample();

using namespace ATG;
using namespace DirectX;

using Microsoft::WRL::ComPtr;
using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

namespace 
{
    //--------------------------------------
    // Definitions

    static const wchar_t* s_sampleTitle = L"Meshlet Cull";

    static const wchar_t* s_meshletCullAsFilename = L"MeshletCullAS.cso";
    static const wchar_t* s_meshletDrawMsFilename = L"BasicMeshletMS.cso";
    static const wchar_t* s_basicdrawPsFilename = L"BasicMeshletPS.cso";
    
    static const wchar_t* s_meshFilenames[] = 
    {
#if _GAMING_DESKTOP
        L"ATGDragon\\Dragon_LOD0.sdkmesh",
        L"ATGDragon\\Dragon_LOD1.sdkmesh",
        L"ATGDragon\\Dragon_LOD2.sdkmesh",
        L"ATGDragon\\Dragon_LOD3.sdkmesh",
        L"ATGDragon\\Dragon_LOD4.sdkmesh",
        L"ATGDragon\\Dragon_LOD5.sdkmesh",
        L"Camera\\Camera.sdkmesh"
#else
        L"Dragon_LOD0.sdkmesh",
        L"Dragon_LOD1.sdkmesh",
        L"Dragon_LOD2.sdkmesh",
        L"Dragon_LOD3.sdkmesh",
        L"Dragon_LOD4.sdkmesh",
        L"Dragon_LOD5.sdkmesh",
        L"Camera.sdkmesh"
#endif
    };

    const uint32_t c_dragonLODStart = 0;
    const uint32_t c_dragonLODCount = 6;

    struct ObjectDefinition
    {
        uint32_t ModelIndex;
        VQS      World;
    };

    const ObjectDefinition c_objectDefs[] = 
    {
        { c_dragonLODStart, VQS::CreateScale(0.2f) },
        { 6, VQS::Identity() }
    };

    enum SceneIndex
    {
        SI_Model = 0,
        SI_Camera
    };

    enum DescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_Crosshair,
        SRV_Count
    };

    const HelpButtonAssignment c_buttonAssignment[] =
    {
        { HelpID::A_BUTTON, L"Toggle Culling" },
        { HelpID::B_BUTTON, L"Switch Cull Camera" },
        { HelpID::Y_BUTTON, L"Pick Meshlet" },
        { HelpID::X_BUTTON, L"Switch Render Mode" },
        { HelpID::DPAD_LEFT, L"Camera Pan" },
        { HelpID::LEFT_STICK, L"Camera Zoom" },
        { HelpID::RIGHT_STICK, L"Camera Rotate" },
        { HelpID::LEFT_SHOULDER, L"Control Debug Camera" },
        { HelpID::RIGHT_SHOULDER, L"Cycle LOD" },
        { HelpID::RIGHT_TRIGGER, L"Cycle LOD" },
        { HelpID::MENU_BUTTON, L"Show Help Screen" },
        { HelpID::VIEW_BUTTON, L"Exit Sample" },
    };

    inline uint32_t DivRoundUp(uint32_t num, uint32_t den) { return (num + den - 1) / den; }

    // Intersection tests
    bool RayIntersectSphere(FXMVECTOR o, FXMVECTOR d, FXMVECTOR s)
    {
        XMVECTOR l = XMVectorSubtract(o, s);
        XMVECTOR r = XMVectorSplatW(s);
        XMVECTOR a = XMVector3Dot(d, d);
        XMVECTOR b = XMVectorScale(XMVector3Dot(l, d), 2.0f);
        XMVECTOR c = XMVectorSubtract(XMVector3Dot(l, l), XMVectorMultiply(r, r));

        // disc = b * b - 4 * a * c;
        XMVECTOR disc = XMVectorSubtract(XMVectorMultiply(b, b), XMVectorScale(XMVectorMultiply(a, c), 4.0f));
        return !XMVector4Less(disc, g_XMZero);
    }

    XMVECTOR RayIntersectTriangle(FXMVECTOR o, FXMVECTOR d, FXMVECTOR p0, GXMVECTOR p1, HXMVECTOR p2)
    {
        XMVECTOR edge1 = XMVectorSubtract(p1, p0);
        XMVECTOR edge2 = XMVectorSubtract(p2, p0);

        XMVECTOR h = XMVector3Cross(d, edge2);
        XMVECTOR a = XMVector3Dot(edge1, h);
        if (XMVector4Less(XMVectorAbs(a), g_XMEpsilon))
            return g_XMQNaN;

        XMVECTOR f = XMVectorReciprocal(a);
        XMVECTOR s = XMVectorSubtract(o, p0);
        XMVECTOR u = XMVectorMultiply(f, XMVector3Dot(s, h));
        if (XMVector4Less(u, g_XMZero) || XMVector4Greater(u, g_XMOne))
            return g_XMQNaN;

        XMVECTOR q = XMVector3Cross(s, edge1);
        XMVECTOR v = XMVectorMultiply(f, XMVector3Dot(d, q));
        if (XMVector4Less(v, g_XMZero) || XMVector4Greater(XMVectorAdd(u, v), g_XMOne))
            return g_XMQNaN;

        // At this stage we can compute t to find out where the intersection point is on the line.
        XMVECTOR t = XMVectorMultiply(f, XMVector3Dot(edge2, q));
        if (XMVector4Greater(t, g_XMEpsilon) && XMVector4Less(t, XMVectorReciprocal(g_XMEpsilon))) // ray intersection
        {
            return t;
        }

        return g_XMQNaN; // This means that there is a line intersection but not a ray intersection.
    }

#pragma warning (disable : 4061)
    uint32_t FormatSize(DXGI_FORMAT f)
    {
        switch (f)
        {
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return 16;
        case DXGI_FORMAT_R32G32B32_FLOAT: return 12;
        case DXGI_FORMAT_R32G32_FLOAT: return 8;
        case DXGI_FORMAT_R32_FLOAT: return 4;
        default: assert(false);
        }
        return 0;
    }
#pragma warning (default : 4061)

    uint32_t ComputeSemanticByteOffset(const std::vector<D3D12_INPUT_ELEMENT_DESC>& vbDesc, const char* name)
    {
        uint32_t offset = 0;
        for (auto& decl : vbDesc)
        {
            if (std::strcmp(decl.SemanticName, name) == 0)
            {
                if (decl.AlignedByteOffset != ~0u)
                {
                    offset = decl.AlignedByteOffset;
                }

                return offset;
            }

            offset += FormatSize(decl.Format);
        }

        return uint32_t(-1);
    }
}


Sample::Sample() noexcept(false) 
    : m_deviceResources(std::make_unique<DX::DeviceResources>())
    , m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
    , m_countsData(nullptr)
    , m_renderHelp(false)
    , m_cull(true)
    , m_debugCull(true)
    , m_renderMode(RM_Meshlets)
    , m_lodIndex(0)
    , m_highlightedIndex(uint32_t(-1))
    , m_selectedIndex(uint32_t(-1))
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

    // Input-agnostic app controls
    struct AppControls
    {
        bool ToggleHelp;
        bool ToggleCull;
        bool ToggleDebugCull;
        bool CycleViz;
        int  LodChange;
        bool SelectMeshlet;
        bool Exit;
    } controls;

    float elapsedTime = float(timer.GetElapsedSeconds());
    auto pad = m_gamePad->GetState(0);

    // Populate our app controls using per-input logic
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        // Grab Controls
        controls.Exit = m_gamePadButtons.view == ButtonState::PRESSED;
        controls.ToggleHelp = m_gamePadButtons.menu == ButtonState::PRESSED;
        controls.ToggleCull = m_gamePadButtons.a == ButtonState::PRESSED;
        controls.ToggleDebugCull = m_gamePadButtons.b == ButtonState::PRESSED;
        controls.CycleViz = m_gamePadButtons.x == ButtonState::PRESSED;
        controls.LodChange = m_gamePadButtons.rightShoulder == ButtonState::PRESSED ? 1 : (m_gamePadButtons.rightTrigger == ButtonState::PRESSED ? -1 : 0);
        controls.SelectMeshlet = m_gamePadButtons.y == ButtonState::PRESSED;

        // Update camera
        DX::OrbitCamera& cam = pad.IsLeftShoulderPressed() ? m_debugCamera : m_camera;
        cam.SetFlags(0);
        cam.Update(elapsedTime, pad);
    }
    else
    {
        auto mouse = m_mouse->GetState();
        auto kb = m_keyboard->GetState();
        m_keyboardButtons.Update(kb);

        // Grab Controls
        controls.Exit = m_keyboardButtons.IsKeyPressed(Keyboard::Escape);
        controls.ToggleCull = m_keyboardButtons.IsKeyPressed(Keyboard::Tab);
        controls.ToggleDebugCull = m_keyboardButtons.IsKeyPressed(Keyboard::Q);
        controls.CycleViz = m_keyboardButtons.IsKeyPressed(Keyboard::Space);
        controls.LodChange = m_keyboardButtons.IsKeyPressed(Keyboard::OemPlus) ? 1 : (m_keyboardButtons.IsKeyPressed(Keyboard::OemMinus) ? -1 : 0);
        controls.SelectMeshlet = mouse.rightButton;
        controls.ToggleHelp = m_keyboardButtons.IsKeyPressed(Keyboard::T);

        // Update camera
        DX::OrbitCamera& cam = kb.LeftShift ? m_debugCamera : m_camera;
        cam.SetFlags(DX::OrbitCamera::c_FlagsDisableRadiusControl);
        cam.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());

        float scrollVal = float(mouse.scrollWheelValue) / 120.0f;
        m_mouse->ResetScrollWheelValue();

        float radius = XMVectorGetX(XMVector3Length(XMVectorSubtract(cam.GetFocus(), cam.GetPosition())));
        radius = std::max(radius - scrollVal, 0.1f);

        cam.SetRadius(radius);
    }

    // Apply controls to the app settings
    m_lodIndex = (m_lodIndex + c_dragonLODCount + controls.LodChange) % c_dragonLODCount;

    if (controls.ToggleCull)
    {
        m_cull = !m_cull;
    }

    if (controls.ToggleDebugCull)
    {
        m_debugCull = !m_debugCull;

        if (m_debugCull)
        {
            m_cull = true;
        }
    }

    if (controls.LodChange != 0)
    {
        m_selectedIndex = uint32_t(-1);
    }

    if (controls.CycleViz)
    {
        m_renderMode = static_cast<RenderMode>((m_renderMode + 1u) % RM_Count);
    }

    if (controls.ToggleHelp)
    {
        m_renderHelp = !m_renderHelp;
    }

    if (controls.SelectMeshlet)
    {
        m_selectedIndex = m_highlightedIndex;
    }

    if (controls.Exit)
    {
        ExitSample();
    }

    // Update scene object items
    m_scene[SI_Model].ModelIndex = c_dragonLODStart + m_lodIndex;
    m_scene[SI_Camera].World = XMMatrixInverse(nullptr, m_debugCamera.GetView());

    for (auto& object : m_scene)
    {
        Model::UpdateEffectMatrices(object.Effects, object.World.ToMatrix(), m_camera.GetView(), m_camera.GetProjection());
    }

    if (m_renderMode == RM_Meshlets)
    {
        Pick();
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

    // Update constant buffer variables
    UpdateConstants(commandList);

    // Set up the root signature & pipeline state
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvPile->Heap() };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);

    // Draw the debug view frustum
    m_frustumDraw.Draw(commandList);

    // Render the help menu if specified
    if (m_renderHelp)
    {
        m_controlHelp->Render(commandList);
    }
    else
    {
        // Render the scene
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_msPso.Get());

        commandList->SetGraphicsRootConstantBufferView(0, m_constants->GetGPUVirtualAddress());
        commandList->SetGraphicsRootUnorderedAccessView(8, m_countsUAV->GetGPUVirtualAddress());

        m_gpuTimer->Start(commandList);

        for (uint32_t i = 0; i < m_scene.size(); ++i)
        {
            auto& object = m_scene[i];

            auto& model = *m_models[object.ModelIndex].Model;
            auto& meshletData = m_models[object.ModelIndex].MeshletData;

            if (!meshletData.empty())
            {
                // Render using Mesh Shaders when meshlets are available
                commandList->SetGraphicsRootConstantBufferView(1, object.ConstantBuffer->GetGPUVirtualAddress());

                for (uint32_t j = 0; j < meshletData.size(); ++j)
                {
                    commandList->SetGraphicsRootConstantBufferView(2, meshletData[j].GetMeshInfoResource()->GetGPUVirtualAddress());
                    commandList->SetGraphicsRootShaderResourceView(3, model.meshes[j]->opaqueMeshParts[0]->vertexBuffer.Resource()->GetGPUVirtualAddress());
                    commandList->SetGraphicsRootShaderResourceView(4, meshletData[j].GetMeshletResource()->GetGPUVirtualAddress());
                    commandList->SetGraphicsRootShaderResourceView(5, meshletData[j].GetUniqueIndexResource()->GetGPUVirtualAddress());
                    commandList->SetGraphicsRootShaderResourceView(6, meshletData[j].GetPrimitiveResource()->GetGPUVirtualAddress());
                    commandList->SetGraphicsRootShaderResourceView(7, meshletData[j].GetCullDataResource()->GetGPUVirtualAddress());

                    // Calculate our final threadgroup dispatch count
                    const uint32_t groupCount = DivRoundUp(meshletData[j].GetMeshletCount(), AS_GROUP_SIZE);
                    commandList->DispatchMesh(groupCount, 1, 1);
                }
            }
            else
            {
                // Render using a standard VS - PS pipeline.
                m_models[object.ModelIndex].Model->DrawOpaque(commandList, object.Effects.begin());
            }
        }

        m_gpuTimer->Stop(commandList);

        // Copy the LOD count buffer back to the CPU.
        TransitionResource(commandList, m_countsUAV.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->CopyResource(m_countsReadback.Get(), m_countsUAV.Get());
        TransitionResource(commandList, m_countsUAV.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // Visualize cull data if it's enabled.
        if (m_renderMode == RM_Meshlets && m_selectedIndex != uint32_t(-1))
        {
            auto& object = m_scene[SI_Model];
            auto& meshletData = m_models[object.ModelIndex].MeshletData[0];

            m_cullDataDraw.SetConstants(DirectX::Colors::Yellow, m_camera, object.World);
            m_cullDataDraw.Draw(commandList, meshletData, m_selectedIndex, 1);
        }

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
    DX::OrbitCamera& cullCam = m_debugCull ? m_debugCamera : m_camera;

    XMMATRIX view = m_camera.GetView();
    XMMATRIX proj = m_camera.GetProjection();
    XMMATRIX viewProj = view * proj;
    XMMATRIX cViewProj = cullCam.GetView() * cullCam.GetProjection();

    // Update camera frustum
    // http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    XMMATRIX vp = XMMatrixTranspose(cViewProj);

    XMVECTOR planes[6] =
    {
        XMPlaneNormalize(XMVectorAdd(vp.r[3], vp.r[0])), // Left
        XMPlaneNormalize(XMVectorSubtract(vp.r[3], vp.r[0])), // Right
        XMPlaneNormalize(XMVectorAdd(vp.r[3], vp.r[1])), // Bottom
        XMPlaneNormalize(XMVectorSubtract(vp.r[3], vp.r[1])), // Top
        XMPlaneNormalize(vp.r[2]),           // Near
        XMPlaneNormalize(XMVectorSubtract(vp.r[3], vp.r[2])), // Far
    };

    m_frustumDraw.Update(viewProj, planes);

    // View constants
    {
        auto cbMem = m_graphicsMemory->AllocateConstant<Constants>();
        Constants& constants = *static_cast<Constants*>(cbMem.Memory());

        XMStoreFloat4x4(&constants.View, XMMatrixTranspose(view));
        XMStoreFloat4x4(&constants.ViewProj, XMMatrixTranspose(viewProj));
        XMStoreFloat3(&constants.CullViewPosition, cullCam.GetPosition());
        XMStoreFloat3(&constants.ViewPosition, m_camera.GetPosition());

        for (uint32_t i = 0; i < _countof(planes); ++i)
        {
            XMStoreFloat4(&constants.Planes[i], planes[i]);
        }

        constants.Cull = m_cull;
        constants.DebugCull = m_debugCull;
        constants.RenderMode = m_renderMode;
        constants.HighlightedIndex = m_highlightedIndex;
        constants.SelectedIndex = m_selectedIndex;

        TransitionResource(commandList, m_constants.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->CopyBufferRegion(m_constants.Get(), 0, cbMem.Resource(), cbMem.ResourceOffset(), cbMem.Size());
        TransitionResource(commandList, m_constants.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    // Per scene object constants
    for (uint32_t i = 0; i < m_scene.size(); ++i)
    {
        auto& object = m_scene[i];

        auto cbMem = m_graphicsMemory->AllocateConstant<Instance>();
        Instance& constants = *static_cast<Instance*>(cbMem.Memory());

        XMMATRIX world = object.World.ToMatrix();

        constants.Scale = object.World.Scale;
        XMStoreFloat4x4(&constants.World, XMMatrixTranspose(world));
        XMStoreFloat4x4(&constants.WorldInvTrans, world);

        TransitionResource(commandList, object.ConstantBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->CopyBufferRegion(object.ConstantBuffer.Get(), 0, cbMem.Resource(), cbMem.ResourceOffset(), cbMem.Size());
        TransitionResource(commandList, object.ConstantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    }
}

void Sample::DrawHUD(ID3D12GraphicsCommandList* commandList)
{
    const wchar_t* c_renderModeNames[] =
    {
        L"Flat",
        L"Meshlets"
    };

    // Compute our culled meshlets counts from the read back data
    const uint32_t modelIndex = m_scene[SI_Model].ModelIndex;
    const uint32_t meshletCount = m_models[modelIndex].MeshletData[0].GetMeshletCount();
    const uint32_t groupCount = DivRoundUp(meshletCount, AS_GROUP_SIZE);

    uint32_t frustumCulled = 0;
    uint32_t backfaceCulled = 0;
    for (uint32_t i = 0; i < groupCount; ++i)
    {
        frustumCulled  += m_countsData[i * 2 + 0];
        backfaceCulled += m_countsData[i * 2 + 1];
    }
    uint32_t totalCulled = frustumCulled + backfaceCulled;
    uint32_t totalRendered = meshletCount - totalCulled;

    // Set up some 
    const RECT safe = SimpleMath::Viewport::ComputeTitleSafeArea(m_displayWidth, m_displayHeight);
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = DirectX::Colors::DarkKhaki;

    m_hudBatch->Begin(commandList);

    // Draw our crosshair if we're in the correct view mode
    if (m_renderMode == RM_Meshlets)
    {
        auto desc = m_crosshair->GetDesc();
        const XMUINT2 texSize = { static_cast<uint32_t>(desc.Width), desc.Height };
        const uint32_t pixelSize = 16;

        XMFLOAT2 point;
        XMStoreFloat2(&point, GetSamplePoint());

        const int halfSize = pixelSize / 2;
        RECT loc = { int(point.x) - halfSize, int(point.y) - halfSize, int(point.x) + halfSize, int(point.y) + halfSize };
        m_hudBatch->Draw(m_srvPile->GetGpuHandle(SRV_Crosshair), texSize, loc);
    }

    // Draw our settings and stats
    wchar_t textBuffer[128] = {};

    m_smallFont->DrawString(m_hudBatch.get(), s_sampleTitle, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    // Draw app state
    swprintf_s(textBuffer, L"Culling: %s", m_cull ? L"Enabled" : L"Disabled");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Cull Camera: %s", m_debugCull ? L"Debug" : L"Main");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Render Mode: %s", c_renderModeNames[m_renderMode]);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    // Draw app stats
    swprintf_s(textBuffer, L"Meshlets: %d / %d", totalRendered, meshletCount);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Culled - Frustum: %d, Backface: %d", frustumCulled, backfaceCulled);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Frame Time: %f", m_gpuTimer->GetAverageMS());
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    // Draw the controls
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
        m_smallFont->DrawString(m_hudBatch.get(), L"Esc -         Exit Sample", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"RMouse - Pick Meshlet", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"+- -          Cycle LODs", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"LShift -     Control Debug Camera", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"WASD -    Move Camera", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"LMouse - Rotate Camera", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"Q -           Switch Cull Camera", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"Space -    Switch Render Mode", textPos, textColor);
        textPos.y -= m_smallFont->GetLineSpacing();

        m_smallFont->DrawString(m_hudBatch.get(), L"Tab -       Toggle Culling", textPos, textColor);
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
    auto device = static_cast<ID3D12Device2*>(m_deviceResources->GetD3DDevice());

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
    m_commonStates = std::make_unique<CommonStates>(device);
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
    m_controlHelp = std::make_unique<Help>(L"Meshlet Cull", nullptr, c_buttonAssignment, _countof(c_buttonAssignment));

    m_srvPile = std::make_unique<DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        128,
        DescriptorHeapIndex::SRV_Count);

    m_cullDataDraw.CreateDeviceResources(*m_deviceResources);
    m_frustumDraw.CreateDeviceResources(*m_deviceResources);

    // Create the Mesh Shader Pipeline State Object
    {
        auto ampShader   = DX::ReadData(s_meshletCullAsFilename);
        auto meshShader  = DX::ReadData(s_meshletDrawMsFilename);
        auto pixelShader = DX::ReadData(s_basicdrawPsFilename);

        // Pull the root signature directly from the mesh shader bytecode
        DX::ThrowIfFailed(device->CreateRootSignature(0, meshShader.data(), meshShader.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        // Disable culling so we can see the backside of geometry through the culled mesh.
        CD3DX12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        rasterDesc.CullMode = D3D12_CULL_MODE_NONE;

        // Populate the Mesh Shader PSO descriptor
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature        = m_rootSignature.Get();
        psoDesc.AS                    = { ampShader.data(), ampShader.size() };
        psoDesc.MS                    = { meshShader.data(), meshShader.size() };
        psoDesc.PS                    = { pixelShader.data(), pixelShader.size() };
        psoDesc.NumRenderTargets      = 1;
        psoDesc.RTVFormats[0]         = m_deviceResources->GetBackBufferFormat();
        psoDesc.DSVFormat             = m_deviceResources->GetDepthBufferFormat();
        psoDesc.RasterizerState       = rasterDesc;
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

    // Load the models and mesh data
    m_models.resize(_countof(s_meshFilenames));

    for (uint32_t i = 0; i < m_models.size(); ++i)
    {
        wchar_t filepath[256];
        DX::FindMediaFile(filepath, _countof(filepath), s_meshFilenames[i]);

        m_models[i].Model = Model::CreateFromSDKMESH(device, filepath);

        wchar_t meshletFilepath[256] = {};
        swprintf_s(meshletFilepath, L"%s.bin", filepath);

        m_models[i].MeshletData = MeshletSet::Read(meshletFilepath);
    }

    // Upload graphics resources to the GPU
    auto texOffsets = std::vector<size_t>(m_models.size());

    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        // Optimize meshes for rendering
        for (uint32_t i = 0; i < m_models.size(); ++i)
        {
            m_models[i].Model->LoadStaticBuffers(device, resourceUpload, true);

            for (auto& m : m_models[i].MeshletData)
            {
                m.CreateResources(device, resourceUpload);
            }
        }

        // Upload textures to GPU.
        m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvPile->Heap());

        for (uint32_t i = 0; i < m_models.size(); ++i)
        {
            if (!m_models[i].Model->textureNames.empty())
            {
                size_t _;
                m_srvPile->AllocateRange(m_models[i].Model->textureNames.size(), texOffsets[i], _);

                m_models[i].Model->LoadTextures(*m_textureFactory, int(texOffsets[i]));
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

        // Load our targeting circle UI
        DX::FindMediaFile(strFilePath, MAX_PATH, L"RedCircle.DDS");
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, strFilePath, &m_crosshair));
        device->CreateShaderResourceView(m_crosshair.Get(), nullptr, m_srvPile->GetCpuHandle(SRV_Crosshair));

        resourceUpload.End(m_deviceResources->GetCommandQueue());
    }

    // Instantiate scene objects, effects, and create per-object constant buffers
    {
        auto effectFactory = EffectFactory(m_srvPile->Heap(), m_commonStates->Heap());

        auto objectRTState = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        auto objectPSD = EffectPipelineStateDescription(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            objectRTState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);


        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignUp(sizeof(Instance), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

        m_scene.resize(_countof(c_objectDefs));
        for (uint32_t i = 0; i < m_scene.size(); i++)
        {
            uint32_t index = c_objectDefs[i].ModelIndex;
            assert(index < m_models.size());

            m_scene[i].World = c_objectDefs[i].World;
            m_scene[i].ModelIndex = index;
            m_scene[i].Effects = m_models[index].Model->CreateEffects(effectFactory, objectPSD, objectPSD, int(texOffsets[index]));

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeap,
                D3D12_HEAP_FLAG_NONE,
                &cbDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_scene[i].ConstantBuffer.ReleaseAndGetAddressOf())));
        }
    }

    // Create GPU resources for various purposes
    {
        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignUp(sizeof(Constants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_constants.ReleaseAndGetAddressOf()))
        );

        // Our culling counts readback buffer must support the maximum number of meshlets we intend to render.
        const uint32_t maxMeshletCount = m_models[c_dragonLODStart].MeshletData[0].GetMeshletCount();
        const uint32_t maxGroupCount = DivRoundUp(maxMeshletCount, AS_GROUP_SIZE);
        const uint32_t countsBufferSize = maxGroupCount * 2 * sizeof(uint32_t);

        auto countsDesc = CD3DX12_RESOURCE_DESC::Buffer(countsBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &countsDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_countsUAV.ReleaseAndGetAddressOf())));

        // Counts readback resource
        auto readbackHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        countsDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &readbackHeap,
            D3D12_HEAP_FLAG_NONE,
            &countsDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_countsReadback.ReleaseAndGetAddressOf())));

        m_countsReadback->Map(0, nullptr, (void**)&m_countsData);
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT size = m_deviceResources->GetOutputSize();
    m_displayWidth = static_cast<uint32_t>(size.right - size.left);
    m_displayHeight = static_cast<uint32_t>(size.bottom - size.top);

    m_controlHelp->SetWindow(size);

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    // Set main camera properties
    m_camera.SetWindow(int(m_displayWidth), int(m_displayHeight));
    m_camera.SetProjectionParameters(XM_PIDIV4, 0.25f, 1000.0f, true);

    m_camera.SetFocus(XMVectorSet(0, 10, 0, 0));
    m_camera.SetRadius(50.0f);
    m_camera.SetRadiusRate(10.0f);
    m_camera.SetSensitivity(50.0f, 25.0f, 100.0f, 1.0f);
    m_camera.SetRotation(XMQuaternionRotationAxis(g_XMIdentityR1, XM_PIDIV4 * 3.0f));

    // Set debug camera properties
    m_debugCamera.SetWindow(int(m_displayWidth), int(m_displayHeight));
    m_debugCamera.SetProjectionParameters(XM_PIDIV4, 0.25f, 100.0f, true);

    m_debugCamera.SetFocus(XMVectorSet(0, 10, 0, 0));
    m_debugCamera.SetRadius(20.0f);
    m_debugCamera.SetRadiusRate(10.0f);
    m_debugCamera.SetSensitivity(50.0f, 25.0f, 100.0f, 1.0f);
}

void Sample::OnDeviceLost()
{
    m_gpuTimer.reset();
    m_graphicsMemory.reset();
    m_commonStates.reset();

    m_rootSignature.Reset();
    m_msPso.Reset();
    m_constants.Reset();

    m_countsUAV.Reset();
    m_countsReadback.Reset();
    m_countsData = nullptr;

    m_hudBatch.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();

    m_models.resize(0);
    m_textureFactory.reset();
    m_srvPile.reset();

    m_scene.resize(0);
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion

XMVECTOR Sample::GetSamplePoint() const
{
    // Get a sample point to query the scene for meshlet intersections for picking
    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        return XMVectorSet(float(m_displayWidth / 2), float(m_displayHeight / 2), 1, 1);
    }
    else
    {
        auto state = m_mouse->GetState();
        return XMVectorSet(float(state.x), float(state.y), 1, 1);
    }
}

void Sample::Pick()
{
    m_highlightedIndex = uint32_t(-1);

    // Determine our ray cast location
    XMVECTOR sampleSS = GetSamplePoint();
    XMVECTOR sampleWS = XMVector3Unproject(sampleSS, 0, 0, float(m_displayWidth), float(m_displayHeight), 0, 1, m_camera.GetProjection(), m_camera.GetView(), XMMatrixIdentity());

    XMVECTOR viewPosWS = m_camera.GetPosition();
    XMVECTOR viewDirWS = XMVector3Normalize(XMVectorSubtract(sampleWS, viewPosWS));

    XMVECTOR minT = g_XMFltMax;

    auto& obj = m_scene[SI_Model];
    auto& model = m_models[obj.ModelIndex];

    // Grab the vertex positions array
    auto& mesh = model.Model->meshes[0]->opaqueMeshParts[0];
    assert(mesh->vbDecl != nullptr);

    uint8_t* vbMem = static_cast<uint8_t*>(mesh->vertexBuffer.Memory());
    uint32_t stride = mesh->vertexStride;
    uint32_t offset = ComputeSemanticByteOffset(*mesh->vbDecl, "SV_Position");
    assert(offset != uint32_t(-1));

    // Grab the meshlet data buffers
    auto& meshletSet = model.MeshletData[0];
    auto& meshlets = meshletSet.GetMeshlets();
    auto& culldata = meshletSet.GetCullData();

    // Transform ray into object space for intersection tests
    XMMATRIX invWorld = obj.World.ToInverseMatrix();
    XMVECTOR dir = XMVector3Normalize(XMVector3TransformNormal(viewDirWS, invWorld));
    XMVECTOR org = XMVector3TransformCoord(viewPosWS, invWorld);

    for (uint32_t i = 0; i < meshletSet.GetMeshletCount(); ++i)
    {
        auto& meshlet = meshlets[i];
        auto& cull = culldata[i];

        // Quick narrow-phase test against the meshlet's sphere bounds.
        if (!RayIntersectSphere(org, dir, XMLoadFloat4(&cull.BoundingSphere)))
        {
            continue;
        }

        // Test each triangle of the meshlet.
        for (uint32_t j = 0; j < meshlet.PrimCount; ++j)
        {
            uint32_t i0, i1, i2;
            meshletSet.GetPrimitive(meshlet.PrimOffset + j, i0, i1, i2);

            uint32_t v0 = meshletSet.GetVertexIndex(meshlet.VertOffset + i0);
            uint32_t v1 = meshletSet.GetVertexIndex(meshlet.VertOffset + i1);
            uint32_t v2 = meshletSet.GetVertexIndex(meshlet.VertOffset + i2);

            XMVECTOR p0 = XMLoadFloat3((XMFLOAT3*)(vbMem + v0 * stride + offset));
            XMVECTOR p1 = XMLoadFloat3((XMFLOAT3*)(vbMem + v1 * stride + offset));
            XMVECTOR p2 = XMLoadFloat3((XMFLOAT3*)(vbMem + v2 * stride + offset));

            XMVECTOR t = RayIntersectTriangle(org, dir, p0, p1, p2);
            if (XMVector4Less(t, minT))
            {
                minT = t;
                m_highlightedIndex = i;
            }
        }
    }
}
