//--------------------------------------------------------------------------------------
// GamepadVibration.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GamepadVibration.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    static const wchar_t* c_TRIGGER_EFFECT_NAME_TEXT[Sample::TRIGGEREFFECTS_MAX] =
    {
        L"<Trigger Test>\n",
        L"<Flat Tire>\n",
        L"<Gun with Recoil>\n",
        L"<Heartbeat>\n",
        L"<Footsteps>\n"
    };

    static const wchar_t* c_TRIGGER_EFFECT_DESC_TEXT[Sample::TRIGGEREFFECTS_MAX] =
    {
        L"Use the [LT] and [RT] to test the feedback\n"
        L"function of the gamepad. The envelope is set based on\n"
        L"the trigger position. The more you pull the triggers,\n"
        L"the more feedback you will feel.",

        L"Impulse triggers can provide feedback about the environment.\n"
        L"Assuming the player is driving a car, this example uses\n"
        L"the impulse triggers to inform a flat tire on the left side.",

        L"Demonstrates how impulse triggers can be combined with the\n"
        L"vibration motors to simulate weapon firing and recoil.\n"
        L"Press the [LT] to activate the effect.",

        L"Impulse triggers can relay information about the player\'s\n"
        L"in-game representation. Here we relay the character\'s\n"
        L"heartbeat, which can be used to let the player know that\n"
        L"their character is exhausted.",

        L"Impulse triggers can relay information external to the\n"
        L"player. This example use the impulse triggers to simulate\n"
        L"footsteps which could indicate the presence of a nearby\n"
        L"character."
    };

    uint32_t flatTireLeftTriggerDurations[] = { 33, 80, 16 };
    float flatTireLeftTriggerLevels[] = { 0.8f, 0.0f, 0.0f };

    uint32_t gunWithRecoilLeftTriggerDurations[] = { 20, 10, 90, 10000 };
    float gunWithRecoilLeftTriggerLevels[] = { 1.0f, 0.0f, 0.0f, 0.0f };

    uint32_t heartbeatLeftTriggerDurations[] = { 25, 200, 25, 10, 745 };
    float heartbeatLeftTriggerLevels[] = { 0.2f, 0.0f, 0.0f, 0.0f, 0.0f };
    uint32_t heartbeatRightTriggerDurations[] = { 25, 200, 25, 10, 745 };
    float heartbeatRightTriggerLevels[] = { 0.0f, 0.0f, 0.2f, 0.02f, 0.0f };

    uint32_t footstepsLeftTriggerDurations[] = { 25, 600, 25, 600 };
    float footstepsLeftTriggerLevels[] = { 0.3f, 0.0f, 0.0f, 0.0f };
    uint32_t footstepsRightTriggerDurations[] = { 25, 600, 25, 600 };
    float footstepsRightTriggerLevels[] = { 0.0f, 0.0f, 0.3f, 0.0f };
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_connected(false),
    m_dPadPressed(false),
    m_leftMotorSpeed(0),
    m_rightMotorSpeed(0),
    m_leftTriggerLevel(0),
    m_rightTriggerLevel(0),
    m_selectedTriggerEffect(TRIGGEREFFECTS::TRIGGEREFFECTS_IMPULSETEST),
    m_triggerEffectCounter(0),
    m_leftTriggerArraySize(0),
    m_pLeftTriggerDurations(nullptr),
    m_pLeftTriggerLevels(nullptr),
    m_rightTriggerArraySize(0),
    m_pRightTriggerDurations(nullptr),
    m_pRightTriggerLevels(nullptr),
    m_leftTriggerIndex(0),
    m_rightTriggerIndex(0),
    m_frequency(0),
    m_counter(0),
    m_leftTriggerIndexUpdateTime(0),
    m_rightTriggerIndexUpdateTime(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
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
    m_deviceResources->SetWindow(window, width, height);

    m_selectedTriggerEffect = TRIGGEREFFECTS::TRIGGEREFFECTS_IMPULSETEST;

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    DX::ThrowIfFailed(GameInputCreate(&m_gameInput));

    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&m_frequency));
}

//--------------------------------------------------------------------------------------
// Name: InitializeImpulseTriggerEffects()
// Desc: Clear variables used by the tigger effects and initialize them as needed for 
//       the currently selected effect.
//--------------------------------------------------------------------------------------
void Sample::InitializeImpulseTriggerEffects()
{
    m_leftTriggerIndex = 0;
    m_rightTriggerIndex = 0;

    m_leftMotorSpeed = 0;
    m_leftTriggerLevel = 0;
    m_rightMotorSpeed = 0;
    m_rightTriggerLevel = 0;

    m_triggerEffectCounter = 0;
    switch (m_selectedTriggerEffect)
    {
    case TRIGGEREFFECTS::TRIGGEREFFECTS_IMPULSETEST:
        break;

    case TRIGGEREFFECTS::TRIGGEREFFECTS_FLATTIRE:
        m_leftTriggerArraySize = 3;

        m_pLeftTriggerDurations = flatTireLeftTriggerDurations;
        m_pLeftTriggerLevels = flatTireLeftTriggerLevels;

        // Set the timing for the transition to the second vibration level
        // Further transition timings will be handled by the transition code.
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&m_counter));
        m_leftTriggerIndexUpdateTime = m_counter + (m_frequency * m_pLeftTriggerDurations[m_leftTriggerIndex]) / 1000;
        break;

    case TRIGGEREFFECTS::TRIGGEREFFECTS_GUNWITHRECOIL:
        m_leftTriggerArraySize = 4;

        m_pLeftTriggerDurations = gunWithRecoilLeftTriggerDurations;
        m_pLeftTriggerLevels = gunWithRecoilLeftTriggerLevels;
        break;

    case TRIGGEREFFECTS::TRIGGEREFFECTS_HEARTBEAT:
        m_leftTriggerArraySize = 5;
        m_rightTriggerArraySize = 5;

        m_pLeftTriggerDurations = heartbeatLeftTriggerDurations;
        m_pLeftTriggerLevels = heartbeatLeftTriggerLevels;
        m_pRightTriggerDurations = heartbeatRightTriggerDurations;
        m_pRightTriggerLevels = heartbeatRightTriggerLevels;

        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&m_counter));
        m_leftTriggerIndexUpdateTime = m_counter + (m_frequency * m_pLeftTriggerDurations[m_leftTriggerIndex]) / 1000;
        m_rightTriggerIndexUpdateTime = m_counter + (m_frequency * m_pRightTriggerDurations[m_rightTriggerIndex]) / 1000;
        break;

    case TRIGGEREFFECTS::TRIGGEREFFECTS_FOOTSTEPS:
        m_leftTriggerArraySize = 4;
        m_rightTriggerArraySize = 4;

        m_pLeftTriggerDurations = footstepsLeftTriggerDurations;
        m_pLeftTriggerLevels = footstepsLeftTriggerLevels;
        m_pRightTriggerDurations = footstepsRightTriggerDurations;
        m_pRightTriggerLevels = footstepsRightTriggerLevels;

        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&m_counter));
        m_leftTriggerIndexUpdateTime = m_counter + (m_frequency * m_pLeftTriggerDurations[m_leftTriggerIndex]) / 1000;
        m_rightTriggerIndexUpdateTime = m_counter + (m_frequency * m_pRightTriggerDurations[m_rightTriggerIndex]) / 1000;
        break;

    default:
        assert(false);
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    if (SUCCEEDED(m_gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &m_reading)))
    {
        m_connected = true;

        GameInputGamepadState state = {};

        if (m_reading->GetGamepadState(&state))
        {
            if (!m_dPadPressed)
            {
                if (state.buttons & GameInputGamepadDPadRight)
                {
                    m_dPadPressed = true;
                    m_selectedTriggerEffect = static_cast<TRIGGEREFFECTS>((static_cast<int>(m_selectedTriggerEffect) + 1) % TRIGGEREFFECTS_MAX);
                    InitializeImpulseTriggerEffects();
                }

                if (state.buttons & GameInputGamepadDPadLeft)
                {
                    m_dPadPressed = true;
                    m_selectedTriggerEffect = static_cast<TRIGGEREFFECTS>((static_cast<int>(m_selectedTriggerEffect) + TRIGGEREFFECTS_MAX - 1) % TRIGGEREFFECTS_MAX);
                    InitializeImpulseTriggerEffects();
                }
            }
            else if(!(state.buttons & GameInputGamepadDPadRight) && !(state.buttons & GameInputGamepadDPadLeft))
            {
                m_dPadPressed = false;
            }

            if (state.buttons & GameInputGamepadView)
            {
                ExitSample();
            }
        }

        ComPtr<IGameInputDevice> device;
        m_reading->GetDevice(&device);

        //Set rumble

        switch (m_selectedTriggerEffect)
        {
        case TRIGGEREFFECTS::TRIGGEREFFECTS_IMPULSETEST:
            // This example uses a very simple vibration envelope waveform by setting the vibration
            // levels to the current trigger values. This means the more you pull the triggers, the more
            // vibration you will feel.
            m_leftTriggerLevel = state.leftTrigger;
            m_rightTriggerLevel = state.rightTrigger;
            m_leftMotorSpeed = state.leftTrigger;
            m_rightMotorSpeed = state.rightTrigger;
            break;

        case TRIGGEREFFECTS::TRIGGEREFFECTS_FLATTIRE:
            m_leftTriggerLevel = m_pLeftTriggerLevels[m_leftTriggerIndex];

            // If we've reached or passed the transition time, update m_leftTriggerIndexUpdateTime
            // with the next transition time and update the effect index.
            // This will cause the effect to change in the next loop iteration.
            QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&m_counter));
            if (m_counter > m_leftTriggerIndexUpdateTime)
            {
                m_leftTriggerIndex = (m_leftTriggerIndex + 1) % m_leftTriggerArraySize;
                m_leftTriggerIndexUpdateTime = m_counter + (m_frequency * m_pLeftTriggerDurations[m_leftTriggerIndex]) / 1000;
            }
            break;

        case TRIGGEREFFECTS::TRIGGEREFFECTS_GUNWITHRECOIL:
            switch (m_triggerEffectCounter)
            {
            case 0: // Wait for the trigger to be fully released before
                    // the effect can begin
                if (state.leftTrigger <= 1.0f / 255.0f)
                {
                    m_triggerEffectCounter = 1;
                }
                break;

            case 1: // Wait for the trigger to be depressed enough to cause the gun to fire
                if (state.leftTrigger >= 32.0f / 255.0f)
                {
                    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&m_counter));
                    m_leftTriggerIndexUpdateTime = m_counter + (m_frequency * m_pLeftTriggerDurations[m_leftTriggerIndex]) / 1000;
                    m_triggerEffectCounter = 2;
                }
                break;

            case 2: // Delay recoil a little after the bullet has left the gun
                m_leftTriggerLevel = m_pLeftTriggerLevels[m_leftTriggerIndex];

                if (m_leftTriggerIndex == 2)
                {
                    m_leftMotorSpeed = 1.0f;
                    m_rightMotorSpeed = 1.0f;
                }
                else
                {
                    m_leftMotorSpeed = 0.0f;
                    m_rightMotorSpeed = 0.0f;
                }

                if (m_leftTriggerIndex == 3)
                {
                    m_leftTriggerIndex = 0;
                    m_triggerEffectCounter = 0;
                    break;
                }

                QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&m_counter));
                if (m_counter > m_leftTriggerIndexUpdateTime)
                {
                    m_leftTriggerIndex = (m_leftTriggerIndex + 1) % m_leftTriggerArraySize;
                    m_leftTriggerIndexUpdateTime = m_counter + (m_frequency * m_pLeftTriggerDurations[m_leftTriggerIndex]) / 1000;
                }
                break;
            }
            break;

        case TRIGGEREFFECTS::TRIGGEREFFECTS_HEARTBEAT:
            // use the left level/duration for both triggers
            m_leftTriggerLevel = m_pLeftTriggerLevels[m_leftTriggerIndex];
            m_rightTriggerLevel = m_pRightTriggerLevels[m_rightTriggerIndex];

            QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&m_counter));
            if (m_counter > m_leftTriggerIndexUpdateTime)
            {
                m_leftTriggerIndex = (m_leftTriggerIndex + 1) % m_leftTriggerArraySize;
                m_leftTriggerIndexUpdateTime = m_counter + (m_frequency * m_pLeftTriggerDurations[m_leftTriggerIndex]) / 1000;
            }
            if (m_counter > m_rightTriggerIndexUpdateTime)
            {
                m_rightTriggerIndex = (m_rightTriggerIndex + 1) % m_rightTriggerArraySize;
                m_rightTriggerIndexUpdateTime = m_counter + (m_frequency * m_pRightTriggerDurations[m_rightTriggerIndex]) / 1000;
            }
            break;

        case TRIGGEREFFECTS::TRIGGEREFFECTS_FOOTSTEPS:
            m_leftTriggerLevel = m_pLeftTriggerLevels[m_leftTriggerIndex];
            m_rightTriggerLevel = m_pRightTriggerLevels[m_rightTriggerIndex];

            QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&m_counter));
            if (m_counter > m_leftTriggerIndexUpdateTime)
            {
                m_leftTriggerIndex = (m_leftTriggerIndex + 1) % m_leftTriggerArraySize;
                m_leftTriggerIndexUpdateTime = m_counter + (m_frequency * m_pLeftTriggerDurations[m_leftTriggerIndex]) / 1000;
            }
            if (m_counter > m_rightTriggerIndexUpdateTime)
            {
                m_rightTriggerIndex = (m_rightTriggerIndex + 1) % m_rightTriggerArraySize;
                m_rightTriggerIndexUpdateTime = m_counter + (m_frequency * m_pRightTriggerDurations[m_rightTriggerIndex]) / 1000;
            }
            break;

        default:
            assert(false);
        }

        GameInputRumbleParams vibration = {};
        vibration.lowFrequency = m_leftMotorSpeed;
        vibration.highFrequency = m_rightMotorSpeed;
        vibration.leftTrigger = m_leftTriggerLevel;
        vibration.rightTrigger = m_rightTriggerLevel;
        device->SetRumbleState(&vibration);
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

    auto const fullscreen = m_deviceResources->GetOutputSize();

    auto const safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_batch->Begin(commandList);

    m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    if (m_connected)
    {
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(),
            L"Use the [DPad] Left and Right to select a vibration effect.", pos, ATG::Colors::OffWhite);
        pos.y += m_font->GetLineSpacing() * 2.f;

        // Draw description
        m_font->DrawString(m_batch.get(), c_TRIGGER_EFFECT_NAME_TEXT[static_cast<int>(m_selectedTriggerEffect)], pos, ATG::Colors::Green);
        pos.y += m_font->GetLineSpacing() * 1.5f;
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), c_TRIGGER_EFFECT_DESC_TEXT[static_cast<int>(m_selectedTriggerEffect)], pos);
    }
    else
    {
        m_font->DrawString(m_batch.get(), L"No controller connected", pos, ATG::Colors::Orange);
    }

    DX::DrawControllerString(m_batch.get(),
        m_smallFont.get(), m_ctrlFont.get(),
        L"[View] Exit   [Menu] Help",
        XMFLOAT2(float(safeRect.left),
            float(safeRect.bottom) - m_smallFont->GetLineSpacing()),
        ATG::Colors::LightGrey);

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
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
void Sample::OnActivated()
{
}

void Sample::OnDeactivated()
{
}

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
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        const SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_24.spritefont");
    m_font = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_smallFont = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"gamepad.dds");
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, strFilePath,
        m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);
}

void Sample::OnDeviceLost()
{
    m_font.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();
    m_batch.reset();
    m_resourceDescriptors.reset();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
