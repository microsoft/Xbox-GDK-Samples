//--------------------------------------------------------------------------------------
// ComputeParticles.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ComputeParticles.h"

#include "ATGColors.h"

extern void ExitSample();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* g_sampleTitle = L"Compute Particles";

    static const float g_minOrbitRadius = 5.0f;
    static const float g_maxOrbitRadius = 60.0f;
    static const float g_maxCameraHeight = 25.0f;
    static const float g_minCameraHeight = 0.2f;

    static const XMVECTOR g_lightDirection = XMVectorSet(0.15f, -2.0f, 3.0f, 0.0f);

    // Assest paths.
    const wchar_t* const s_modelPaths[] =
    {
        L"particle_sample.sdkmesh",
        L"Gargoyle.sdkmesh",
    };
}

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_orbitRadius(40.0f)
    , m_orbitAngle(XM_PI * 1.75f)
    , m_cameraHeight(6.0f)
    , m_frameCounter(0)
    , m_frameTime(0)
    , m_frameRate(0)
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

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        // Our emitter height is adjusted by the right stick y axis when the right bumper is depressed.
        if (!pad.IsRightShoulderPressed())
        {
            // Our camera height is adjusted by the right-stick y axis, and is clamped to an allowed range.
            m_cameraHeight += pad.thumbSticks.rightY * elapsedTime * 10.0f;
            m_cameraHeight = std::max(std::min(g_maxCameraHeight, m_cameraHeight), g_minCameraHeight);

            // Our orbit angle is adjusted by left-stick x-axis.
            m_orbitAngle += pad.thumbSticks.leftX * elapsedTime * 5.0f;

            // Our orbit radius is adjusted by left stick y-axis input, and is clamped to an allowed range
            m_orbitRadius -= pad.thumbSticks.leftY * elapsedTime * 25.0f;
            m_orbitRadius = std::max(std::min(g_maxOrbitRadius, m_orbitRadius), g_minOrbitRadius);
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Update the camera transform.
    auto lookFrom = XMVectorSet(m_orbitRadius * cosf(m_orbitAngle), m_cameraHeight, m_orbitRadius * sinf(m_orbitAngle), 0.0f);
    auto view = XMMatrixLookAtLH(lookFrom, XMVectorSet(0, 10, 0, 0), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    m_particles.Update(elapsedTime, pad, m_gamePadButtons, view, m_proj);

    // Update the frame-rate counters.
    if (m_frameCounter < 20)
    {
        m_frameTime += elapsedTime;
        ++m_frameCounter;
    }
    else
    {
        m_frameRate = 1.0f / (m_frameTime / 20.0f);
        m_frameTime = 0;
        m_frameCounter = 0;
    }

    // Set up gargoyle transform info
    m_scene[1].World =
        Matrix::CreateFromQuaternion(m_particles.GetEmitterRotation()) *
        Matrix::CreateFromAxisAngle(Vector3(0, 1, 0), -XM_PIDIV2) *
        Matrix::CreateTranslation(m_particles.GetEmitterPosition());

    for (auto& obj : m_scene)
    {
        Model::UpdateEffectMatrices(obj.Effects, XMLoadFloat4x4(&obj.World), view, m_proj);
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
    auto commandList = m_deviceResources->GetCommandList();

    ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    
    // Move onto scene rendering.
    Clear();

    {
        ScopedPixEvent render(commandList, PIX_COLOR_DEFAULT, L"Render");

        // Render the scene geometry
        {
            ScopedPixEvent scene(commandList, PIX_COLOR_DEFAULT, L"Scene");
            for (auto& obj : m_scene)
            {
                obj.Model->DrawOpaque(commandList, obj.Effects.begin());
            }
        }

        // Render the particle system
        m_particles.Render(commandList);

        // Render the HUD elements
        {
            ScopedPixEvent hud(commandList, PIX_COLOR_DEFAULT, L"HUD");

            auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(m_displayWidth), UINT(m_displayHeight));

            wchar_t textBuffer[1024] = {};
            XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
            XMVECTOR textColor = ATG::Colors::Green;

            m_hudBatch->Begin(commandList);

            m_smallFont->DrawString(m_hudBatch.get(), g_sampleTitle, textPos, textColor);
            textPos.y += m_smallFont->GetLineSpacing();

            swprintf_s(textBuffer, _countof(textBuffer),
                L"Framerate: %.2f \nParticle Count: %d \nBounciness: %.2f \nParticle Update Speed: %.2f",
                m_frameRate,
                m_particles.GetParticleCount(),
                m_particles.GetParticleBounciness(),
                m_particles.GetParticleUpdateSpeed());
            m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);

            swprintf_s(textBuffer, _countof(textBuffer),
                L"[LThumb] Camera rotation/zoom \n\
                [RThumb] Camera height \n\
                [LT] Decrease Particle Bounciness \n\
                [RT] Increase Particle Bounciness \n\
                [LThumb] + [RB] Emitter rotation/zoom \n\
                [RThumb] + [RB] Emitter height \n\
                [LT] + [RB] Decrease Particle Update Speed \n\
                [RT] + [RB] Increase Particle Update Speed \n\
                [DPad] Up/Down - Increase/Decrease Particle Count \n\
                [A] Toggle Particle Render \n\
                [B] Toggle Particle Simulation");

            textPos.y = float(safe.bottom - m_smallFont->GetLineSpacing() * 11);
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

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

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

    m_particles.Initialize(m_deviceResources.get(), resourceUpload);
    m_particles.GenerateGeometry(*m_models[0]);

    // HUD
    auto backBufferRts = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();

    // Instantiate objects from basic scene definition.
    auto effectFactory = EffectFactory(m_srvPile->Heap(), m_commonStates->Heap());

    auto objectRTState = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto objectPSD = EffectPipelineStateDescription(
        nullptr,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullCounterClockwise,
        objectRTState,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

    m_scene.resize(_countof(s_modelPaths));
    for (size_t i = 0; i < m_scene.size(); i++)
    {
        auto& model = *m_models[i];

        m_scene[i].Model = &model;
        m_scene[i].Effects = model.CreateEffects(effectFactory, objectPSD, objectPSD, int(texOffsets[i]));
        XMStoreFloat4x4(&m_scene[i].World, XMMatrixIdentity());

        std::for_each(m_scene[i].Effects.begin(), m_scene[i].Effects.end(), [&](std::shared_ptr<IEffect>& e)
        {
            auto effect = reinterpret_cast<NormalMapEffect*>(e.get());
            effect->EnableDefaultLighting();
            effect->SetLightEnabled(0, true);
            effect->SetLightDiffuseColor(0, XMVectorSet(1, 1, 1, 1));
            effect->SetLightDirection(0, XMVector3NormalizeEst(g_lightDirection));
        });
    }
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
#pragma endregion
