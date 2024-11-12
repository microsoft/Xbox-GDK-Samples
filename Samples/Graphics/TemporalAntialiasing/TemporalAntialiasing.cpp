//--------------------------------------------------------------------------------------
// TemporalAntialiasing.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace
{
    // Fly-Camera.
    constexpr Vector3       CAMERA_START_POS = Vector3(-20.f, 20.f, -20.f);
    constexpr float         CAMERA_NEAR = 0.1f;  
    constexpr float         CAMERA_FAR = 1000.0f;

    // Zoomed-in area center, in UV space. Right now, at the center of the screen.
    constexpr Vector2 ZoomAreaCenterUVSpace = Vector2(0.5f, 0.5f);

    // Extents of the zoom area. The smaller these are, the more zoomed in the region will be.
    constexpr Vector2 ZoomAreaExtentUVSpace = Vector2(0.05f, 0.05f);

    // Number of GBuffer targets and formats.
    constexpr uint32_t GPASS_RT_COUNT = 3;
    constexpr DXGI_FORMAT gbufferAlbedoFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr DXGI_FORMAT gbufferNormalsFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    constexpr DXGI_FORMAT gbufferMotionFormat = DXGI_FORMAT_R16G16_FLOAT;

    // RTV clear color.
    constexpr float RTVClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // For sharpening pass.
    constexpr float SharpenFactor = 2.0f;

    // For scaling the jitter amount (Jitter will be in NDC).
    // Increasing this makes the borders antialias better, but the jitter will be more noticeable too.
    constexpr float JITTER_SCALE_DEFAULT = 1.0f;
    
    // following work of Vaidyanathan et all: https://software.intel.com/content/www/us/en/develop/articles/coarse-pixel-shading-with-temporal-supersampling.html
    static const float Halton23_16[16][2] =
    {
        { 0.0000f, 0.000000f }, { 0.5000f, 0.333333f }, { 0.2500f, 0.666667f }, { 0.7500f, 0.111111f },
        { 0.1250f, 0.444444f }, { 0.6250f, 0.777778f }, { 0.3750f, 0.222222f }, { 0.8750f, 0.555556f },
        { 0.0625f, 0.888889f }, { 0.5625f, 0.037037f }, { 0.3125f, 0.370370f }, { 0.8125f, 0.703704f },
        { 0.1875f, 0.148148f }, { 0.6875f, 0.481481f }, { 0.4375f, 0.814815f }, { 0.9375f, 0.259259f }
    };

    // https://github.com/GameTechDev/TAA/blob/main/MiniEngine/Core/TemporalEffects.cpp
    static const float BlueNoise_16[16][2] =
    {
        { 1.50000f, 0.59375f }, { 1.21875f, 1.37500f }, { 1.6875f, 1.90625f }, { 0.37500f, 0.84375f },
        { 1.12500f, 1.87500f }, { 0.71875f, 1.65625f }, { 1.9375f, 0.71875f }, { 0.65625f, 0.12500f },
        { 0.90625f, 0.93750f }, { 1.65625f, 1.43750f }, { 0.5000f, 1.28125f }, { 0.21875f, 0.06250f },
        { 1.843750, 0.312500 }, { 1.09375f, 0.56250f }, { 0.0625f, 1.21875f }, { 0.28125f, 1.65625f },
    };

    // Mini-Engine Halton sequence.
    static const float Halton23_8[8][2] =
    {
        { 0.0f / 8.0f, 0.0f / 9.0f }, { 4.0f / 8.0f, 3.0f / 9.0f },
        { 2.0f / 8.0f, 6.0f / 9.0f }, { 6.0f / 8.0f, 1.0f / 9.0f },
        { 1.0f / 8.0f, 4.0f / 9.0f }, { 5.0f / 8.0f, 7.0f / 9.0f },
        { 3.0f / 8.0f, 2.0f / 9.0f }, { 7.0f / 8.0f, 5.0f / 9.0f }
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_useTAA(false),
    m_performSharpenPass(false),
    m_hideZoomRegion(false),
    m_dilationMode(DILATION_MODE_NONE),
    m_clipMode(CLIP_MODE_RGB_CLAMP),
    m_filterMode(FILTER_MODE_BILINEAR),
    m_samplingBias(0.0f),
    m_jitterScale(JITTER_SCALE_DEFAULT),
    m_jitterSequence(JITTER_QR_HALTON23_8),
    m_currentHistoryBufferIndex(0u),
    m_currentJitter(0.5f),
    m_previousJitter(0.5f),
    m_JitterDeltaX(0.0f),
    m_JitterDeltaY(0.0f),
    m_sceneWorldMatrix()
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, 3u,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD );

    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    for (GameObject* go : m_gameobjects)
    {
        delete go;
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

    // Rotate the scene mesh (once) to correct its orientation.
    m_sceneWorldMatrix = Matrix::CreateRotationX(XM_PIDIV2);
    m_sceneNormalMatrix = m_sceneWorldMatrix;

    // GameObjects - initialize transform values and control points so they move in the scene.
    InitializeGameObjectsPositions();

    // Create the gpu timer
    auto device = m_deviceResources->GetD3DDevice();
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
}

void Sample::InitializeGameObjectsPositions()
{
    // Hexahedron
    m_gameobjects[0]->AddControlPoint(Vector3(-20.0f, 30.0f, 280.0f));
    m_gameobjects[0]->AddControlPoint(Vector3(-10.0f, 40.0f, 250.0f));
    m_gameobjects[0]->AddControlPoint(Vector3(0.0f, 20.0f, 200.0f));
    m_gameobjects[0]->AddControlPoint(Vector3(10.0f, 40.0f, 100.0f));
    m_gameobjects[0]->AddControlPoint(Vector3(20.0f, 30.0f, 000.0f));
    m_gameobjects[0]->SetInMotion(true);

    // Cactus
    m_gameobjects[1]->SetScale(Vector3(0.5f, 0.5f, 0.5f));
    m_gameobjects[1]->AddControlPoint(Vector3(150.0f, 0.0f, 45.0f));
    m_gameobjects[1]->AddControlPoint(Vector3(160.0f, 0.0f, 55.0f));
    m_gameobjects[1]->AddControlPoint(Vector3(170.0f, 0.0f, 45.0f));
    m_gameobjects[1]->AddControlPoint(Vector3(160.0f, 0.0f, 35.0f));
    m_gameobjects[1]->SetInMotion(true);

    // Dragon
    m_gameobjects[2]->SetScale(Vector3(2.5f, 2.5f, 2.5f));
    m_gameobjects[2]->AddControlPoint(Vector3(-150.0f, 30.0f, 40.0f));
    m_gameobjects[2]->AddControlPoint(Vector3(-150.0f, 30.0f, 40.0f));
    m_gameobjects[2]->AddControlPoint(Vector3(-150.0f, 30.0f, 0.0f));
    m_gameobjects[2]->AddControlPoint(Vector3(-150.0f, 30.0f, 0.0f));
    m_gameobjects[2]->SetInMotion(true);
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
        using ButtonState = GamePad::ButtonStateTracker;

        m_sceneCamera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == ButtonState::RELEASED)
        {
            m_useTAA = !m_useTAA;
        }

        // These configs should not be active if TAA is disabled.
        if (m_useTAA)
        {
            if (m_gamePadButtons.b == ButtonState::RELEASED)
            {
                m_dilationMode = static_cast<DilationMode>((static_cast<uint32_t>(m_dilationMode) + 1u) % DILATION_MODE_COUNT);
            }

            if (m_gamePadButtons.x == ButtonState::RELEASED)
            {
                m_clipMode = static_cast<ClipMode>((static_cast<uint32_t>(m_clipMode) + 1u) % CLIP_MODE_COUNT);
            }

            if (m_gamePadButtons.y == ButtonState::RELEASED)
            {
                m_jitterSequence = static_cast<JitterQRSequence>((static_cast<uint32_t>(m_jitterSequence) + 1u) % JITTER_QR_SEQUENCE_COUNT);
            }

            if (m_gamePadButtons.start == ButtonState::RELEASED)
            {
                m_performSharpenPass = !m_performSharpenPass;
            }

            if (m_gamePadButtons.dpadRight == ButtonState::RELEASED)
            {
                m_filterMode = static_cast<FilterMode>((m_filterMode + 1u) % FILTER_MODE_COUNT);
            }
            else if (m_gamePadButtons.dpadLeft == ButtonState::RELEASED)
            {
                m_filterMode = static_cast<FilterMode>((m_filterMode + (FILTER_MODE_COUNT - 1u)) % FILTER_MODE_COUNT);
            }

            if (m_gamePadButtons.dpadUp == ButtonState::RELEASED)
            {
                m_jitterScale = std::min(m_jitterScale + 0.2f, 1.0f);
            }
            else if (m_gamePadButtons.dpadDown == ButtonState::RELEASED)
            {
                m_jitterScale = std::max(m_jitterScale - 0.2f, 0.2f);
            }
        }

        if (m_gamePadButtons.leftShoulder == ButtonState::RELEASED)
        {
            m_hideZoomRegion = !m_hideZoomRegion;
        }

        if (m_gamePadButtons.rightShoulder == ButtonState::RELEASED)
        {
            for (GameObject* go : m_gameobjects)
            {
                go->SetInMotion(!go->InMotion());
            }
        }
    }
    else
    {
        m_gamePadButtons.Reset();

        m_sceneCamera.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());
    }

#ifdef _GAMING_DESKTOP
    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Enter))
    {
        m_useTAA = !m_useTAA;
    }

    // These configs should not be active if TAA is disabled.
    if (m_useTAA)
    {
        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Z))
        {
            m_dilationMode = static_cast<DilationMode>((static_cast<uint32_t>(m_dilationMode) + 1u) % DILATION_MODE_COUNT);
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::X))
        {
            m_clipMode = static_cast<ClipMode>((static_cast<uint32_t>(m_clipMode) + 1u) % CLIP_MODE_COUNT);
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Space))
        {
            m_performSharpenPass = !m_performSharpenPass;
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::C))
        {
            m_jitterSequence = static_cast<JitterQRSequence>((static_cast<uint32_t>(m_jitterSequence) + 1u) % JITTER_QR_SEQUENCE_COUNT);
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::R))
        {
            m_filterMode = static_cast<FilterMode>((m_filterMode + 1u) % FILTER_MODE_COUNT);
        }
        else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::F))
        {
            m_filterMode = static_cast<FilterMode>((m_filterMode + (FILTER_MODE_COUNT - 1u)) % FILTER_MODE_COUNT);
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::T))
        {
            m_jitterScale = std::min(m_jitterScale + 0.2f, 1.0f);
        }
        else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::G))
        {
            m_jitterScale = std::max(m_jitterScale - 0.2f, 0.2f);
        }
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::V))
    {
        m_hideZoomRegion = !m_hideZoomRegion;
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Tab))
    {
        for (GameObject* go : m_gameobjects)
        {
            go->SetInMotion(!go->InMotion());
        }
    }
#endif

    // Update the Jitter with the chosen sequence.
    if (m_jitterSequence == JITTER_QR_HALTON23_8)
    {
        m_JitterDeltaX = Halton23_8[m_frame % 8][0] - 0.5f;
        m_JitterDeltaY = Halton23_8[m_frame % 8][1] - 0.5f;
    }
    else if (m_jitterSequence == JITTER_QR_HALTON23_16)
    {
        m_JitterDeltaX = Halton23_16[m_frame % 16][0] - 0.5f;
        m_JitterDeltaY = Halton23_16[m_frame % 16][1] - 0.5f;
    }
    else // JITTER_QR_BLUENOISE_16
    {
        m_JitterDeltaX = (BlueNoise_16[m_frame % 16][0]) * 0.5f - 0.5f;
        m_JitterDeltaY = (BlueNoise_16[m_frame % 16][1]) * 0.5f - 0.5f;
    }

    // Jitter is in pixel space, centered around 0 going from -0.5f to 0.5f.
    // Needs to be in NDC space, so we scale it by (2.0f / resolution).
    m_JitterDeltaY *= -1.0f;
    m_currentJitter = Vector2(m_JitterDeltaX, m_JitterDeltaY) * Vector2(2.0f / static_cast<float>(m_displayWidth), 2.0f / static_cast<float>(m_displayHeight));
    m_currentJitter *= m_jitterScale;

    // GameObjects update
    for (GameObject* go : m_gameobjects)
    {
        go->Update(elapsedTime);
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

    m_gpuTimer->BeginFrame(commandList);

    // Add the Jitter to the projection matrix.
    // We add it to the z coordinate since, after perspective divide, this will be added in NDC space to the NDC x and y values.
    Matrix view = m_sceneCamera.GetView();
    Matrix proj = m_sceneCamera.GetProjection();
    if (m_useTAA)
    {
        proj._31 = m_currentJitter.x;
        proj._32 = m_currentJitter.y;
    }

    Vector4 resolutionVector = Vector4(static_cast<float>(m_displayWidth), static_cast<float>(m_displayHeight), 1.0f / m_displayWidth, 1.0f / m_displayHeight);

    ID3D12Resource* previousHistoryBuffer = m_historyBufferResource[1U - m_currentHistoryBufferIndex].Get();
    ID3D12Resource* currentHistoryBuffer = m_historyBufferResource[m_currentHistoryBufferIndex].Get();
    static_assert(_countof(m_historyBufferResource) == 2 && L"History buffer array should have two elements.");

    /// Geometry Buffer Pass.
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GeometryPass");
        m_gpuTimer->Start(commandList, TIMER_GEOMETRY_PASS);

        D3D12_RESOURCE_BARRIER gbufferBarriers[GPASS_RT_COUNT] = {};
        gbufferBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferAlbedoResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        gbufferBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferNormalsResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        gbufferBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferMotionResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(gbufferBarriers)), gbufferBarriers);

        commandList->ClearRenderTargetView(m_rtvHandleAlbedo, RTVClearColor, 0, nullptr);
        commandList->ClearRenderTargetView(m_rtvHandleNormals, RTVClearColor, 0, nullptr);
        commandList->ClearRenderTargetView(m_rtvHandleMotion, RTVClearColor, 0, nullptr);

        D3D12_CPU_DESCRIPTOR_HANDLE sceneRenderRTVs[GPASS_RT_COUNT] = { m_rtvHandleAlbedo, m_rtvHandleNormals, m_rtvHandleMotion };
        auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();
        commandList->OMSetRenderTargets(static_cast<uint32_t>(std::size(sceneRenderRTVs)), sceneRenderRTVs, false, &dsvDescriptor);

        // Set descriptor heap (with model uploaded textures).
        ID3D12DescriptorHeap* gbufferDescHeaps[] = { m_uploadedAssetsDescHeap->Heap() };
        commandList->SetDescriptorHeaps(1, gbufferDescHeaps);

        commandList->SetPipelineState(m_gbufferPSO.Get());
        commandList->SetGraphicsRootSignature(m_gbufferRS.Get());

        // Bind CBV with matrices.
        SceneCB gbufferCB = {};
        gbufferCB.viewProj = (view * proj).Transpose();
        gbufferCB.halfRes = Vector2(m_displayWidth / 2.0f, m_displayHeight / 2.0f);
        gbufferCB.jitterNDC = Vector4(m_currentJitter.x, m_currentJitter.y, m_previousJitter.x, m_previousJitter.y);
        gbufferCB.samplingBias = m_samplingBias;
        auto constantBuffer = m_graphicsMemory->AllocateConstant<SceneCB>(gbufferCB);
        commandList->SetGraphicsRootConstantBufferView(/*Root index*/0, constantBuffer.GpuAddress());

        PerObjectCB perObjectCB = {};
        perObjectCB.world = m_sceneWorldMatrix.Transpose();
        perObjectCB.normalTransform = m_sceneNormalMatrix.Transpose();
        perObjectCB.previousWorld = m_sceneWorldMatrix.Transpose();
        perObjectCB.diffuseColor = Vector3(1.0f, 1.0f, 1.0f);
        constantBuffer = m_graphicsMemory->AllocateConstant<PerObjectCB>(perObjectCB);
        commandList->SetGraphicsRootConstantBufferView(/*Root index*/1, constantBuffer.GpuAddress());

        // Drawing the scene geometry.
        for (size_t i = 0; i < m_sceneModel->meshes.size(); ++i)
        {
            auto& opaqueParts = m_sceneModel->meshes[i]->opaqueMeshParts;
            for (size_t j = 0; j < opaqueParts.size(); ++j)
            {
                auto material = m_sceneModel->materials[opaqueParts[j]->materialIndex];
                uint32_t textureIndex = (material.diffuseTextureIndex != -1) ? material.diffuseTextureIndex : 0u;
                commandList->SetGraphicsRootDescriptorTable(/*Root index*/2, m_uploadedAssetsDescHeap->GetGpuHandle(textureIndex));
                opaqueParts[j]->Draw(commandList);
            }
        }

        // Draw all other objects in the scene.
        for (GameObject* go : m_gameobjects)
        {
            perObjectCB.world = go->GetWorldMatrix().Transpose();
            perObjectCB.normalTransform = go->GetNormalMatrix().Transpose();
            perObjectCB.previousWorld = go->GetPreviousFrameWorldMatrix().Transpose();
            perObjectCB.diffuseColor = Vector3(1.0f, 1.0f, 0.0f);
            constantBuffer = m_graphicsMemory->AllocateConstant<PerObjectCB>(perObjectCB);
            commandList->SetGraphicsRootConstantBufferView(/*Root index*/1, constantBuffer.GpuAddress());
            go->Render(commandList);
        }

        m_gpuTimer->Stop(commandList, TIMER_GEOMETRY_PASS);
        m_gpuTimerMeasuresMS[TIMER_GEOMETRY_PASS] = m_gpuTimer->GetAverageMS(TIMER_GEOMETRY_PASS);
        PIXEndEvent(commandList);
    }

    /// Full screen stage.
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"LightingPass");
        m_gpuTimer->Start(commandList, TIMER_FULL_SCREEN_SHADING_PASS);

        D3D12_RESOURCE_BARRIER FSBarriers[5] = {};
        FSBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferAlbedoResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        FSBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferNormalsResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        FSBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferMotionResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        FSBarriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        FSBarriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(m_colorBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(FSBarriers)), FSBarriers);

        // Set fullscreen pass rendertarget.
        auto colorBufferRTVDescriptor = m_gBufferAndColorRTVDescHeap->GetCpuHandle(RTV_COLOR_BUFFER);
        commandList->OMSetRenderTargets(1, &colorBufferRTVDescriptor, true, nullptr);
        commandList->ClearRenderTargetView(colorBufferRTVDescriptor, RTVClearColor, 0, nullptr);

        // Set descriptor heap.
        ID3D12DescriptorHeap* FSPassDescHeaps[] = { m_shaderVisibleDescHeap->Heap() };
        commandList->SetDescriptorHeaps(1, FSPassDescHeaps);

        // Bind Root Signature.
        commandList->SetGraphicsRootSignature(m_fullscreenPassRS.Get());

        // Fullscreen pass CB.
        FullscreenPassCB fsPassCB = {};
        fsPassCB.invView = view.Invert().Transpose();
        fsPassCB.invProj = proj.Invert().Transpose();
        fsPassCB.prevFrameView = m_previousFrameView.Transpose();
        fsPassCB.prevFrameProj = m_previousFrameProj.Transpose();
        fsPassCB.resolution = resolutionVector;
        fsPassCB.cameraPosition = m_sceneCamera.GetPosition();
        auto constantBuffer = m_graphicsMemory->AllocateConstant<FullscreenPassCB>(fsPassCB);
        commandList->SetGraphicsRootConstantBufferView(/*Root index*/0, constantBuffer.GpuAddress());

        // descriptor table for the albedo, normals.
        commandList->SetGraphicsRootDescriptorTable(/*Root index*/1, m_shaderVisibleDescHeap->GetGpuHandle(SRV_GBUFFER_ALBEDO));

        // Descriptor table for depth.
        commandList->SetGraphicsRootDescriptorTable(/*Root index*/2, m_shaderVisibleDescHeap->GetGpuHandle(SRV_CURRENT_DEPTH));

        // Descriptor table for motion vector (UAV).
        commandList->SetGraphicsRootDescriptorTable(/*Root index*/3, m_shaderVisibleDescHeap->GetGpuHandle(UAV_FSQ_MOTION_VECTORS));

        // Bind PSO and draw.
        commandList->SetPipelineState(m_fullscreenPassPSO.Get());
        commandList->DrawInstanced(3, 1, 0, 0);

        m_gpuTimer->Stop(commandList, TIMER_FULL_SCREEN_SHADING_PASS);
        m_gpuTimerMeasuresMS[TIMER_FULL_SCREEN_SHADING_PASS] = m_gpuTimer->GetAverageMS(TIMER_FULL_SCREEN_SHADING_PASS);
        PIXEndEvent(commandList);
    }
  
    /// Temporal resolve.
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"TemporalResolve");
        m_gpuTimer->Start(commandList, TIMER_TAA_RESOLVE_PASS);

        // Transition both the srv for past frame and uav for current frame's accumulation.
        D3D12_RESOURCE_BARRIER barriers[3] = {};
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(previousHistoryBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_colorBufferResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferMotionResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

        ID3D12DescriptorHeap* descriptorHeaps[] = { m_shaderVisibleDescHeap->Heap() };
        commandList->SetDescriptorHeaps(1, descriptorHeaps);

        commandList->SetComputeRootSignature(m_temporalResolveRS.Get());

        // TAA resolve CB
        TemporalResolveCB taaCB = {};
        taaCB.resolution = resolutionVector;
        taaCB.flags |= (m_useTAA) ? TAA_FLAG_ENABLE_TAA : TAA_FLAG_NONE;
        taaCB.flags |= (1 << m_filterMode) << LogBase2(static_cast<uint32_t>(TAA_FLAG_USE_BILINEAR_FILTER));
        taaCB.flags |= (1 << m_dilationMode) << LogBase2(static_cast<uint32_t>(TAA_FLAG_USE_DILATION_NONE));
        taaCB.flags |= (1 << m_clipMode) << LogBase2(static_cast<uint32_t>(TAA_FLAG_USE_RGB_CLAMP));
        taaCB.varianceNeighbourCountRCP = 1.0f / VARIANCE_NEIGHBOUR_COUNT;
        auto constantBuffer = m_graphicsMemory->AllocateConstant<TemporalResolveCB>(taaCB);
        commandList->SetComputeRootConstantBufferView(/*Root index*/0, constantBuffer.GpuAddress());

        // SRVs for color, depth and previous depth
        commandList->SetComputeRootDescriptorTable(/*Root index*/1, m_shaderVisibleDescHeap->GetFirstGpuHandle());

        // SRV for previous accumulation buffer resource
        commandList->SetComputeRootDescriptorTable(/*Root index*/2, m_shaderVisibleDescHeap->GetGpuHandle(SRV_PREVIOUS_ACCUMULATION_0 + (1U - m_currentHistoryBufferIndex)));

        // SRV Motion vectors
        commandList->SetComputeRootDescriptorTable(/*Root index*/3, m_shaderVisibleDescHeap->GetGpuHandle(SRV_GBUFFER_MOTION_VECTORS));

        // UAV for current accumulation buffer resource
        commandList->SetComputeRootDescriptorTable(/*Root index*/4, m_shaderVisibleDescHeap->GetGpuHandle(UAV_CURRENT_ACCUMULATION_0 + m_currentHistoryBufferIndex));

        commandList->SetPipelineState(m_temporalResolvePSO.Get());

        uint32_t groupX = (m_displayWidth + TEMPORAL_RESOLVE_THREAD_X - 1) / TEMPORAL_RESOLVE_THREAD_X;
        uint32_t groupY = (m_displayHeight + TEMPORAL_RESOLVE_THREAD_Y - 1) / TEMPORAL_RESOLVE_THREAD_Y;
        commandList->Dispatch(groupX, groupY, 1u);

        m_gpuTimer->Stop(commandList, TIMER_TAA_RESOLVE_PASS);
        m_gpuTimerMeasuresMS[TIMER_TAA_RESOLVE_PASS] = m_gpuTimer->GetAverageMS(TIMER_TAA_RESOLVE_PASS);
        PIXEndEvent(commandList);

        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(previousHistoryBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ResourceBarrier(2, barriers);
    }

    /// Copy HistoryBuffer plus (optional) sharpening pass.
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Sharpen/Copy pass");
        m_gpuTimer->Start(commandList, TIMER_TAA_COPY_TO_BACKBUFFER_PASS);

        D3D12_RESOURCE_BARRIER barriers[1] = {};
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(currentHistoryBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, barriers);

        ID3D12DescriptorHeap* descHeaps[] = { m_shaderVisibleDescHeap->Heap() };
        commandList->SetDescriptorHeaps(1, descHeaps);

        commandList->SetComputeRootSignature(m_sharpenPassRS.Get());

        SharpenPassCB taaSharpenCB = {};
        taaSharpenCB.skipSharpen = !(m_performSharpenPass && m_useTAA);
        taaSharpenCB.SharpeningAmount = SharpenFactor;
        auto constantBuffer = m_graphicsMemory->AllocateConstant<SharpenPassCB>(taaSharpenCB);
        commandList->SetComputeRootConstantBufferView(/*Root index*/0, constantBuffer.GpuAddress());

        // Input texture.
        commandList->SetComputeRootDescriptorTable(/*Root index*/1, m_shaderVisibleDescHeap->GetGpuHandle(SRV_PREVIOUS_ACCUMULATION_0 + m_currentHistoryBufferIndex));

        // Output texture.
        commandList->SetComputeRootDescriptorTable(/*Root index*/2, m_shaderVisibleDescHeap->GetGpuHandle(UAV_INTERMEDIATE_RT));

        commandList->SetPipelineState(m_sharpenPassPSO.Get());

        uint32_t groupX = (m_displayWidth + TEMPORAL_RESOLVE_THREAD_X - 1) / TEMPORAL_RESOLVE_THREAD_X;
        uint32_t groupY = (m_displayHeight + TEMPORAL_RESOLVE_THREAD_Y - 1) / TEMPORAL_RESOLVE_THREAD_Y;
        commandList->Dispatch(groupX, groupY, 1u);

        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(currentHistoryBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, barriers);

        m_gpuTimer->Stop(commandList, TIMER_TAA_COPY_TO_BACKBUFFER_PASS);
        m_gpuTimerMeasuresMS[TIMER_TAA_COPY_TO_BACKBUFFER_PASS] = m_gpuTimer->GetAverageMS(TIMER_TAA_COPY_TO_BACKBUFFER_PASS);
        PIXEndEvent(commandList);
    }

    // (Final) copy of intermediate target to swapchain
    {
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateCopyResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(2, barriers);

        D3D12_TEXTURE_COPY_LOCATION copyLocationSrc = {};
        copyLocationSrc.pResource = m_intermediateCopyResource.Get();

        D3D12_TEXTURE_COPY_LOCATION copyLocationDst = {};
        copyLocationDst.pResource = m_deviceResources->GetRenderTarget();

        commandList->CopyTextureRegion(&copyLocationDst, 0, 0, 0, &copyLocationSrc, nullptr);

        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateCopyResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(2, barriers);
    }

    auto mainRTV = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &mainRTV, true, nullptr);

    // Magnifying glass and Zoom Region pass.
    if (!m_hideZoomRegion)
    {
        RenderMagnifyingGlassRegion(commandList, m_intermediateCopyResource.Get());
    }

    // Draw HUD.
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"HUD rendering");

    DrawHUD();

    PIXEndEvent(commandList);

    // Show the new frame.
    m_gpuTimer->EndFrame(commandList);
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();

    // Update UAV index for history buffer.
    m_currentHistoryBufferIndex = (m_currentHistoryBufferIndex + 1u) % std::size(m_historyBufferResource);

    // Set the previous viewProj matrix
    m_previousFrameView = view;
    m_previousFrameProj = proj;

    // Set previous Jitter
    m_previousJitter = m_currentJitter;
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
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::RenderMagnifyingGlassRegion(ID3D12GraphicsCommandList * commandList, ID3D12Resource* zoomTarget)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"MagnifyingGlassPass");

    // Extent of the screen (in NDC) where the magnifying glass will be rendered.
    // NDC goes from -1 to 1, so we will be rendering in a portion of the upper right quadrant of the screen.
    auto mgTopLeft = Vector2(0.2f, 0.2f);
    auto mgBottomRight = Vector2(0.9f, 0.9f);

    // UV min and max, representing the portion of the texture we will zoom into.
    auto minUV = ZoomAreaCenterUVSpace - 0.5f * ZoomAreaExtentUVSpace;
    auto maxUV = ZoomAreaCenterUVSpace + 0.5f * ZoomAreaExtentUVSpace;

    D3D12_RESOURCE_BARRIER barriers[1];
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(zoomTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, barriers);

    ID3D12DescriptorHeap* descHeapArr[] = { m_shaderVisibleDescHeap->Heap() };
    commandList->SetDescriptorHeaps(1, descHeapArr);

    commandList->SetGraphicsRootSignature(m_magnifyingGlassRS.Get());

    MagnifyingGlassPassCB magCB = {};
    magCB.vertexMinCoordsNDC = mgTopLeft;
    magCB.vertexMaxCoordsNDC = mgBottomRight;
    magCB.texMinUV = minUV;
    magCB.texMaxUV = maxUV;
    auto constantBuffer = m_graphicsMemory->AllocateConstant<MagnifyingGlassPassCB>(magCB);
    commandList->SetGraphicsRootConstantBufferView(/*Root index*/0, constantBuffer.GpuAddress());

    commandList->SetGraphicsRootDescriptorTable(/*Root index*/1, m_shaderVisibleDescHeap->GetGpuHandle(SRV_INTERMEDIATE_RT));

    commandList->SetPipelineState(m_magnifyingGlassPSO.Get());
    commandList->DrawInstanced(6, 1, 0, 0);

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(zoomTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, barriers);

    // Draw the region border
    commandList->SetGraphicsRootSignature(m_ZoomRegionRS.Get());

    ZoomPassCB zoomCB = {};
    zoomCB.minUV = minUV;
    zoomCB.maxUV = maxUV;
    zoomCB.borderColor = Vector3(1.0f, 0.0f, 0.0f);
    zoomCB.width = static_cast<float>(m_displayWidth);
    zoomCB.height = static_cast<float>(m_displayHeight);
    zoomCB.margin = 1.5f;
    constantBuffer = m_graphicsMemory->AllocateConstant<ZoomPassCB>(zoomCB);
    commandList->SetGraphicsRootConstantBufferView(/*Root index*/0, constantBuffer.GpuAddress());

    commandList->SetPipelineState(m_ZoomRegionPSO.Get());
    commandList->DrawInstanced(6, 1, 0, 0);

    PIXEndEvent(commandList);
}

void Sample::DrawHUD()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render (UI Pass)");
    m_hudBatch->Begin(commandList);

    ID3D12DescriptorHeap* heaps[] = { m_HUDDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Text is always rendered at 1080p
    RECT size = { 0, 0, 1920, 1080 };
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(uint32_t(size.right), uint32_t(size.bottom));

    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    const XMVECTOR textColor1 = DirectX::Colors::Orange;
    const XMVECTOR textColor2 = DirectX::Colors::LightYellow;
    const XMVECTOR textColorDisabled = DirectX::Colors::Gray;

    wchar_t buffer[100];
    {
        swprintf_s(buffer, std::size(buffer), L"Temporal Antialiasing");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor2);
        textPos.y += m_smallFont->GetLineSpacing();

        // Current state section
        swprintf_s(buffer, std::size(buffer), L"Resolution: %i x %i", static_cast<uint32_t>(m_displayWidth), static_cast<uint32_t>(m_displayHeight));
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor2);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Use TAA: %ls", (m_useTAA) ? L"True" : L"False");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        uint32_t const filterModeIndex = static_cast<uint32_t>(m_filterMode);
        swprintf_s(buffer, std::size(buffer), L"Filter mode: %ls", s_filterModeNames[filterModeIndex]);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        uint32_t const dilationModeIndex = static_cast<uint32_t>(m_dilationMode);
        swprintf_s(buffer, std::size(buffer), L"Dilation mode: %ls", s_dilationModeNames[dilationModeIndex]);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Sharpen pass: %ls", (m_performSharpenPass) ? L"Enabled" : L"Disabled");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Jitter scale: %.2f", m_jitterScale);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        uint32_t const jitterSequenceIndex = static_cast<uint32_t>(m_jitterSequence);
        swprintf_s(buffer, std::size(buffer), L"Jitter sequence: %ls", s_jitterQRSequenceName[jitterSequenceIndex]);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        uint32_t const clipModeIndex = static_cast<uint32_t>(m_clipMode);
        swprintf_s(buffer, std::size(buffer), L"Clip mode: %ls", s_clipModeNames[clipModeIndex]);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Zoom region: %ls", (m_hideZoomRegion) ? L"Hidden" : L"Visible");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        // Timer section
        swprintf_s(buffer, std::size(buffer), L"TAA Resolve Pass: %.2f ms", (m_useTAA) ? m_gpuTimerMeasuresMS[TIMER_TAA_RESOLVE_PASS] : 0.0f);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, (m_useTAA) ? textColor2 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();
        
        swprintf_s(buffer, std::size(buffer), L"Backbuffer copy time: %.2f ms", (m_useTAA) ? m_gpuTimerMeasuresMS[TIMER_TAA_COPY_TO_BACKBUFFER_PASS] : 0.0f);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, (m_useTAA) ? textColor2 : textColorDisabled);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        // Jump to bottom of screen
        textPos.y = size.bottom - (12.0f * m_smallFont->GetLineSpacing());

        // Instructions
#ifdef _GAMING_DESKTOP
        swprintf_s(buffer, std::size(buffer), L"[Enter] Toggle TAA.");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Z] Switch dilation mode.");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[X] Toggle color clamping technique.");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[C] Change jitter sequence.");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[R]/[F] - Switch filter mode.");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Space] Toggle sharpen pass.");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[T]/[G] - Modify jitter scale.");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Tab] Toggle object movement.");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[V] Hide/Show zoom region.");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Esc] Exit Sample");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);

#else // _GAMING_XBOX 
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[A] Toggle TAA.", textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[B] Switch dilation mode.", textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[X] Toggle color clamping technique.", textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[Y] Change jitter sequence.", textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[DPad] Left/Right - Switch filter mode.", textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[DPad] Up/Down - Modify jitter scale.", textPos, (m_useTAA) ? textColor1 : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[RB] Toggle object movement.", textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[LB] Hide/Show zoom region.", textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[View] Exit Sample", textPos, textColor1);
#endif
    }

    m_hudBatch->End();
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
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_uploadedAssetsDescHeap = std::make_unique<DirectX::DescriptorHeap>(device, 16u);
    m_uploadedAssetsDescHeap->Heap()->SetName(L"UploadedAssetsDescHeap");

    // Upload resources to video memory
    ResourceUploadBatch upload(device);
    upload.Begin();

    m_texFactory = std::make_unique<EffectTextureFactory>(device, upload, m_uploadedAssetsDescHeap->Heap());

    // Load the model(s) and its textures to memory.
    CreateModelFromPath(device, upload, m_texFactory, L"FPSRoom.sdkmesh", m_sceneModel);

    // Create objects to populate and move through the scene.
    m_gameobjects.push_back(new GameObject(device));
    m_gameobjects.push_back(new GameObject(device, upload, L"cactus.sdkmesh"));
    m_gameobjects.push_back(new GameObject(device, upload, L"Dragon.sdkmesh"));

    // UI (HUD)
    {
        m_HUDDescriptorHeap = std::make_unique<DirectX::DescriptorHeap>(device, 2);

        auto backBufferRTS = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRTS, &CommonStates::AlphaBlend);
        m_hudBatch = std::make_unique<SpriteBatch>(device, upload, spritePSD);

        wchar_t strFilePath[MAX_PATH] = {};
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_smallFont = std::make_unique<SpriteFont>(device, upload,
            strFilePath,
            m_HUDDescriptorHeap->GetCpuHandle(0),
            m_HUDDescriptorHeap->GetGpuHandle(0));

        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(device, upload, strFilePath,
            m_HUDDescriptorHeap->GetCpuHandle(1),
            m_HUDDescriptorHeap->GetGpuHandle(1));
    }

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    D3D12_INPUT_ELEMENT_DESC gElemDescPosition = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescNormal = { "NORMALS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescTexcoord0 = { "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gInputElemArr[] = { gElemDescPosition , gElemDescNormal, gElemDescTexcoord0 };

    // Geometry pass PSO and Root Signature.
    {
        // Load shader BLOB.
        auto const gShaderBlobVS = DX::ReadData(L"GeometryPassVS.cso");
        auto const gShaderBlobPS = DX::ReadData(L"GeometryPassPS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(0, gShaderBlobVS.data(), gShaderBlobVS.size(),
            IID_GRAPHICS_PPV_ARGS(m_gbufferRS.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
        PSODesc.VS = { gShaderBlobVS.data(), gShaderBlobVS.size() };
        PSODesc.PS = { gShaderBlobPS.data(), gShaderBlobPS.size() };
        PSODesc.pRootSignature = m_gbufferRS.Get();
        PSODesc.InputLayout = { gInputElemArr, static_cast<uint32_t>(std::size(gInputElemArr)) };
        PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        PSODesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        PSODesc.SampleMask = UINT32_MAX;
        PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        PSODesc.NumRenderTargets = GPASS_RT_COUNT;
        PSODesc.RTVFormats[0] = gbufferAlbedoFormat;
        PSODesc.RTVFormats[1] = gbufferNormalsFormat;
        PSODesc.RTVFormats[2] = gbufferMotionFormat;
        PSODesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&PSODesc,
            IID_GRAPHICS_PPV_ARGS(m_gbufferPSO.ReleaseAndGetAddressOf())));
    }

    // Fullscreen pass PSO and Root Signature.
    {
        auto vertexShaderBlob = DX::ReadData(L"FullscreenColorPassVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"FullscreenColorPassPS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(m_fullscreenPassRS.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_fullscreenPassRS.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_fullscreenPassPSO.ReleaseAndGetAddressOf())));
        m_fullscreenPassPSO->SetName(L"FullscreenPassPSO");
    }

    // Temporal Resolve PSO and Root Signature
    {
        // Load shader BLOB.
        auto const computeShaderBlob = DX::ReadData(L"TemporalResolve.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(m_temporalResolveRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC computePSODesc = {};
        computePSODesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        computePSODesc.pRootSignature = m_temporalResolveRS.Get();
        DX::ThrowIfFailed(device->CreateComputePipelineState(&computePSODesc,
            IID_GRAPHICS_PPV_ARGS(m_temporalResolvePSO.ReleaseAndGetAddressOf())));
        m_temporalResolvePSO->SetName(L"TemporalResolvePSO");
    }

    // Sharpen pass PSO and Root Signature.
    {
        // Load shader BLOB.
        auto const computeShaderBlob = DX::ReadData(L"SharpenCS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_sharpenPassRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC computePSODesc = {};
        computePSODesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        computePSODesc.pRootSignature = m_sharpenPassRS.Get();
        DX::ThrowIfFailed(device->CreateComputePipelineState(&computePSODesc,
            IID_GRAPHICS_PPV_ARGS(m_sharpenPassPSO.ReleaseAndGetAddressOf())));
        m_sharpenPassPSO->SetName(L"SharpenPassPSO");
    }

    // Zoom-Region and Magnifying-glass PSO and Root Signature.
    {
        // Zoom-Region pass
        auto vertexShaderBlob = DX::ReadData(L"ZoomRegionBorderVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"ZoomRegionBorderPS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(m_ZoomRegionRS.ReleaseAndGetAddressOf())));
        m_ZoomRegionRS->SetName(L"ZoomRegionRS");

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_ZoomRegionRS.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = false;
        psoDesc.DepthStencilState.StencilEnable = false;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_ZoomRegionPSO.ReleaseAndGetAddressOf())));
        m_ZoomRegionPSO->SetName(L"ZoomRegionPSO");

        // Magnifying glass pass
        vertexShaderBlob = DX::ReadData(L"MagnifyingGlassVS.cso");
        pixelShaderBlob = DX::ReadData(L"MagnifyingGlassPS.cso");

        DX::ThrowIfFailed(device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(m_magnifyingGlassRS.ReleaseAndGetAddressOf())));
        m_magnifyingGlassRS->SetName(L"MagnifyingGlassRS");

        psoDesc.pRootSignature = m_magnifyingGlassRS.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
       
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_magnifyingGlassPSO.ReleaseAndGetAddressOf())));
        m_magnifyingGlassPSO->SetName(L"MagnifyingGlassPSO");
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    RECT size = m_deviceResources->GetOutputSize();
    m_displayWidth = static_cast<uint64_t>(size.right - size.left);
    m_displayHeight = static_cast<uint64_t>(size.bottom - size.top);

    /// Scene camera.
    m_sceneCamera.SetWindow(static_cast<int32_t>(m_displayWidth), static_cast<int32_t>(m_displayHeight));
    m_sceneCamera.SetProjectionParameters(XM_PIDIV4, CAMERA_NEAR, CAMERA_FAR, true);
    m_sceneCamera.SetSensitivity(40.0f, 40.0f, 40.0f, 0.0f);
    m_sceneCamera.SetPosition(CAMERA_START_POS);
    m_sceneCamera.SetFlags(m_sceneCamera.c_FlagsDisableSensitivityControl);

    // Render all UI at 1080p so that it's easy to switch between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_hudBatch->SetViewport(viewportUI);

    // Temporal resolve, history buffer resource(s).
    {
        const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_deviceResources->GetBackBufferFormat(), m_displayWidth, m_displayHeight, 1, 1);
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        for (size_t i = 0; i < std::size(m_historyBufferResource); ++i)
        {
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_historyBufferResource[i].ReleaseAndGetAddressOf())));
        }
        m_historyBufferResource[0]->SetName(L"HistoryBufferResource 0");
        m_historyBufferResource[1]->SetName(L"HistoryBufferResource 1");
    }

    // Color buffer resource, rendertarget for the fullscreen pass.
    {
        const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_deviceResources->GetBackBufferFormat(), m_displayWidth, m_displayHeight, 1, 1);
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
#ifdef _GAMING_XBOX_SCARLETT
        resourceDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC;
        resourceDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
#endif

        CD3DX12_CLEAR_VALUE clearVal(m_deviceResources->GetBackBufferFormat(), RTVClearColor);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            &clearVal,
            IID_GRAPHICS_PPV_ARGS(m_colorBufferResource.ReleaseAndGetAddressOf())));
        m_colorBufferResource->SetName(L"ColorBufferResource");
    }

    // Intermediate copy target, needed for desktop scenario before swapchain copy.
    {
        const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC resDesc = m_deviceResources->GetRenderTarget()->GetDesc();
        resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        CD3DX12_CLEAR_VALUE clearVal(m_deviceResources->GetBackBufferFormat(), RTVClearColor);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            &clearVal,
            IID_GRAPHICS_PPV_ARGS(m_intermediateCopyResource.ReleaseAndGetAddressOf())));
        m_intermediateCopyResource->SetName(L"IntermediateCopyResource");
    }

    // GBuffer Resources
    {
        // For Albedo and Normals
        const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(gbufferAlbedoFormat, m_displayWidth, m_displayHeight, 1, 1);
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
#ifdef _GAMING_XBOX_SCARLETT
        resourceDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC;
        resourceDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
#endif

        CD3DX12_CLEAR_VALUE clearVal(gbufferAlbedoFormat, RTVClearColor);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &clearVal,
            IID_GRAPHICS_PPV_ARGS(m_gbufferAlbedoResource.ReleaseAndGetAddressOf())));
        m_gbufferAlbedoResource->SetName(L"GBufferAlbedoResource");

        resourceDesc.Format = gbufferNormalsFormat;
        clearVal = CD3DX12_CLEAR_VALUE(gbufferNormalsFormat, RTVClearColor);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &clearVal,
            IID_GRAPHICS_PPV_ARGS(m_gbufferNormalsResource.ReleaseAndGetAddressOf())));
        m_gbufferNormalsResource->SetName(L"GBufferNormalsResource");

        // Motion Vectors
        resourceDesc.Format = gbufferMotionFormat;
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        clearVal = CD3DX12_CLEAR_VALUE(gbufferMotionFormat, RTVClearColor);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            &clearVal,
            IID_GRAPHICS_PPV_ARGS(m_gbufferMotionResource.ReleaseAndGetAddressOf())));
        m_gbufferMotionResource->SetName(L"MotionVectorsResource");
    }

    // Descriptor Heap and views
    {
        // This RT heap is for the GBuffer plus the colorBuffer used in the fullscreen pass (GPASS_RT_COUNT + 1).
        m_gBufferAndColorRTVDescHeap = std::make_unique<DirectX::DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, GPASS_RT_COUNT + 1u);
        m_gBufferAndColorRTVDescHeap->Heap()->SetName(L"RenderTargetDescriptorHeap");

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        // Render target view for full screen shading pass.
        rtvDesc.Format = m_deviceResources->GetBackBufferFormat();
        device->CreateRenderTargetView(m_colorBufferResource.Get(), &rtvDesc, m_gBufferAndColorRTVDescHeap->GetCpuHandle(RTV_COLOR_BUFFER));

        // GBuffer RTVs.
        rtvDesc.Format = gbufferMotionFormat;
        m_rtvHandleMotion = m_gBufferAndColorRTVDescHeap->GetCpuHandle(RTV_GBUFFER_MOTION);
        device->CreateRenderTargetView(m_gbufferMotionResource.Get(), &rtvDesc, m_rtvHandleMotion);

        rtvDesc.Format = gbufferNormalsFormat;
        m_rtvHandleNormals = m_gBufferAndColorRTVDescHeap->GetCpuHandle(RTV_GBUFFER_NORMALS);
        device->CreateRenderTargetView(m_gbufferNormalsResource.Get(), &rtvDesc, m_rtvHandleNormals);

        rtvDesc.Format = gbufferAlbedoFormat;
        m_rtvHandleAlbedo = m_gBufferAndColorRTVDescHeap->GetCpuHandle(RTV_GBUFFER_ALBEDO);
        device->CreateRenderTargetView(m_gbufferAlbedoResource.Get(), &rtvDesc, m_rtvHandleAlbedo);

        // Shader visible descriptor heap.
        m_shaderVisibleDescHeap = std::make_unique<DirectX::DescriptorHeap>(device, SHADER_VISIBLE_DESC_HEAP_INDICES_COUNT);
        m_shaderVisibleDescHeap->Heap()->SetName(L"ShaderVisibleDescHeap");

        // SRV for color buffer (used in temporal Resolve pass).
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = m_deviceResources->GetBackBufferFormat();
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_colorBufferResource.Get(), &srvDesc, m_shaderVisibleDescHeap->GetCpuHandle(SRV_COLOR_BUFFER));

        // SRV for current depth (used in temporal Resolve).
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDepthDesc = {};
        srvDepthDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDepthDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDepthDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDepthDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_deviceResources->GetDepthStencil(), &srvDepthDesc, m_shaderVisibleDescHeap->GetCpuHandle(SRV_CURRENT_DEPTH));

        // SRVs (2) for m_historyBufferResource.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvAccumDesc = {};
        srvAccumDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvAccumDesc.Format = m_deviceResources->GetBackBufferFormat(),
        srvAccumDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvAccumDesc.Texture2D.MipLevels = 1;
        for (size_t i = 0; i < std::size(m_historyBufferResource); ++i)
        {
            device->CreateShaderResourceView(m_historyBufferResource[i].Get(), &srvAccumDesc, m_shaderVisibleDescHeap->GetCpuHandle(SRV_PREVIOUS_ACCUMULATION_0 + i));
        }

        // UAVs (2) for m_historyBufferResource.
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = m_deviceResources->GetBackBufferFormat(),
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        for (size_t i = 0; i < std::size(m_historyBufferResource); ++i)
        {
            device->CreateUnorderedAccessView(m_historyBufferResource[i].Get(), nullptr, &uavDesc, m_shaderVisibleDescHeap->GetCpuHandle(UAV_CURRENT_ACCUMULATION_0 + i));
        }

        // SRV for gbuffer Albedo (used in full screen shading pass).
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDescGBuffer = {};
        srvDescGBuffer.Format = gbufferAlbedoFormat;
        srvDescGBuffer.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDescGBuffer.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDescGBuffer.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_gbufferAlbedoResource.Get(), &srvDescGBuffer, m_shaderVisibleDescHeap->GetCpuHandle(SRV_GBUFFER_ALBEDO));

        // SRV for gbuffer Normals (used in full screen shading pass).
        srvDescGBuffer.Format = gbufferNormalsFormat;
        device->CreateShaderResourceView(m_gbufferNormalsResource.Get(), &srvDescGBuffer, m_shaderVisibleDescHeap->GetCpuHandle(SRV_GBUFFER_NORMALS));

        // SRV for motion vectors (used in temporal resolve pass).
        srvDescGBuffer.Format = gbufferMotionFormat;
        device->CreateShaderResourceView(m_gbufferMotionResource.Get(), &srvDescGBuffer, m_shaderVisibleDescHeap->GetCpuHandle(SRV_GBUFFER_MOTION_VECTORS));

        // UAV for motion vectors (used in full screen pass).
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescMotion = {};
        uavDescMotion.Format = gbufferMotionFormat;
        uavDescMotion.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateUnorderedAccessView(m_gbufferMotionResource.Get(), nullptr, &uavDescMotion, m_shaderVisibleDescHeap->GetCpuHandle(UAV_FSQ_MOTION_VECTORS));

        // UAV for intermediate resource used as copy dest of the history buffer (and potentially, sharpening pass). Needed due to desktop not accepting UAV for swapchain.
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavIntermediateDesc = {};
        uavIntermediateDesc.Format = m_deviceResources->GetBackBufferFormat();
        uavIntermediateDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateUnorderedAccessView(m_intermediateCopyResource.Get(), nullptr, &uavIntermediateDesc, m_shaderVisibleDescHeap->GetCpuHandle(UAV_INTERMEDIATE_RT));

        // SRV for intermediate resource used as copy dest of the history buffer (and potentially, sharpening pass).
        D3D12_SHADER_RESOURCE_VIEW_DESC srvIntermediateDesc = {};
        srvIntermediateDesc.Format = m_deviceResources->GetBackBufferFormat();
        srvIntermediateDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvIntermediateDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvIntermediateDesc.Texture2D.MipLevels = 1u;
        device->CreateShaderResourceView(m_intermediateCopyResource.Get(), &srvIntermediateDesc, m_shaderVisibleDescHeap->GetCpuHandle(SRV_INTERMEDIATE_RT));
    }
}

void Sample::OnDeviceLost()
{
    m_deviceResources.reset();              
    m_graphicsMemory.reset();               
    m_gpuTimer.reset();                     
    m_HUDDescriptorHeap.reset();            
    m_hudBatch.reset();                     
    m_smallFont.reset();                    
    m_ctrlFont.reset();
    m_sceneModel.reset();                   
    m_uploadedAssetsDescHeap.reset();
    m_shaderVisibleDescHeap.reset();
    m_gBufferAndColorRTVDescHeap.reset();                   
    m_texFactory.reset();                   
    m_gamePad.reset();                      
    m_keyboard.reset();                     
    m_mouse.reset();                        

    m_temporalResolvePSO.Reset();
    m_temporalResolveRS.Reset();         
    m_gbufferMotionResource.Reset();        
    m_gbufferAlbedoResource.Reset();        
    m_gbufferNormalsResource.Reset();       
    m_gbufferPSO.Reset();                   
    m_gbufferRS.Reset();                    
    m_magnifyingGlassPSO.Reset();           
    m_magnifyingGlassRS.Reset();            
    m_ZoomRegionPSO.Reset();                
    m_ZoomRegionRS.Reset();                 
    m_fullscreenPassPSO.Reset();            
    m_fullscreenPassRS.Reset();             
    m_colorBufferResource.Reset();          
    m_intermediateCopyResource.Reset();        
    for (uint32_t i = 0; i < 2; ++i)
    {
        m_historyBufferResource[i].Reset();
    }
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

void Sample::CreateModelFromPath(ID3D12Device* device, ResourceUploadBatch& upload, std::unique_ptr<EffectTextureFactory>& texFactory,
    std::wstring const& meshName, std::unique_ptr<Model>& outModel)
{
    wchar_t const* const folderPaths[2] = { L"Media\\Meshes\\FPSRoom", nullptr };

    wchar_t s_meshFilename[1024];
    wchar_t filepath[1024];
    _snwprintf_s(s_meshFilename, std::size(s_meshFilename), _TRUNCATE, meshName.c_str());

    DX::FindMediaFile(filepath, static_cast<int>(std::size(filepath)), s_meshFilename, folderPaths);

    // Store the media directory
    std::wstring pathTemp = filepath;
    std::wstring mediaDirectory = pathTemp.substr(0, pathTemp.find_last_of('\\'));

    auto modelBLOB = DX::ReadData(filepath);
    outModel = Model::CreateFromSDKMESH(device, modelBLOB.data(), modelBLOB.size());

    outModel->LoadStaticBuffers(device, upload);

    texFactory->SetDirectory(mediaDirectory.c_str());

    // Load the model's textures (for scene)
    texFactory->EnableForceSRGB(true);
    outModel->LoadTextures(*texFactory);
}

uint32_t Sample::LogBase2(unsigned long mask)
{
    unsigned long index;
    _BitScanReverse(&index, mask);
    return index;
}
