//--------------------------------------------------------------------------------------
// GeometricExpansion.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GeometricExpansion.h"

#include "ATGColors.h"
#include "FindMedia.h"

#include "Shared.h"

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

    enum DescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_Count
    };

    static const wchar_t* s_sampleTitle = L"Geometric Expansion";

    constexpr float c_minLifetime = 3.0f;
    constexpr float c_maxLifetime = 8.0f;

    constexpr float c_minSize = 0.1f;
    constexpr float c_maxSize = 1.0f;

    constexpr float c_spawnRate = 1000.0f;
    constexpr float c_springCoeff = 4.0f;
    constexpr float c_dragFactor = 1.0f;
    constexpr float c_initialSpeed = 3.0f;
}

Sample::Sample() noexcept(false)
    : m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
    , m_spawnRate(c_spawnRate)
    , m_springCoeff(c_springCoeff)
    , m_dragFactor(c_dragFactor)
    , m_initialSpeed(c_initialSpeed)
    , m_particleSystem(
        g_XMZero,
        m_spawnRate,
        m_springCoeff,
        m_dragFactor,
        m_initialSpeed,
        ValueRange(c_minLifetime, c_maxLifetime), 
        ValueRange(c_minSize, c_maxSize))
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());
    
    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        m_camera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        // Spawn Rate
        if (pad.buttons.rightShoulder)
        {
            m_spawnRate = std::min(m_spawnRate + 10.0f, 4000.0f);
            m_particleSystem.SetSpawnRate(m_spawnRate);
        }
        if (pad.triggers.right > 0.0f)
        {
            m_spawnRate = std::max(m_spawnRate - 10.0f, 0.0f);
            m_particleSystem.SetSpawnRate(m_spawnRate);
        }

        // Initial Particle Speed
        if (pad.buttons.leftShoulder)
        {
            m_initialSpeed = std::min(m_initialSpeed + 0.1f, 50.0f);
            m_particleSystem.SetInitialSpeed(m_initialSpeed);
        }
        if (pad.triggers.left > 0.0f)
        {
            m_initialSpeed = std::max(m_initialSpeed - 0.1f, 0.0f);
            m_particleSystem.SetInitialSpeed(m_initialSpeed);
        }

        // Spring Coefficient
        if (pad.buttons.x)
        {
            m_springCoeff = std::min(m_springCoeff + 0.1f, 20.0f);
            m_particleSystem.SetSpringCoefficent(m_springCoeff);
        }
        if (pad.buttons.y)
        {
            m_springCoeff = std::max(m_springCoeff - 0.1f, 0.0f);
            m_particleSystem.SetSpringCoefficent(m_springCoeff);
        }

        // Drag Factor
        if (pad.buttons.a)
        {
            m_dragFactor = std::min(m_dragFactor + 0.1f, 5.0f);
            m_particleSystem.SetDragFactor(m_dragFactor);
        }
        if (pad.buttons.b)
        {
            m_dragFactor = std::max(m_dragFactor - 0.1f, 0.0f);
            m_particleSystem.SetDragFactor(m_dragFactor);
        }
    }
    else
    {
        m_gamePadButtons.Reset();

        auto kb = m_keyboard->GetState();
        m_keyboardButtons.Update(kb);

        if (kb.Escape)
        {
            ExitSample();
        }

        if (kb.F5)
        {
            m_particleSystem.ReloadPipelineState(*m_deviceResources);
        }

        // Spawn Rate
        if (kb.OemPlus)
        {
            m_spawnRate = std::min(m_spawnRate + 10.0f, 4000.0f);
            m_particleSystem.SetSpawnRate(m_spawnRate);
        }
        if (kb.OemMinus)
        {
            m_spawnRate = std::max(m_spawnRate - 10.0f, 0.0f);
            m_particleSystem.SetSpawnRate(m_spawnRate);
        }

        // Initial Particle Speed
        if (kb.O)
        {
            m_initialSpeed = std::min(m_initialSpeed + 0.1f, 50.0f);
            m_particleSystem.SetInitialSpeed(m_initialSpeed);
        }
        if (kb.P)
        {
            m_initialSpeed = std::max(m_initialSpeed - 0.1f, 0.0f);
            m_particleSystem.SetInitialSpeed(m_initialSpeed);
        }

        // Spring Coefficient
        if (kb.K)
        {
            m_springCoeff = std::min(m_springCoeff + 0.1f, 20.0f);
            m_particleSystem.SetSpringCoefficent(m_springCoeff);
        }
        if (kb.L)
        {
            m_springCoeff = std::max(m_springCoeff - 0.1f, 0.0f);
            m_particleSystem.SetSpringCoefficent(m_springCoeff);
        }

        // Drag Factor
        if (kb.N)
        {
            m_dragFactor = std::min(m_dragFactor + 0.1f, 5.0f);
            m_particleSystem.SetDragFactor(m_dragFactor);
        }
        if (kb.M)
        {
            m_dragFactor = std::max(m_dragFactor - 0.1f, 0.0f);
            m_particleSystem.SetDragFactor(m_dragFactor);
        }

        m_camera.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());
    }

    m_particleSystem.Update((float)m_timer.GetElapsedSeconds(), m_camera.GetView(), m_camera.GetProjection());

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

    auto heaps = m_srvPile->Heap();
    commandList->SetDescriptorHeaps(1, &heaps);
    
    m_particleSystem.Draw(commandList, m_graphicsMemory.get());

    DrawHUD(commandList);

    PIXEndEvent(commandList);

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

void Sample::DrawHUD(ID3D12GraphicsCommandList* commandList)
{
    m_hudBatch->Begin(commandList);

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(m_displayWidth, m_displayHeight);

    wchar_t textBuffer[128] = {};
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = DirectX::Colors::DarkKhaki;

    m_smallFont->DrawString(m_hudBatch.get(), s_sampleTitle, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Spawn Rate %.2f", m_spawnRate);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Spring Coefficient %.2f", m_springCoeff);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Drag Factor %.2f", m_dragFactor);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Initial Speed %.2f", m_initialSpeed);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    textPos = XMFLOAT2(float(safe.left), float(safe.bottom));
#ifdef _GAMING_DESKTOP

    m_smallFont->DrawString(m_hudBatch.get(), L"[+-] Increase/Decrease Spawn Rate", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[OP] Increase/Decrease Initial Speed", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[KL] Increase/Decrease Spring Coefficient", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[NM] Increase/Decrease Drag Factor", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[ASWD] Pan Camera", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[LMouse] Rotate Camera (Hold)", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();
#else

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[RB][RT] Increase/Decrease Spawn Rate", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[LB][RT] Increase/Decrease Initial Speed", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[X][Y] Increase/Decrease Spring Coefficient", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[A][B] Increase/Decrease Drag Factor", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[LThumb][RThumb] [DPad] Control Camera", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[View] Exit Sample", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();
#endif

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

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Create heap
    m_srvPile = std::make_unique<DescriptorPile>(device,
        128,
        DescriptorHeapIndex::SRV_Count);

    m_particleSystem.CreateResources(*m_deviceResources);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // HUD
    const RenderTargetState backBufferRts(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

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

    resourceUpload.End(m_deviceResources->GetCommandQueue());
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();
    m_displayWidth = size.right - size.left;
    m_displayHeight = size.bottom - size.top;

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    m_camera.SetWindow(m_displayWidth, m_displayHeight);
    m_camera.SetProjectionParameters(XM_PIDIV4, 0.25f, 1000.0f, true);

    m_camera.SetRadius(50.0f);
    m_camera.SetRotation(Quaternion::CreateFromAxisAngle(Vector3::Right, XM_PIDIV4));
    m_camera.SetSensitivity(100.0f, 50.0f, 200.0f, 10.0f);

#ifdef _GAMING_XBOX_SCARLETT
    m_camera.SetRadiusRate(10.0f);
#endif
}

void Sample::OnDeviceLost()
{
    m_particleSystem.ReleaseDevice();
    m_hudBatch.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();
    m_srvPile.reset();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
