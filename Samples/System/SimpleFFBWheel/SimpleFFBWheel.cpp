//--------------------------------------------------------------------------------------
// SimpleFFBWheel.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleFFBWheel.h"

#include "ATGColors.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const GameInputForceFeedbackEnvelope c_envelope =
    {
        1000000,     // uint64_t attackDuration (us)
        1000000,     // sustainDuration
        1000000,     // releaseDuration
        1.0f,   // attackGain
        1.0f,   // sustainGain
        1.0f,   // releaseGain
        3,      // uint32_t playCount
        0       // uint64_t repeatDelay
    };

// NOTE: The behavior of the force feedback API changed in the October 2021 GDK
// If using October 2021 GDK or newer, the "angular X" value should be provided
// instead of the "linear X" value used previously.
    const GameInputForceFeedbackMagnitude c_magnitude =
    {
#if (_GXDK_EDITION >= 211000)
        0.0f,   // linear X
        0.0f,   // Y
        0.0f,   // Z
        1.0f,   // angular X
        0.0f,   // Y
        0.0f,   // Z
        0.0f    // normal
#else
        1.0f,   // linear X
        0.0f,   // Y
        0.0f,   // Z
        0.0f,   // angular X
        0.0f,   // Y
        0.0f,   // Z
        0.0f    // normal
#endif
    };

    //const GameInputForceFeedbackConstantParams c_constParams =
    //{
    //    c_envelope,
    //    c_magnitude
    //};

    //const GameInputForceFeedbackRampParams c_rampParams =
    //{
    //    c_envelope,
    //    { 0, 0, 0, 0, 0, 0, 0 }, // startMagnitude
    //    c_magnitude               // endMagnitude
    //};

    const GameInputForceFeedbackPeriodicParams c_periodicParams =
    {
        c_envelope,
        c_magnitude,
        2.0f,   // frequency (Hz). set low for human feel in testing
        0.0f,   // phase (degrees)
        0.0f    // bias
    };

    //const GameInputForceFeedbackConditionParams c_conditionParams =
    //{
    //    c_magnitude,
    //    1.0f,   // pos coefficient
    //    -1.0f,  // neg coefficient
    //    1.0f,   // max pos magnitude
    //    1.0f,   // max neg magnitude
    //    0,      // deadzone
    //    0       // bias
    //};
}

static void CALLBACK OnGameInputDeviceAddedRemoved(
    _In_ GameInputCallbackToken,
    _In_ void * context,
    _In_ IGameInputDevice * device,
    _In_ uint64_t,
    _In_ GameInputDeviceStatus currentStatus,
    _In_ GameInputDeviceStatus) noexcept
{
    auto sample = reinterpret_cast<Sample*>(context);

    if (currentStatus & GameInputDeviceConnected)
    {
        if (!sample->m_device)
        {
            const GameInputDeviceInfo* deviceInfo = device->GetDeviceInfo();

            if (deviceInfo->forceFeedbackMotorCount > 0)
            {
                //We only want devices with force feedback wheels
                sample->m_device = device;

                for (uint32_t i = 0; i < deviceInfo->forceFeedbackMotorCount; i++)
                {
                    const GameInputForceFeedbackMotorInfo motorInfo = deviceInfo->forceFeedbackMotorInfo[i];

                    if (motorInfo.isSineWaveEffectSupported)
                    {
                        GameInputForceFeedbackParams params;
                        params.kind = GameInputForceFeedbackSineWave;
                        params.data.sineWave = c_periodicParams;
                        device->CreateForceFeedbackEffect(i, &params, sample->m_feedbackEffect.GetAddressOf());
                    }
                }
            }
        }
    }
    else
    {
        if (sample->m_device.Get() == device)
        {
            sample->m_device.Reset();
        }
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_deviceToken(0),
    m_wheelValue(0.f),
    m_clutchValue(0.f),
    m_brakeValue(0.f),
    m_throttleValue(0.f),
    m_handbrakeValue(0.f),
    m_patternShifterGearValue(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    DX::ThrowIfFailed(GameInputCreate(&m_gameInput));
    DX::ThrowIfFailed(m_gameInput->RegisterDeviceCallback(nullptr,
        GameInputKindRacingWheel,
        GameInputDeviceConnected,
        GameInputBlockingEnumeration,
        this,
        OnGameInputDeviceAddedRemoved,
        &m_deviceToken));
}

Sample::~Sample()
{
    if (m_deviceToken)
    {
        if (m_gameInput)
        {
            (void)m_gameInput->UnregisterCallback(m_deviceToken, UINT64_MAX);
        }

        m_deviceToken = 0;
    }
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
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    if (m_device)
    {
        if (FAILED(m_gameInput->GetCurrentReading(GameInputKindRacingWheel | GameInputKindUiNavigation, m_device.Get(), &m_reading)))
        {
            m_buttonString.clear();
        }
        else
        {
            m_buttonString = L"Buttons pressed:  ";

            GameInputRacingWheelState racingWheelState;
            if (m_reading->GetRacingWheelState(&racingWheelState))
            {
                int exitComboPressed = 0;

                if (racingWheelState.buttons & GameInputRacingWheelDpadUp)
                {
                    m_buttonString += L"[DPad]Up ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelDpadDown)
                {
                    m_buttonString += L"[DPad]Down ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelDpadRight)
                {
                    m_buttonString += L"[DPad]Right ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelDpadLeft)
                {
                    m_buttonString += L"[DPad]Left ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelPreviousGear)
                {
                    m_buttonString += L"PrevGear ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelNextGear)
                {
                    m_buttonString += L"NextGear ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelMenu)
                {
                    m_buttonString += L"[Menu] ";
                    exitComboPressed += 1;
                }

                if (racingWheelState.buttons & GameInputRacingWheelView)
                {
                    m_buttonString += L"[View] ";
                    exitComboPressed += 1;
                }

                m_wheelValue = racingWheelState.wheel;
                m_throttleValue = racingWheelState.throttle;
                m_brakeValue = racingWheelState.brake;
                m_clutchValue = racingWheelState.clutch;
                m_handbrakeValue = racingWheelState.handbrake;
                m_patternShifterGearValue = racingWheelState.patternShifterGear;

                if (exitComboPressed == 2)
                    ExitSample();
            }

            GameInputUiNavigationState uiState;
            if (m_reading->GetUiNavigationState(&uiState))
            {
                if (uiState.buttons & GameInputUiNavigationAccept)
                {
                    m_buttonString += L"[A] ";

                    m_feedbackEffect->SetState(GameInputFeedbackRunning);
                }

                if (uiState.buttons & GameInputUiNavigationCancel)
                {
                    m_buttonString += L"[B] ";

                    m_feedbackEffect->SetState(GameInputFeedbackStopped);
                }
            }
        }
    }
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

    auto fullscreen = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    wchar_t tempString[256] = {};
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    m_batch->Begin(commandList);

    if (!m_buttonString.empty())
    {
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), m_buttonString.c_str(), pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"Wheel  %1.3f", m_wheelValue);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"Clutch  %1.3f", m_clutchValue);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"Brake  %1.3f", m_brakeValue);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"Throttle  %1.3f", m_throttleValue);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"Handbrake  %1.3f", m_handbrakeValue);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"Shifter  %d", m_patternShifterGearValue);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        if (m_feedbackEffect->GetState() == GameInputFeedbackRunning)
        {
            swprintf(tempString, 255, L"Feedback On");
        }
        else
        {
            swprintf(tempString, 255, L"Feedback Off");
        }

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;
    }
    else
    {
        m_font->DrawString(m_batch.get(), L"No wheel connected", pos, ATG::Colors::Orange);
    }

    DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), L"[A] Feedback On   [B] Feedback Off   [View][Menu] Exit",
        XMFLOAT2(float(safeRect.left), float(safeRect.bottom) - m_font->GetLineSpacing()), ATG::Colors::LightGrey);

    m_batch->End();

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
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    m_font = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_24.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintFont));

    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        L"XboxOneControllerLegendSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"ATGSampleBackground.dds", m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);
}
#pragma endregion
