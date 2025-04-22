//--------------------------------------------------------------------------------------
// SimpleFFBWheel.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleFFBWheel.h"

#include "ATGColors.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
    // [SAMPLE] some sane default parameters for the various effects
    GameInputForceFeedbackMagnitude g_magnitude =
    {
        // [SAMPLE] Starting with the October 2021 GDK, use the angularX value instead the linearX value
        0.0f, // float linearX
        0.0f, // float linearY
        0.0f, // float linearZ
        1.0f, // float angularX
        0.0f, // float angularY
        0.0f, // float angularZ
        0.0f  // float normal
    };

    GameInputForceFeedbackConditionParams g_returnToCenterParams =
    {
        g_magnitude, // GameInputForceFeedbackMagnitude magnitude
        0.0f,        // float positiveCoefficient
        0.0f,        // float negativeCoefficient
        0.5f,        // float maxPositiveMagnitude
       -0.5f,        // float maxNegativeMagnitude
        0.005f,      // float deadZone
        0.0f         // float bias
    };

    GameInputForceFeedbackEnvelope g_gravelEnvelope =
    {
        250000,  // uint64_t attackDuration (us)
        3000000, // uint64_t sustainDuration (us)
        250000,  // uint64_t releaseDuration (us)
        0.5f,    // float attackGain
        1.0f,    // float sustainGain
        0.5f,    // float releaseGain
        1,       // uint32_t playCount
        0        // uint64_t repeatDelay
    };

    GameInputForceFeedbackPeriodicParams g_gravelParams =
    {
        g_gravelEnvelope, // GameInputForceFeedbackEnvelope envelope
        g_magnitude,      // GameInputForceFeedbackMagnitude magnitude
        15.0f,            // float frequency
         0.0f,            // float phase
         0.0f,            // float bias
    };

    GameInputForceFeedbackConditionParams g_damperParams =
    {
        g_magnitude, // GameInputForceFeedbackMagnitude magnitude
        0.0f,        // float positiveCoefficient
        0.0f,        // float negativeCoefficient
        0.25f,       // float maxPositiveMagnitude
       -0.25f,       // float maxNegativeMagnitude
        0.0f,        // float deadZone
        0.0f         // float bias
    };
}


void Sample::StartEffect(EffectType type)
{
    if (!m_wheelDevice)
        return;

    const GameInputDeviceInfo* deviceInfo;

#if GAMEINPUT_API_VERSION >= 1
    m_wheelDevice->GetDeviceInfo(&deviceInfo);
#else
    deviceInfo = m_wheelDevice->GetDeviceInfo();
#endif

    bool foundMotor = false;
    SampleEffect& sampleEffect = GetSampleEffect(type);
    uint32_t motorIndex;

    // [SAMPLE] Find a motor that supports the requested effect, and set the default parameters
    for (motorIndex = 0; motorIndex < deviceInfo->forceFeedbackMotorCount && !foundMotor; motorIndex++)
    {
        auto& motorInfo = deviceInfo->forceFeedbackMotorInfo[motorIndex];

        switch(type)
        {
            case EffectType::Spring:
                if (motorInfo.isSpringEffectSupported)
                {
                    sampleEffect.params.kind = GameInputForceFeedbackSpring;
                    sampleEffect.params.data.spring = g_returnToCenterParams;
                    sampleEffect.name = L"Spring";
                    foundMotor = true;
                }
                else
                {
                    m_log->Format(L"Spring effect not supported on motor %d", motorIndex);
                }
                break;
            case EffectType::Damper:
                if (motorInfo.isDamperEffectSupported)
                {
                    sampleEffect.params.kind = GameInputForceFeedbackDamper;
                    sampleEffect.params.data.damper = g_damperParams;
                    sampleEffect.name = L"Damper";
                    foundMotor = true;
                }
                else
                {
                    m_log->Format(L"Damper effect not supported on motor %d", motorIndex);
                }
                break;
            case EffectType::Gravel:
                if (motorInfo.isSawtoothUpWaveEffectSupported)
                {
                    sampleEffect.params.kind = GameInputForceFeedbackSawtoothUpWave;
                    sampleEffect.params.data.sawtoothUpWave = g_gravelParams;
                    sampleEffect.name = L"SawtoothUp";
                    foundMotor = true;
                }
                else
                {
                    m_log->Format(L"Sawtooth Up Wave effect not supported on motor %d", motorIndex);
                }
                break;
        }

        if (foundMotor)
        {
            break;
        }
    }

    if (foundMotor)
    {
        // [SAMPLE] A motor was found, now create the force feedback effect with the default parameters
        HRESULT hr = m_wheelDevice->CreateForceFeedbackEffect(motorIndex, &sampleEffect.params, sampleEffect.effect.GetAddressOf());
        if (SUCCEEDED(hr))
        {
            // [SAMPLE] ...and start the effect immediately
            m_log->Format(L"Starting %ls effect\n", sampleEffect.name.c_str());
            sampleEffect.effect->SetState(GameInputFeedbackEffectState::GameInputFeedbackRunning);
        }
        else
        {
            m_log->Format(Colors::Red, L"Failed to create force feedback effect on motor %d: %08X\n", motorIndex, hr);
        }
    }
    else
    {
        m_log->WriteLine(Colors::Red, L"Could not find a motor that supports the requested effect");
    }
}

void Sample::UpdateEffect(EffectType type)
{
    SampleEffect& sampleEffect = GetSampleEffect(type);

    // [SAMPLE] If an effect is running, update its parameters
    // Note that the scaling and clamping done below is just an example and isn't intended
    // to provide an accurate force feedback experience
    if (sampleEffect.effect && sampleEffect.effect->GetState() == GameInputFeedbackEffectState::GameInputFeedbackRunning)
    {
        auto& params = GetSampleEffect(type).params;

        switch(type)
        {
            case EffectType::Spring:
                params.data.spring.negativeCoefficient = std::clamp(m_speed/-50.0f, -1.0f, 0.0f);
                params.data.spring.positiveCoefficient = std::clamp(m_speed/-50.0f, -1.0f, 0.0f);
                break;
            case EffectType::Damper:
                params.data.damper.negativeCoefficient = std::clamp(-1.0f - (m_speed/-100.0f), -1.0f, -0.05f);
                params.data.damper.positiveCoefficient = std::clamp(-1.0f - (m_speed/-100.0f), -1.0f, -0.05f);
                break;
            case EffectType::Gravel:
                // [SAMPLE] the SawtoothUp envelope has all the info it needs to run the effect
                // Note that we do not take speed into account for this effect
                break;
        }
        sampleEffect.effect->SetParams(&params);
    }
}

void Sample::StopEffect(EffectType type)
{
    SampleEffect& sampleEffect = GetSampleEffect(type);

    if (sampleEffect.effect)
    {
        m_log->Format(L"Stopping %ls effect\n", sampleEffect.name.c_str());
        sampleEffect.effect->SetState(GameInputFeedbackEffectState::GameInputFeedbackStopped);
        sampleEffect.effect = nullptr;
    }
}

void Sample::UpdateForceFeedbackFrame()
{
    wchar_t ffbStateText[256]{};

    if(m_wheelDevice)
    {
        // [SAMPLE] Request the most recent reading from the wheel
        HRESULT hr = m_gameInput->GetCurrentReading(GameInputKindRacingWheel, nullptr, &m_reading);
        if (SUCCEEDED(hr))
        {
            // [SAMPLE] From that reading, get the state of thw wheel, pedals, etc.
            if (m_reading->GetRacingWheelState(&m_wheelState))
            {
                swprintf_s(ffbStateText, L"Throttle   Brake   Clutch   Handbrake   Wheel");
                m_valuesHeader->SetText(ffbStateText);

                // update on-screen stats
                swprintf_s(ffbStateText, L"%.3f        %.3f    %.3f     %.3f             %.3f",
                            m_wheelState.throttle, m_wheelState.brake, m_wheelState.clutch, m_wheelState.handbrake, m_wheelState.wheel);
                m_valuesLabel->SetForegroundColor(Colors::White);
                m_valuesLabel->SetText(ffbStateText);

                m_inputState.Accelerator = m_wheelState.throttle;
                m_inputState.Brake = m_wheelState.brake;

                // allow simple accelerator/brake control with dpad
                if (m_wheelState.buttons & GameInputRacingWheelDpadUp)
                {
                    m_inputState.Accelerator = 0.5f;
                }
                else if (m_wheelState.buttons & GameInputRacingWheelDpadDown)
                {
                    m_inputState.Brake = 0.3f;
                }

                m_inputState.Gravel = GetRacingWheelButtonPressed(GameInputRacingWheelMenu);
                m_inputState.Exit   = GetRacingWheelButtonPressed(GameInputRacingWheelView);

                m_lastRacingWheelButtons = m_wheelState.buttons;
            }
            else
            {
                m_valuesLabel->SetForegroundColor(Colors::Red);
                m_valuesLabel->SetText(L"GetRacingWheelState failed");
            }
        }
        else
        {
            swprintf_s(ffbStateText, L"Error getting reading: %08X", hr);
            m_valuesLabel->SetForegroundColor(Colors::Red);
            m_valuesLabel->SetText(ffbStateText);
        }
    }
    else
    {
        swprintf_s(ffbStateText, L"Supported racing wheel not connected.");
        m_valuesLabel->SetForegroundColor(Colors::Red);
        m_valuesLabel->SetText(ffbStateText);
    }

    if (m_inputState.Exit)
    {
        ExitSample();
    }

    if (m_inputState.Gravel)
    {
        // [SAMPLE] Start the Gravel (SawtoothUp) effect
        StartEffect(EffectType::Gravel);
    }

    // [SAMPLE] do some very simple accelerator/brake handling so we can alter effects based on the "speed" of the car
    if (m_inputState.Accelerator > 0.0f)
    {
        m_speed += m_inputState.Accelerator / 2.5f;
    }
    else if (m_inputState.Brake > 0.0f)
    {
        m_speed -= m_inputState.Brake * 1.5f;
    }
    else
    {
        m_speed -= 0.1f;
    }

    m_speed = std::clamp(m_speed, 0.0f, 200.0f);

    wchar_t speedText[16]{};
    swprintf_s(speedText, L"%d MPH", static_cast<uint32_t>(m_speed));
    m_speedLabel->SetText(speedText);

    // [SAMPLE] update our effects on a per-frame basis
    UpdateEffect(EffectType::Spring);
    UpdateEffect(EffectType::Damper);
}

void CALLBACK Sample::OnGameInputDeviceAddedRemoved(
    _In_ GameInputCallbackToken,
    _In_ void * context,
    _In_ IGameInputDevice * device,
    _In_ uint64_t,
    _In_ GameInputDeviceStatus currentStatus,
    _In_ GameInputDeviceStatus previousStatus) noexcept
{
    auto sample = reinterpret_cast<Sample*>(context);

    bool wasConnected = (previousStatus & GameInputDeviceConnected) != 0;
    bool isConnected = (currentStatus & GameInputDeviceConnected) != 0;

    // [SAMPLE] Device connected and we're not already tracking a wheel?
    if (isConnected && !wasConnected && !sample->m_wheelDevice)
    {
        const GameInputDeviceInfo* deviceInfo;

#if GAMEINPUT_API_VERSION >= 1
        device->GetDeviceInfo(&deviceInfo);
#else
        deviceInfo = device->GetDeviceInfo();
#endif

        // [SAMPLE] determine if this device has more than zero FFB motors (i.e. it's a device that supports force feedback)
        // If it does, we'll use it for the sample.
        if (deviceInfo->forceFeedbackMotorCount > 0)
        {
            sample->m_log->Format(L"with %d force feedback motor(s)\n", deviceInfo->forceFeedbackMotorCount);
            sample->m_wheelDevice = device;

            sample->StartEffect(EffectType::Spring);
            sample->StartEffect(EffectType::Damper);
        }
        else
        {
            sample->m_log->WriteLine(L"with no force feedback motors");
        }
    }
    else if(!isConnected && wasConnected && sample->m_wheelDevice.Get() == device) // [SAMPLE] Device disconnected
    {
        sample->m_log->WriteLine(L"Device disconnected");
        sample->m_wheelDevice.Reset();
        sample->m_wheelDevice = nullptr;
    }
}

Sample::Sample() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    ATG::UIConfig uiconfig;

    m_ui = std::make_unique<ATG::UIManager>(uiconfig);
    m_log = std::make_unique<DX::TextConsoleImage>();
}

Sample::~Sample()
{
    // [SAMPLE] Unregister the device callback on exit
    if (m_callbackToken)
    {
#if GAMEINPUT_API_VERSION >= 1
        m_gameInput->UnregisterCallback(m_callbackToken);
#else
        m_gameInput->UnregisterCallback(m_callbackToken, 5000);
#endif
        m_callbackToken = 0;
    }

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    wchar_t result[MAX_PATH];
    DX::FindMediaFile(result, MAX_PATH, L".\\Assets\\SampleUI.csv");
    m_ui->LoadLayout(result, L".\\Assets\\");
    m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Show();
    m_valuesHeader = m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_valuesHeader);
    m_valuesLabel = m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_valuesLabel);
    m_descBox = m_ui->FindControl<ATG::TextBox>(c_sampleUIPanel, c_descBox);
    m_speedLabel = m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_speedLabel);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_log->WriteLine(L"Please connect a Racing Wheel...");

    // [SAMPLE] create the GameInput stack
    HRESULT hr = GameInputCreate(&m_gameInput);
    DX::ThrowIfFailed(hr);

    // [SAMPLE] setup callback when a Racing Wheel is connected or disconnected
    // Note: Racing wheels that use HID drivers may not report as a GameInputKindRacingWheel, so
    // it is best to also check for the base Controller type for mapping later
    hr = m_gameInput->RegisterDeviceCallback(nullptr,
        GameInputKindRacingWheel,
        GameInputDeviceConnected,
        GameInputBlockingEnumeration,
        this,
        OnGameInputDeviceAddedRemoved,
        &m_callbackToken);
    DX::ThrowIfFailed(hr);

    SetupUI();
}

bool Sample::GetRacingWheelButtonPressed(GameInputRacingWheelButtons button)
{
    return (m_wheelState.buttons & button && !(m_lastRacingWheelButtons & button));
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

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    UpdateForceFeedbackFrame();

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

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    // [SAMPLE] Rotate and draw the steering wheel image based on the current state of the wheel
    // This was tested against a Thrustmaster TS-XW, but may not line up perfectly for other wheels
    m_spriteBatch->Begin(commandList);
        m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::SteeringWheel), GetTextureSize(m_wheel.Get()), XMFLOAT2(900, 500), nullptr, Colors::White, m_wheelState.wheel * 10.0f, m_wheelOrigin);
    m_spriteBatch->End();

    m_log->Render(commandList);
    m_ui->Render(commandList);

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::SetupUI()
{
    m_descBox->SetText(
        L"Use the pedals or DPad up/down to accelerate or brake. The wheel's force feedback (Spring and Damper) will change based on the speed of the vehicle.\n\n"
        L"Press the View button to simulate the effect of driving over a rough road or gravel (SawtoothUp)."
    );
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

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    wchar_t font[MAX_PATH];
    DX::FindMediaFile(font, MAX_PATH, L"courier_16.spritefont");

    wchar_t background[MAX_PATH];
    DX::FindMediaFile(background, MAX_PATH, L"ATGSampleBackground.DDS");

    m_log->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        font,
        background,
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Background)
    );

    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    wchar_t path[MAX_PATH];
    DX::FindMediaFile(path, MAX_PATH, L".\\Assets\\SteeringWheel.dds");
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, path, m_wheel.ReleaseAndGetAddressOf()));
    CreateShaderResourceView(device, m_wheel.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::SteeringWheel));
    auto size = GetTextureSize(m_wheel.Get());
    m_wheelOrigin = XMFLOAT2(size.x/2.0f, size.y/2.0f);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT fullscreen = m_deviceResources->GetOutputSize();
    auto viewport = m_deviceResources->GetScreenViewport();

    static const RECT logSize = { 980, 800, 1780, 1050 };
    m_log->SetWindow(logSize, false);
    m_log->SetViewport(viewport);

    m_spriteBatch->SetViewport(viewport);

    m_ui->SetWindow(fullscreen);
}

void Sample::OnDeviceLost()
{
    m_ui->ReleaseDevice();
    m_graphicsMemory.reset();
    m_resourceDescriptors.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
