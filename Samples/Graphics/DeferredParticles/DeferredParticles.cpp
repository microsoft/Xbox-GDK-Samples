//--------------------------------------------------------------------------------------
// DeferredParticles.cpp
//
// Demonstrates how to render deferred lit particles.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DeferredParticles.h"

// C4238: nonstandard extension used: class rvalue used as lvalue
#pragma warning(disable : 4238)

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* g_sampleTitle = L"Deferred Particles";

    constexpr float g_minOrbitRadius = 50.0f;
    constexpr float g_maxOrbitRadius = 600.0f;
    constexpr float g_maxCameraHeight = 250.0f;
    constexpr float g_minCameraHeight = 25.0f;

    static const XMVECTOR g_lightDirection = XMVectorSet(0.15f, -2.0f, 3.0f, 0.0f);
    static const XMVECTOR g_lightColor = g_XMOne;
    static const XMVECTOR g_worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    const static wchar_t* s_modelPaths[] =
    {
        L"Terrain.sdkmesh",
        L"Tank_Barricades.sdkmesh",
        L"desertsky.sdkmesh",
    };

    // Mapping of scene objects to model index.
    struct ObjectDefinition
    {
        using SetupPipelineCallback = void (Sample::*)(ID3D12GraphicsCommandList*, const ModelMeshPart&, int instIndex, int texOffset);

        int MeshIndex;
        float ScaleFactor;
        SetupPipelineCallback Callback;
    };

    // Basic scene definition that enables associating additional data with the scene objects.
    const ObjectDefinition s_sceneDefinition[] =
    {
        { 0, 0.1f, &Sample::SetupPipelineScene },
        { 1, 0.1f, &Sample::SetupPipelineScene },
        { 2, 5.0f, &Sample::SetupPipelineSky },
    };
}

Sample::Sample() noexcept(false)
    : m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
    , m_orbitRadius(300.0f)
    , m_orbitAngle(0.0f)
    , m_cameraHeight(150.0f)
    , m_isPaused(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

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

    m_timer.Tick([&]() { Update(m_timer); });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(const DX::StepTimer& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_particleWorld.ToggleDeferred();
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_isPaused = !m_isPaused;
        }

        // Our orbit radius is adjusted by left stick y-axis input, and is clamped to an allowed range
        m_orbitRadius -= pad.thumbSticks.leftY * elapsedTime * 250.0f;
        m_orbitRadius = std::max(std::min(g_maxOrbitRadius, m_orbitRadius), g_minOrbitRadius);

        // Our orbit angle is adjusted by left-stick x-axis.
        m_orbitAngle += pad.thumbSticks.leftX * elapsedTime * 5.0f;

        // Our camera height is adjusted by the right-stick y axis, and is clamped to an allowed range.
        m_cameraHeight += pad.thumbSticks.rightY * elapsedTime * 50.0f;
        m_cameraHeight = std::max(std::min(g_maxCameraHeight, m_cameraHeight), g_minCameraHeight);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    m_camPos = XMVectorSet(m_orbitRadius * cosf(m_orbitAngle), m_cameraHeight, m_orbitRadius * sinf(m_orbitAngle), 0.0f);
    m_view = XMMatrixLookAtLH(m_camPos, g_XMZero, g_worldUp);

    m_particleWorld.Update(m_isPaused, m_view, m_proj, m_camPos);

    cbScene sceneConstants = {};
    XMStoreFloat3(&sceneConstants.LightDir, XMVector3Normalize(g_lightDirection));
    XMStoreFloat4(&sceneConstants.Color, g_lightColor);
    sceneConstants.CameraPos = m_camPos;
    m_sceneCBAddr = m_graphicsMemory->AllocateConstant(sceneConstants).GpuAddress();

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

    ID3D12DescriptorHeap* pHeaps[] = { m_srvPile->Heap() };
    commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    {
        ScopedPixEvent render(commandList, PIX_COLOR_DEFAULT, L"Render");

        // Draw the scene geometry.
        {
            ScopedPixEvent scene(commandList, PIX_COLOR_DEFAULT, L"Scene");
            for (auto& obj : m_scene)
            {
                obj.Model->DrawOpaque(commandList, obj.SetupFunc);
            }
        }

        // Restore the viewport, as the sky changed the min depth bounds.
        auto vp = m_deviceResources->GetScreenViewport();
        commandList->RSSetViewports(1, &vp);

        // Render the particle systems.
        m_particleWorld.Render(commandList, m_deviceResources->GetRenderTargetView(), m_deviceResources->GetDepthStencilView());

        // Draw the HUD
        {
            ScopedPixEvent hud(commandList, PIX_COLOR_DEFAULT, L"HUD");

            auto safe = DirectX::SimpleMath::Viewport::ComputeTitleSafeArea(UINT(m_displayWidth), UINT(m_displayHeight));

            wchar_t textBuffer[1024] = {};
            XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
            XMVECTOR textColor = ATG::Colors::Green;

            m_hudBatch->Begin(commandList);

            m_smallFont->DrawString(m_hudBatch.get(), g_sampleTitle, textPos, textColor);
            textPos.y += m_smallFont->GetLineSpacing();

            m_smallFont->DrawString(m_hudBatch.get(), m_particleWorld.IsDeferred() ? L"Deferred : On" : L"Deferred : Off", textPos, textColor);

            swprintf_s(textBuffer, _countof(textBuffer),
                L"[LThumb] Camera Rotation/Zoom \n\
                [RThumb] Camera Height \n\
                [A] Toggle Deferred Particles \n\
                [X] Toggle Simulation Pause");

            textPos.y = float(safe.bottom - m_smallFont->GetLineSpacing() * 4);
            DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), textBuffer, textPos, textColor);

            m_hudBatch->End();
        }
    }

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
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
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_commonStates = std::make_unique<DirectX::CommonStates>(device);

    m_srvPile = std::make_unique<DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        128,
        SRV_Count);

    // Load models from disk.
    m_models.resize(_countof(s_modelPaths));
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        m_models[i] = Model::CreateFromSDKMESH(device, s_modelPaths[i]);
    }

    auto resourceUpload = ResourceUploadBatch(device);
    resourceUpload.Begin();

    // HUD
    auto backBufferRts = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    m_particleWorld.CreateDeviceDependentResources(m_deviceResources.get(), resourceUpload, *m_models[0].get());

    // Optimize meshes for rendering
    for (auto& model : m_models)
    {
        model->LoadStaticBuffers(device, resourceUpload);
    }

    // Upload textures to GPU.
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvPile->Heap());

    auto texOffsets = std::vector<size_t>(m_models.size());
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        size_t _;
        m_srvPile->AllocateRange(m_models[i]->textureNames.size(), texOffsets[i], _);

        m_models[i]->LoadTextures(*m_textureFactory, int(texOffsets[i]));
    }

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();

    //-------------------------------------------------------
    // Instantiate objects from basic scene definition.

    m_scene.resize(_countof(s_sceneDefinition));
    for (size_t i = 0; i < m_scene.size(); i++)
    {
        auto& def = s_sceneDefinition[i];
        assert(def.MeshIndex < int(m_models.size()));

        m_scene[i].Model = m_models[size_t(def.MeshIndex)].get();
        XMStoreFloat4x4(&m_scene[i].World, XMMatrixScaling(def.ScaleFactor, def.ScaleFactor, def.ScaleFactor));
        m_scene[i].SetupFunc = std::bind(def.Callback, this, std::placeholders::_1, std::placeholders::_2, int(i), int(texOffsets[i]));
    }

    const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // Load the scene vertex and pixel shaders
    auto sceneVS = DX::ReadData(L"SceneVS.cso");
    auto scenePS = DX::ReadData(L"ScenePS.cso");
    auto skyPS = DX::ReadData(L"SkyPS.cso");

    // Strip the root signature from one of the shaders (they all leverage the same root signature.)
    DX::ThrowIfFailed(device->CreateRootSignature(
        0,
        sceneVS.data(),
        sceneVS.size(),
        IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

    // create the scene PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
    descPso.InputLayout.pInputElementDescs = inputLayout;
    descPso.InputLayout.NumElements = ARRAYSIZE(inputLayout);
    descPso.pRootSignature = m_rootSignature.Get();
    descPso.VS.pShaderBytecode = sceneVS.data();
    descPso.VS.BytecodeLength = sceneVS.size();
    descPso.PS.pShaderBytecode = scenePS.data();
    descPso.PS.BytecodeLength = scenePS.size();
    descPso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    descPso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    descPso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    descPso.SampleMask = UINT_MAX;
    descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPso.NumRenderTargets = 1;
    descPso.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
    descPso.DSVFormat = m_deviceResources->GetDepthBufferFormat();
    descPso.SampleDesc.Count = 1;

    DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPso, IID_GRAPHICS_PPV_ARGS(m_scenePSO.ReleaseAndGetAddressOf())));

    // create the sky PSO
    descPso.PS.pShaderBytecode = skyPS.data();
    descPso.PS.BytecodeLength = skyPS.size();
    descPso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // enable depth, but don't write anything

    DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPso, IID_GRAPHICS_PPV_ARGS(m_skyPSO.ReleaseAndGetAddressOf())));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    const auto size = m_deviceResources->GetOutputSize();

    // Calculate display dimensions.
    m_displayWidth = size.right - size.left;
    m_displayHeight = size.bottom - size.top;

    // Initialize the view.
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, m_deviceResources->GetScreenViewport().Width / static_cast<float>(m_deviceResources->GetScreenViewport().Height), 0.1f, 1000.0f);

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    m_particleWorld.CreateWindowSizeDependendentResources(m_deviceResources.get());

    // Begin uploading texture resources
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_Font),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1080) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_CtrlFont),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_CtrlFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }
}

void Sample::SetupPipelineCommon(ID3D12GraphicsCommandList* commandList, const ModelMeshPart& part, int instIndex, int texOffset, FXMMATRIX view)
{
    auto viewProj = XMMatrixMultiply(view, m_proj);

    XMMATRIX world = XMLoadFloat4x4(&m_scene[size_t(instIndex)].World);

    cbModel perModel = {};
    XMStoreFloat4x4(&perModel.World, XMMatrixTranspose(world));
    XMStoreFloat4x4(&perModel.WorldRot, XMMatrixIdentity());
    XMStoreFloat4x4(&perModel.WorldViewProj, XMMatrixTranspose(XMMatrixMultiply(world, viewProj)));
    auto modelCBMem = m_graphicsMemory->AllocateConstant(perModel);

    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(RootParamCBModel, modelCBMem.GpuAddress());
    commandList->SetGraphicsRootConstantBufferView(RootParamCBScene, m_sceneCBAddr);
    commandList->SetGraphicsRootConstantBufferView(RootParamCBGlowLights, m_particleWorld.GetGlowLightConstant());

    auto texHandle = m_scene[size_t(instIndex)].Model->GetGpuTextureHandleForMaterialIndex(part.materialIndex, m_srvPile->Heap(), m_srvPile->Increment(), size_t(texOffset));
    commandList->SetGraphicsRootDescriptorTable(RootParamSRV, texHandle);
}

void Sample::SetupPipelineScene(ID3D12GraphicsCommandList* commandList, const ModelMeshPart& part, int instIndex, int texOffset)
{
    SetupPipelineCommon(commandList, part, instIndex, texOffset, m_view);

    commandList->SetPipelineState(m_scenePSO.Get());
}

void Sample::SetupPipelineSky(ID3D12GraphicsCommandList* commandList, const ModelMeshPart& part, int instIndex, int texOffset)
{
    auto skyView = XMLoadFloat4x4(&m_view);
    skyView.r[3] = XMVectorSelect(skyView.r[3], g_XMZero, g_XMSelect1110);

    SetupPipelineCommon(commandList, part, instIndex, texOffset, skyView);

    D3D12_VIEWPORT vpSky = m_deviceResources->GetScreenViewport();
    vpSky.MinDepth = 0.999f;
    commandList->RSSetViewports(1, &vpSky);
    commandList->SetPipelineState(m_skyPSO.Get());
}
#pragma endregion
