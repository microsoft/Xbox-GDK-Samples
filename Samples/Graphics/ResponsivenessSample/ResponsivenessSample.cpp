//--------------------------------------------------------------------------------------
// ResponsiveSample.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ResponsivenessSample.h"

#include <GameInput.h>

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"
#include "Random.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#pragma warning(disable:4244) // warning C4244 : 'argument' : conversion from 'x' to 'y', possible loss of data
#pragma warning(disable:4365) // warning C4365 : 'argument' : conversion from 'x' to 'y', signed / unsigned mismatch

namespace
{
    constexpr float MAXSPEED = 1000.0 * 1.5f;
    constexpr float ACCELERATION = 0.3f;

    //constexpr float ANGULARVELOCITY = 2.0f;

    enum SETTINGS
    {
        SETTINGS_PRESET_CONFIGURATION,
        SETTINGS_PRESENT_INTERVAL,
        SETTINGS_PRESENT_FLAGS,
        SETTINGS_GPU_WORKLOAD,
        SETTINGS_BACK_BUFFER_COUNT,
        SETTINGS_COUNT,
    };

    enum PRESET_FAMILY
    {
        PRESET_FAMILY_55FPS_COMPARISON,
        PRESET_FAMILY_40FPS_COMPARISON,
        PRESET_FAMILY_TRIPLE_BUFFER_COMPARISON,
        PRESET_FAMILY_ALL,
        PRESET_FAMILY_COUNT,
    };

    enum PRESET_CONFIGURATION
    {
        PRESET_CONFIGURATION_60FPS_NORMAL,
        PRESET_CONFIGURATION_55FPS_STUTTER,
        PRESET_CONFIGURATION_55FPS_FREESYNC,
        PRESET_CONFIGURATION_90FPS_WITH_60FPS_PLUS,
        PRESET_CONFIGURATION_55FPS_120HZ,
        PRESET_CONFIGURATION_120FPS_NORMAL,
        PRESET_CONFIGURATION_100FPS_FREESYNC,
        PRESET_CONFIGURATION_60FPS_NORMAL_PART_TWO,
        PRESET_CONFIGURATION_30FPS_NORMAL,
        PRESET_CONFIGURATION_40FPS_NORMAL,
        PRESET_CONFIGURATION_TRIPLE_BUFFER_PERIOD_3,
        PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_2,
        PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_2_MISSED_FRAME_RATE,
        PRESET_CONFIGURATION_TRIPLE_BUFFER_PERIOD_2,
        PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_1,
        PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_1_120FPS,
#ifdef _GAMING_XBOX_SCARLETT
        PRESET_CONFIGURATION_NO_DLI,
        PRESET_CONFIGURATION_DLI,
#endif
        PRESET_CONFIGURATION_COUNT,
    };

    const wchar_t* PRESET_CONFIGURATION_STRING[] =
    {
        L"60 FPS HAPPY",
        L"55 FPS STUTTER @ 60 Hz",
        L"55 FPS VRR",
        L"'60 FPS Plus' Mode",
        L"55 FPS @ 120 Hz",
        L"120 FPS",
        L"100 FPS VRR",
        L"60 FPS",
        L"30 FPS",
        L"40 FPS",
        L"TRIPLE BUFFERING PERIOD=3 LATENCY",
        L"DOUBLE BUFFERING PERIOD=2 LATENCY",
        L"DOUBLE BUFFERING PERIOD=2 MISSED FRAME RATE",
        L"TRIPLE BUFFERING PERIOD=2 LATENCY",
        L"DOUBLE BUFFERING PERIOD=1 LATENCY",
        L"DOUBLE BUFFERING PERIOD=1 120 FPS LATENCY",
#ifdef _GAMING_XBOX_SCARLETT
        L"NO DLI",
        L"DLI",
#endif
    };

    const wchar_t* DEMO_DESCRIPTION_STRING[] =
    {
        // PRESET_CONFIGURATION_60FPS_NORMAL
        L"60 FPS Happy: A happy 60 fps title with consistent frame rate",
        // PRESET_CONFIGURATION_55FPS_STUTTER
        L"55 FPS Stutter @ 60 Hz Transmission: Up to 16ms of stutter",
        // PRESET_CONFIGURATION_55FPS_FREESYNC
        L"55 FPS Variable Refresh Rate: HDMI Transmission will vary to match the frame rate",
        // PRESET_CONFIGURATION_90FPS_WITH_60FPS_PLUS
        L"90 FPS: Title runs at 60 fps, but can present early using '60 fps plus' mode",
        // PRESET_CONFIGURATION_55FPS_120HZ
        L"55 FPS @ 120 Hz Transmission: Significantly reduced 8.3ms of stutter",
        // PRESET_CONFIGURATION_120FPS_NORMAL
        L"120 FPS Title",
        // PRESET_CONFIGURATION_100FPS_FREESYNC
        L"100 FPS Variable Refresh Rate",
        // PRESET_CONFIGURATION_60FPS_NORMAL_PART_TWO
        L"60 FPS Title",
        // PRESET_CONFIGURATION_30FPS_NORMAL
        L"30 FPS Title",
        // PRESET_CONFIGURATION_40FPS_NORMAL
        L"40 FPS Title @ 120 Hz Transmission: Every frame is transmitted 3 * 8.3ms = 25ms (40fps)",
        // PRESET_CONFIGURATION_TRIPLE_BUFFER_PERIOD_3
        L"Triple Buffering w/ Period 3: 3 frames of latency worse case",
        // PRESET_CONFIGURATION_DOUBLE_BUFFER
        L"Double Buffering: 2 frames of latency",
        // PRESET_CONFIGURATION_DOUBLE_BUFFER_MISSED_FRAME_RATE
        L"Double Buffering: When frame rate is missed, the penality is severe",
        // PRESET_CONFIGURATION_TRIPLE_BUFFER_PERIOD_2
        L"PresentX() allows Triple Buffering w/ Period 2: Only 2 frames of latency and smooth triple buffering",
        // PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_1
        L"Super-responsive: Double-Buffering /w Period 1: Only 1 frame of latency (minus 1 ms offset)",
        // PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_1_120FPS,
        L"Ultra-responsive: Double-Buffering /w Period 1: Only 1 frame of latency at 120 Hz (minus 1 ms offset)",
#ifdef _GAMING_XBOX_SCARLETT
        // PRESET_CONFIGURATION_NO_DLI
        L"DLI Off",
        // PRESET_CONFIGURATION_DLI
        L"DLI On",
#endif
    };

    enum PRESENT_FLAGS {
        PRESENT_FLAGS_NONE,
        PRESENT_FLAGS_DISABLE_VRR,
    };

    const wchar_t* PRESENT_FLAGS_STRINGS[] =
    {
        L"NONE",
        L"VRR Disabled",
    };

    enum PRESENT_INTERVAL
    {
        PRESENT_INTERVAL_8MS,
        PRESENT_INTERVAL_16MS,
        PRESENT_INTERVAL_25MS,
        PRESENT_INTERVAL_33MS,
    };

    const wchar_t* PRESENT_INTERVAL_STRINGS[] =
    {
        L"8.3ms (Requires 120hz)",
        L"16.6ms",
        L"25.0ms (Requires 120hz)",
        L"33.3ms",
    };

    enum BACK_BUFFER_COUNT
    {
        BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_1,
        BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_2,
        BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2,
        BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_3,
        BACK_BUFFER_COUNT_COUNT,
    };

    const wchar_t* BACK_BUFFER_COUNT_STRING[] = {
        L"Double Buffer w/ SetFrameIntervalX() PeriodInIntervals = 1, IntervalOffsetInMicroseconds = 1000",
        L"Double Buffer w/ SetFrameIntervalX() PeriodInIntervals = 2",
        L"Triple Buffer w/ SetFrameIntervalX() PeriodInIntervals = 2",
        L"Triple Buffer w/ SetFrameIntervalX() PeriodInIntervals = 3",
    };
}

void Sample::DrawString(const SimpleMath::Vector2& pos, FXMVECTOR color, const wchar_t* string, ...)
{
    wchar_t buffer[1024] = {};
    va_list va;
    va_start(va, string);
    vswprintf_s(buffer, string, va);
    va_end(va);

    auto commandList = m_deviceResources->GetCommandList();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"DrawString(): %s", buffer);

    m_font->DrawString(m_sprites.get(), buffer, pos, color, 0.0f, g_XMZero, 2.0f);

    PIXEndEvent(commandList);
}

void Sample::DrawString(const SimpleMath::Vector2& pos, const wchar_t* string, ...)
{
    wchar_t buffer[1024] = {};
    va_list va;
    va_start(va, string);
    vswprintf_s(buffer, string, va);
    va_end(va);

    auto commandList = m_deviceResources->GetCommandList();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"DrawString(): %s", buffer);

    m_font->DrawString(m_sprites.get(), buffer, pos, DirectX::Colors::LightGray, 0.0f, g_XMZero, 2.0f);

    PIXEndEvent(commandList);
}

// Game Logic Helper functions
void Sample::TargetState::GenerateHits()
{
    numHits = 1;
}

void Sample::TargetState::Reset()
{
    isHit = false;
    explosionState = 0;
    acceleration = Vector2(0.3f, 0.0);
    GenerateHits();
    movementRadius = ATG::GetRandomValue<float>(41.0f) - 1.f;

    amplitude = ATG::GetRandomValue<float>(47.0f) - 7.f;
    frequency = ATG::GetRandomValue<float>(4.0f) - 1.f;
    phase = ATG::GetRandomValue<float>(100.0f);
}

void Sample::TargetState::CheckCollision(PlayerState& player)
{
    // Player Information
    float width = player.spriteSize;
    float height = width * player.aspectRatio;

    // Target Information
    float t_width = spriteSize;
    float t_height = width * aspectRatio;

    if (player.position.x < position.x + t_width &&
        player.position.x + width > position.x    &&
        player.position.y < position.y + t_height &&
        player.position.y + height > position.y)
    {
        if (!isHit)
        {
            player.points += 50;
            numHits--;

            if (numHits == 0)
            {
                isHit = true;
            }
        }
    }
}

void Sample::TargetState::Update(PlayerState& player, float time, float screenWidth, float screenHeight)
{
    if (isHit)
    {
        if (explosionState == 4)
        {
            Reset();
            position.x = screenWidth + ATG::GetRandomValue<float>(1100.f) - 300.f;
            position.y = ATG::GetRandomValue<float>((screenHeight - (float)spriteSize * aspectRatio) / 2.0f);
        }
    }
    else
    {
        // Difficulty metric
        auto distance = (player.position - position).Length();

        // Right-Left Movement
        acceleration.x = 0.3f + 2 * (distance * (1 / (screenWidth + screenHeight)));
        targetSpeed = Vector2(-1.0f * (float)MAXSPEED * 0.6f, 0.0f);
        currentSpeed = ACCELERATION * targetSpeed + (1.0f - ACCELERATION) * currentSpeed;
        position += currentSpeed * time;

        // Sinusoidal Movement
        theta += time;
        position.y += amplitude * cos(frequency * theta + phase);

        // Circular Movements
        auto t = distance / (sqrt(screenWidth*screenWidth + screenHeight * screenHeight));
        auto r_radius = 0.5 * (t)+3.5 * (1 - t);
        // auto r_angularVelocity = 0.5 * ANGULARVELOCITY * (t) + 1.0f * ANGULARVELOCITY * (1-t);
        movementRadius = powf(2.0f, r_radius);
        theta += time;
        position.x += movementRadius * cos(theta);
    }

    // Once drone goes off-screen, reset
    if (position.x < (-1.0f * screenWidth))
    {
        Reset();
        position.x = screenWidth + ATG::GetRandomValue<float>(1100.f) - 300.f;
        position.y = id * screenHeight / 4 + (spriteSize * aspectRatio / 2.0f);
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_gpuHwConfig{},
    m_canRun120(false),
    m_settingsChanged(false),
    m_gpuWorkload(13.0f),
    m_settingsIndex(0),
    m_presetConfig(PRESET_CONFIGURATION_60FPS_NORMAL),
    m_presentInterval(PRESENT_INTERVAL_16MS),
    m_presentFlags(0),
    m_backbufferCount(BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2),
    m_playerFiring{},
    m_playerFiringHeld{},
    m_targetSpeed{},
    m_position{}
{
    // We will always create TRIPLE buffered
    // But limit to double buffered internally when configured to
    // To avoid destroying/recreating over and over again and the demo changes settings
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        3 /* TRIPLE BUFFERED */,
        DX::DeviceResources::c_Enable4K_UHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
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

    // Game State Initialization
    TargetState targets[4];

    auto widthf = m_deviceResources->GetScreenViewport().Width;
    auto heightf = m_deviceResources->GetScreenViewport().Height;

    auto increment = heightf / 4.0f;

    // Targets
    for (int i = 0; i < 4; i++)
    {
        targets[i].Reset();
        targets[i].id = i;
        targets[i].position.x = widthf + ATG::GetRandomValue<float>(1000.0f) - 200.f;
        targets[i].position.y = i * increment + (targets[i].spriteSize * targets[i].aspectRatio / 2.0f);
        m_spriteStates.push_back(std::make_shared<TargetState>(std::move(targets[i])));
    }

    // Player
    PlayerState player;
    player.position = Vector2(
        (widthf - (float)player.spriteSize) / 2.0f,
        (heightf - (float)player.spriteSize * player.aspectRatio) / 2.0f
    );
    player.targetSpeed = Vector2(0.0f, 0.0f);
    player.currentSpeed = Vector2(0.0f, 0.0f);
    m_spriteStates.push_back(std::make_shared<PlayerState>(std::move(player)));

    //Background
    m_playerFiring = false;
    m_playerFiringHeld = false;
    m_targetSpeed = -300.0f;
    m_position = 0.0f;

    // Create the wait compute shader RS & PSO
    auto device = m_deviceResources->GetD3DDevice();
    auto bytecode = DX::ReadData(L"WaitShader.cso");

    DX::ThrowIfFailed(device->CreateRootSignature(0, bytecode.data(), bytecode.size(), IID_GRAPHICS_PPV_ARGS(m_waitRootSignature.ReleaseAndGetAddressOf())));

    D3D12_COMPUTE_PIPELINE_STATE_DESC waitPsoDesc = {};
    waitPsoDesc.pRootSignature = m_waitRootSignature.Get();
    waitPsoDesc.CS = D3D12_SHADER_BYTECODE { bytecode.data(), bytecode.size() };

    DX::ThrowIfFailed(device->CreateComputePipelineState(&waitPsoDesc, IID_GRAPHICS_PPV_ARGS(m_waitPipelineState.ReleaseAndGetAddressOf())));

    device->GetGpuHardwareConfigurationX(&m_gpuHwConfig);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Wait for ORIGIN event");

    // For PresentX presentation loops, the wait for the origin event
    // should be just before input is processed.
    m_deviceResources->WaitForOrigin();

    PIXEndEvent();

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

void Sample::UpdateFrameIntervalAndBackBufferCount()
{
    UINT frameIntervalUs = D3D12XBOX_FRAME_INTERVAL_60_HZ;
#ifdef _GAMING_XBOX_SCARLETT
    if (m_presentInterval == PRESENT_INTERVAL_8MS && m_canRun120)
    {
        frameIntervalUs = D3D12XBOX_FRAME_INTERVAL_120_HZ;
    }
    if (m_presentInterval == PRESENT_INTERVAL_25MS)
    {
        frameIntervalUs = D3D12XBOX_FRAME_INTERVAL_40_HZ;
    }
#endif
    if (m_presentInterval == PRESENT_INTERVAL_33MS)
    {
        frameIntervalUs = D3D12XBOX_FRAME_INTERVAL_30_HZ;
    }

    UINT periodInIntervals;
    if (m_backbufferCount == BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_1)
    {
        periodInIntervals = 1u;
    }
    else if (m_backbufferCount == BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_2 || m_backbufferCount == BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2)
    {
        periodInIntervals = 2u;
    }
    else // if (m_backbufferCount == BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_3)
    {
        periodInIntervals = 3u;
    }

    UINT intervalOffsetInMicroseconds = 0u;
    if (m_backbufferCount == BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_1)
    {
        // Let's cut things even closer in low latency modes!
        intervalOffsetInMicroseconds = 1000u;
    }

    D3D12XBOX_FRAME_INTERVAL_FLAGS flags = D3D12XBOX_FRAME_INTERVAL_FLAG_NONE;
#ifdef _GAMING_XBOX_SCARLETT
    if (m_presetConfig == PRESET_CONFIGURATION_90FPS_WITH_60FPS_PLUS)
        flags = D3D12XBOX_FRAME_INTERVAL_FLAG_60FPS_PLUS;
#endif
    m_deviceResources->SetFrameIntervalX(frameIntervalUs, periodInIntervals, intervalOffsetInMicroseconds, flags);

    UINT backbufferCountLimiter = 0u;
    if (m_backbufferCount == BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_1 || m_backbufferCount == BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_2)
    {
        backbufferCountLimiter = 2u;
    }
    else // if (m_backbufferCount == BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2 || m_backbufferCount == BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_3)
    {
        backbufferCountLimiter = 3u;
    }

    m_deviceResources->SetBackBufferCountLimiter(backbufferCountLimiter);
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    m_deviceResources->Prepare();

    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());
    auto frameCount = timer.GetFrameCount();
    auto frameWidth = m_deviceResources->GetScreenViewport().Width;
    auto frameHeight = m_deviceResources->GetScreenViewport().Height;

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Sample input");

    auto pad = m_gamePad->GetState(GamePad::c_MostRecent);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    PIXEndEvent();

    // Settings
    m_settingsChanged = false;

    static bool firstCall = true;
    if (firstCall)
    {
        firstCall = false;
        m_settingsChanged = true;
    }

    if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::ButtonState::PRESSED)
    {
        m_settingsChanged = true;

        switch (m_settingsIndex) {
        case SETTINGS_PRESET_CONFIGURATION:
            m_presetConfig = m_presetConfig + 1;
            if (m_presetConfig >= PRESET_CONFIGURATION_COUNT) { m_presetConfig = 0; }
            break;
        case SETTINGS_PRESENT_FLAGS:
            m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
            break;
        case SETTINGS_GPU_WORKLOAD:
            m_gpuWorkload = std::min(50.0f, m_gpuWorkload + 0.5f);
            break;
        case SETTINGS_BACK_BUFFER_COUNT:
            m_backbufferCount = m_backbufferCount + 1;
            if (m_backbufferCount >= BACK_BUFFER_COUNT_COUNT) { m_backbufferCount = 0; }
            break;
        }
    }
    if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::ButtonState::PRESSED)
    {
        m_settingsChanged = true;

        switch (m_settingsIndex) {
        case SETTINGS_PRESET_CONFIGURATION:
            m_presetConfig = m_presetConfig - 1;
            if (m_presetConfig >= PRESET_CONFIGURATION_COUNT) { m_presetConfig = PRESET_CONFIGURATION_COUNT - 1; }
            break;
        case SETTINGS_PRESENT_FLAGS:
            m_presentFlags = PRESENT_FLAGS_NONE;
            break;
        case SETTINGS_GPU_WORKLOAD:
            m_gpuWorkload = std::max(1.0f, m_gpuWorkload - 0.5f);
            break;
        case SETTINGS_BACK_BUFFER_COUNT:
            m_backbufferCount = m_backbufferCount - 1;
            if (m_backbufferCount >= BACK_BUFFER_COUNT_COUNT) { m_backbufferCount = BACK_BUFFER_COUNT_COUNT - 1; }
            break;
        }
    }

    // Apply Presets
    switch (m_presetConfig)
    {
    case PRESET_CONFIGURATION_60FPS_NORMAL:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 13.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        if (m_settingsChanged)
        {
            IGameInputDevice* device;
            m_gamePad->GetDevice(-1, &device);
            if (device != nullptr)
            {
                device->SetInputSynchronizationState(false);
            }
        }
        break;
    case PRESET_CONFIGURATION_55FPS_STUTTER:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 18.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_55FPS_FREESYNC:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_NONE;
        m_gpuWorkload = 18.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_90FPS_WITH_60FPS_PLUS:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_NONE;
        m_gpuWorkload = 11.11f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_55FPS_120HZ:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 18.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_120FPS_NORMAL:
        m_presentInterval = PRESENT_INTERVAL_8MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 7.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_100FPS_FREESYNC:
        m_presentInterval = PRESENT_INTERVAL_8MS;
        m_presentFlags = PRESENT_FLAGS_NONE;
        m_gpuWorkload = 10.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_60FPS_NORMAL_PART_TWO:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 13.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_30FPS_NORMAL:
        m_presentInterval = PRESENT_INTERVAL_33MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 30.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_40FPS_NORMAL:
        m_presentInterval = PRESENT_INTERVAL_25MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 22.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_TRIPLE_BUFFER_PERIOD_3:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 13.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_3;
        break;
    case PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_2:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 13.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_2_MISSED_FRAME_RATE:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 20.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_TRIPLE_BUFFER_PERIOD_2:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 13.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        break;
    case PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_1:
        m_presentInterval = PRESENT_INTERVAL_16MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 12.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_1;
        break;
    case PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_1_120FPS:
        m_presentInterval = PRESENT_INTERVAL_8MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 4.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_DOUBLE_BUFFER_PERIOD_1;
        break;
#ifdef _GAMING_XBOX_SCARLETT
    case PRESET_CONFIGURATION_NO_DLI:
        m_presentInterval = PRESENT_INTERVAL_33MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 30.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        if (m_settingsChanged)
        {
            IGameInputDevice* device;
            m_gamePad->GetDevice(-1, &device);
            if (device != nullptr)
            {
                device->SetInputSynchronizationState(false);
            }
        }
        break;
    case PRESET_CONFIGURATION_DLI:
        m_presentInterval = PRESENT_INTERVAL_33MS;
        m_presentFlags = PRESENT_FLAGS_DISABLE_VRR;
        m_gpuWorkload = 30.0f;
        m_backbufferCount = BACK_BUFFER_COUNT_TRIPLE_BUFFER_PERIOD_2;
        if (m_settingsChanged)
        {
            IGameInputDevice* device;
            m_gamePad->GetDevice(-1, &device);
            if (device != nullptr)
            {
                device->SetInputSynchronizationState(true);
            }
        }
        break;
#endif
    }

    if (m_settingsChanged)
    {
        UpdateFrameIntervalAndBackBufferCount();
    }

    // Update Player Controls Based on Button Presses
    PlayerState* player = dynamic_cast<PlayerState *>(m_spriteStates[m_spriteStates.size() - 1].get());

    player->targetSpeed = Vector2(0.0f, 0.0f);

    player->targetSpeed.x = pad.thumbSticks.leftX * (float)MAXSPEED;
    player->targetSpeed.y = -pad.thumbSticks.leftY * (float)MAXSPEED;

    player->currentSpeed = ACCELERATION * player->targetSpeed + (1.0f - ACCELERATION) * player->currentSpeed;
    player->position += player->currentSpeed * elapsedTime;

    player->position.x = __min(__max(player->position.x, 0.0f), m_deviceResources->GetScreenViewport().Width);
    player->position.y = __min(__max(player->position.y, 0.0f), m_deviceResources->GetScreenViewport().Height);

    // Check Collision status of targets if X Button is pressed
    m_playerFiring = (m_gamePadButtons.x == GamePad::ButtonStateTracker::ButtonState::PRESSED || m_gamePadButtons.a == GamePad::ButtonStateTracker::ButtonState::PRESSED);
    m_playerFiringHeld = m_playerFiring || (m_gamePadButtons.x == GamePad::ButtonStateTracker::ButtonState::HELD || m_gamePadButtons.a == GamePad::ButtonStateTracker::ButtonState::HELD);

    if (m_playerFiring)
    {
        PIXSetMarker(PIX_COLOR_DEFAULT, "INPUT: X Button Pressed");

        // top left corner position
        for (int i = 0; i < 4; i++)
        {
            TargetState *target = dynamic_cast<TargetState *>(m_spriteStates[i].get());
            // AABB Collision detection with Drone and CrossHair
            target->CheckCollision(*player);
        }
    }

    // Update Explosion animation
    for (int i = 0; i < 4; i++)
    {
        TargetState *target = dynamic_cast<TargetState *>(m_spriteStates[i].get());
        target->Update(*player, elapsedTime, frameWidth, frameHeight);
        if (target->isHit)
        {
            // Change Fire animation after every 10 frames
            if (frameCount % 10 == 0)
            {
                target->explosionState++;
                target->explosionState %= 5;
            }
        }
    }

    // Endless Background animation
    m_position += m_targetSpeed * elapsedTime;
    m_position = round(m_position);

    if (m_position <= (-1.0f * m_deviceResources->GetScreenViewport().Width))
    {
        m_position = 0.0f;
    }
}

#pragma endregion
#pragma region Frame Render
bool Sample::CanRun120Hz()
{
    ComPtr<IDXGIDevice> dxgiDevice;
    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));

    ComPtr<IDXGIAdapter> dxgiAdapter;
    DX::ThrowIfFailed(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

    ComPtr<IDXGIOutput> dxgiOutput;
    DX::ThrowIfFailed(dxgiAdapter->EnumOutputs(0, &dxgiOutput));

    UINT numModes = 0;
    dxgiOutput->GetDisplayModeList(m_deviceResources->GetBackBufferFormat(), 0, &numModes, nullptr);

    if (numModes)
    {
        auto modes = std::make_unique<DXGI_MODE_DESC[]>(numModes);
        dxgiOutput->GetDisplayModeList(m_deviceResources->GetBackBufferFormat(), 0, &numModes, modes.get());

        for (UINT i = 0; i < numModes; i++)
        {
            if (modes[i].RefreshRate.Numerator == 120)
            {
                return true;
                break;
            }
        }
    }

    return false;
}

void Sample::HiccupTheGpu()
{
    // Simulate a rude 1 Frame Hiccup
    WaitShader(40.0f);
}

void Sample::WaitShader(float ms)
{
    static const UINT dispatches_per_ms[] = {
        10236, // Xbox One - (853MHz * 12 * 64 * 2) / (64 * 2 * 1000)
        10968, // Xbox One S - (914MHz * 12 * 64 * 2) / (64 * 2 * 1000)
        46880, // Xbox One X - (1172MHz * 40 * 64 * 2) / (64 * 2 * 1000)
        51568, // Xbox One X Devkit 44CU - (1172MHz * 44 * 64 * 2) / (64 * 2 * 1000)
#ifdef _GAMING_XBOX_SCARLETT
        31300, // Xbox Series S - (1565MHz * 20 * 64 * 2) / (64 * 2 * 1000)
        94900, // Xbox Series X - (1825MHz * 52 * 64 * 2) / (64 * 2 * 1000)
        94900, // Xbox Series X Devkit - (1825MHz * 52 * 64 * 2) / (64 * 2 * 1000)
#endif
    };
    static const UINT max_threadgroups_1d = 65536u;
    D3D12XBOX_HARDWARE_VERSION console = m_gpuHwConfig.HardwareVersion;
#ifndef _GAMING_XBOX_SCARLETT
    if (console == D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X_DEVKIT && m_gpuHwConfig.GpuCuCount == 40) {
        // xbconfig ConsoleMode = XboxOneXDevkit versus XboxOneXDevkit44
        console = D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X;
    }
#endif

    auto commandList = m_deviceResources->GetCommandList();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Wait Shader");
    commandList->SetGraphicsRootSignature(m_waitRootSignature.Get());
    commandList->SetPipelineState(m_waitPipelineState.Get());

    const UINT totalGroupCount = static_cast<UINT>(ms * dispatches_per_ms[console]);

    // Wait for GPU idle before starting wait shader waves
    commandList->FlushPipelineX(D3D12XBOX_FLUSH_IDLE, 0, 0xFFFFFFFF00ull); 

    // Dispatch one 2D workload to fulfill bulk of the busy work
    UINT countY = totalGroupCount / max_threadgroups_1d;
    if (countY > 0)
    {
        commandList->Dispatch(max_threadgroups_1d, countY, 1);
    }

    // ...and one 1D workload encompassing the remainder.
    UINT remainder = totalGroupCount % max_threadgroups_1d;
    commandList->Dispatch(remainder, 1, 1);

    PIXEndEvent(commandList);
}
#pragma endregion

void Sample::RenderUI()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    auto const size = m_deviceResources->GetOutputSize();

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right),
        UINT(size.bottom));

    float XOFFSET = float(safe.left);
    float YOFFSET = m_font->GetLineSpacing() * 1.5f;
    float y = float(safe.top) + (m_font->GetLineSpacing() * 5.f);

    DrawString(SimpleMath::Vector2(XOFFSET, y), L"Frame %i, FPS %i", m_timer.GetFrameCount(), m_timer.GetFramesPerSecond());

    y += YOFFSET * 2.0f;
    DrawString(SimpleMath::Vector2(XOFFSET, y), L"Demo Configuration: %s", PRESET_CONFIGURATION_STRING[m_presetConfig]);

    y += YOFFSET;
    DrawString(SimpleMath::Vector2(XOFFSET, y), DirectX::Colors::Green, L"Demo Description: %s", DEMO_DESCRIPTION_STRING[m_presetConfig]);

    y += YOFFSET * 2.0;
    DrawString(SimpleMath::Vector2(XOFFSET, y), L"Configuration Details:");

    y += YOFFSET;
    DrawString(SimpleMath::Vector2(XOFFSET, y), L"%s SetFrameIntervalX(): LengthInMicroseconds = %s", (m_settingsIndex == SETTINGS_PRESENT_INTERVAL) ? L">>" : L"  ", PRESENT_INTERVAL_STRINGS[m_presentInterval]);

    y += YOFFSET;
    DrawString(SimpleMath::Vector2(XOFFSET, y), L"%s Present() Flags: %s", (m_settingsIndex == SETTINGS_PRESENT_FLAGS) ? L">>" : L"  ", PRESENT_FLAGS_STRINGS[m_presentFlags]);

    y += YOFFSET;
    float workloadfps = 1000.0f / m_gpuWorkload;
    DrawString(SimpleMath::Vector2(XOFFSET, y), L"%s GPU Workload: %0.1f ms (%0.1f fps)", (m_settingsIndex == SETTINGS_GPU_WORKLOAD) ? L">>" : L"  ", m_gpuWorkload, workloadfps);

    y += YOFFSET;
    DrawString(SimpleMath::Vector2(XOFFSET, y), L"%s Back Buffer Count: %s", (m_settingsIndex == SETTINGS_BACK_BUFFER_COUNT) ? L">>" : L"  ", BACK_BUFFER_COUNT_STRING[m_backbufferCount]);

    if (!m_canRun120)
    {
        y += YOFFSET * 2.0;
        DrawString(SimpleMath::Vector2(XOFFSET, y), DirectX::Colors::Red, L"120HZ IS NOT SUPPORTED ON THIS DISPLAY");
    }

    bool latency_ui = false;

    switch (m_presetConfig)
    {
    case PRESET_CONFIGURATION_TRIPLE_BUFFER_PERIOD_3:
    case PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_2:
    case PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_2_MISSED_FRAME_RATE:
    case PRESET_CONFIGURATION_TRIPLE_BUFFER_PERIOD_2:
    case PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_1:
    case PRESET_CONFIGURATION_DOUBLE_BUFFER_PERIOD_1_120FPS:
        latency_ui = true;
        break;
    }

    UINT flip_gpu_latency_us = 0;
    UINT gpu_cpu_latency_us = 0;
    UINT total_latency_us = 0;
    HRESULT hr = GetFrameLatencyStatistics(flip_gpu_latency_us, gpu_cpu_latency_us, total_latency_us);

    latency_ui = (latency_ui) ? SUCCEEDED(hr) : latency_ui;

    if (latency_ui)
    {
        y += YOFFSET * 2.0;
        DrawString(SimpleMath::Vector2(XOFFSET, y), L"Latency:\n");

        y += YOFFSET;
        DrawString(SimpleMath::Vector2(XOFFSET, y), L"  CPU->GPU: %.1f ms\n", gpu_cpu_latency_us / 1000.0f);

        y += YOFFSET;
        DrawString(SimpleMath::Vector2(XOFFSET, y), L"  GPU->FLIP: %.1f ms\n", flip_gpu_latency_us / 1000.0f);

        y += YOFFSET;
        DrawString(SimpleMath::Vector2(XOFFSET, y), L"  Total CPU->FLIP: %.1f ms\n", total_latency_us / 1000.0f);

        y += YOFFSET * 2.0;
        DrawString(SimpleMath::Vector2(XOFFSET, y), L"CPU is the first CPU Draw call when WaitFrameEventX() returns\n");
        y += YOFFSET;
        DrawString(SimpleMath::Vector2(XOFFSET, y), L"GPU is the last GPU Draw call at the GPU Present\n");
        y += YOFFSET;
        DrawString(SimpleMath::Vector2(XOFFSET, y), L"FLIP is during the vblank when this frame begins HDMI Transmission\n");
    }

    DX::DrawControllerString(m_sprites.get(),
        m_legendFont.get(), m_ctrlFont.get(),
        L"[View] Exit   [DPad] Change settings  [B] Single long frame spike",
        XMFLOAT2(float(safe.left),
            float(safe.bottom) - m_legendFont->GetLineSpacing()),
        ATG::Colors::LightGrey);

    PIXEndEvent(commandList);
}

HRESULT Sample::GetFrameLatencyStatistics(UINT& flip_gpu_latency_us, UINT& gpu_cpu_latency_us, UINT& total_latency_us)
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Frame Statistics");

    D3D12XBOX_FRAME_STATISTICS eventFrameStatistics = {};
    D3D12XBOX_FRAME_STATISTICS presentFrameStatistics = {};
    D3D12XBOX_FRAME_STATISTICS displayFrameStatistics = {};

    HRESULT hr = S_OK;
    if (SUCCEEDED(hr))
    {
        UINT count = 1;
        hr = m_deviceResources->GetFrameStatisticsX(4, D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_EVENT, &count, &eventFrameStatistics);
    }

    if (SUCCEEDED(hr))
    {
        UINT count = 1;
        hr = m_deviceResources->GetFrameStatisticsX(4, D3D12XBOX_FRAME_STATISTICS_TYPE_PRESENT, &count, &presentFrameStatistics);
    }

    if (SUCCEEDED(hr))
    {
        UINT count = 1;
        hr = m_deviceResources->GetFrameStatisticsX(4, D3D12XBOX_FRAME_STATISTICS_TYPE_DISPLAY, &count, &displayFrameStatistics);
    }

    if (SUCCEEDED(hr))
    {
        LARGE_INTEGER qpf = {};
        QueryPerformanceFrequency(&qpf);

        UINT64 flip = displayFrameStatistics.Display.FlipTime;
        UINT64 gpu_present = presentFrameStatistics.Present.GPUProcessTime;
        UINT64 cpu_wait_return = eventFrameStatistics.Event.WaitReturnTime;

        flip_gpu_latency_us = ((flip - gpu_present) * 1000000) / qpf.QuadPart;
        gpu_cpu_latency_us = ((gpu_present - cpu_wait_return) * 1000000) / qpf.QuadPart;
        total_latency_us = ((flip - cpu_wait_return) * 1000000) / qpf.QuadPart;
    }

    PIXEndEvent(commandList);

    return hr;
}

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Set the descriptor heaps
    auto heaps = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heaps);

    // Draw sprites
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Draw Sprites");

    auto const viewportWidth = m_deviceResources->GetScreenViewport().Width;
    auto const viewPortHeight = m_deviceResources->GetScreenViewport().Height;

    //Background
    float background_seam = m_position + viewportWidth;

    m_sprites->Begin(commandList);
    m_sprites->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2((UINT)viewportWidth, (UINT)viewPortHeight),
        XMFLOAT2(m_position, 0.0f));
    m_sprites->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2((UINT)viewportWidth, (UINT)viewPortHeight),
        XMFLOAT2(background_seam, 0.0f));

    // Enemy Drones
    for (size_t i = 0; i < (m_spriteStates.size() - 1); i++)
    {
        TargetState *target = dynamic_cast<TargetState *>(m_spriteStates[i].get());

        if (target->isHit)
        {
            m_sprites->Draw(m_resourceDescriptors->GetGpuHandle(3 + target->explosionState), XMUINT2(m_spriteStates[i]->spriteSize,
                ((float)m_spriteStates[i]->spriteSize) * m_spriteStates[i]->aspectRatio),
                m_spriteStates[i]->position);
        }
        else
        {
            m_sprites->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Drone), XMUINT2(m_spriteStates[i]->spriteSize,
                ((float)m_spriteStates[i]->spriteSize) * m_spriteStates[i]->aspectRatio),
                m_spriteStates[i]->position);
        }
    }

    // Crosshair
    const DirectX::XMVECTOR crosshairColor = (m_playerFiringHeld || m_playerFiring) ? DirectX::Colors::Red : DirectX::Colors::White;
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, (m_playerFiring) ? L"Crosshair: X Button Pressed" : L"Crosshair");
    m_sprites->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::CrossHair),
        XMUINT2(m_spriteStates[m_spriteStates.size() - 1]->spriteSize,
        ((float)m_spriteStates[m_spriteStates.size() - 1]->spriteSize) * m_spriteStates[m_spriteStates.size() - 1]->aspectRatio),
        m_spriteStates[m_spriteStates.size() - 1]->position, crosshairColor);
    PIXEndEvent(commandList); // "Crosshair color"

    // Red Line
    m_sprites->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Line), XMUINT2(5, (UINT)viewPortHeight),
        XMFLOAT2(background_seam, 0.0f), DirectX::Colors::Red);

    PIXEndEvent(commandList); // "Draw sprite"

    // Score
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Draw Points");

    auto points = std::dynamic_pointer_cast<PlayerState>(m_spriteStates[m_spriteStates.size() - 1])->points;
    char points_s[12/*-2147483648\0*/] = {};

    _itoa_s((int)points, points_s, 10);
    std::string message = std::string("POINTS: ") + points_s;
    auto message_cstr = message.c_str();

    auto size = strlen(message_cstr) + 1;
    wchar_t* message_wct = new wchar_t[size];
    size_t convertedChars = 0;

    mbstowcs_s(&convertedChars, message_wct, size, message_cstr, _TRUNCATE);

    auto const osize = m_deviceResources->GetOutputSize();

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(osize.right),
        UINT(osize.bottom));

    m_font->DrawString(m_sprites.get(), message_wct, XMFLOAT2(safe.left, safe.top), Colors::Yellow, 0, XMFLOAT2(0.0f, 0.0f), 5.0f);

    PIXEndEvent(commandList); // "Draw Points"

    RenderUI();
    m_sprites->End();

    // Wait
    if (m_gamePadButtons.b == GamePad::ButtonStateTracker::ButtonState::PRESSED)
    {
        HiccupTheGpu();
    }

    WaitShader(m_gpuWorkload);

    PIXEndEvent(commandList); // "Render"

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");

    D3D12XBOX_PRESENT_PARAMETERS presentParamters = {};
    presentParamters.Flags = (m_presentFlags == PRESENT_FLAGS_DISABLE_VRR) ? D3D12XBOX_PRESENT_FLAG_DISABLE_VARIABLE_REFRESH_RATE : D3D12XBOX_PRESENT_FLAG_NONE;

    m_deviceResources->Present(D3D12_RESOURCE_STATE_RENDER_TARGET, &presentParamters);
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent(); // "Present"
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
    m_gamePadButtons.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        Descriptors::Count);

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

    {
        ResourceUploadBatch resourceUpload(device);

        resourceUpload.Begin();

        const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/CrossHair.png", m_texture1.ReleaseAndGetAddressOf(), true)
        );

        CreateShaderResourceView(device, m_texture1.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::CrossHair));

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/Drone.png", m_texture2.ReleaseAndGetAddressOf(), true)
        );

        CreateShaderResourceView(device, m_texture2.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Drone));

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/Background.png", m_backgroundTexture.ReleaseAndGetAddressOf(), true)
        );

        CreateShaderResourceView(device, m_backgroundTexture.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/Explosion1.png", m_textureE1.ReleaseAndGetAddressOf(), true)
        );

        CreateShaderResourceView(device, m_textureE1.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Explosion1));

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/Explosion2.png", m_textureE2.ReleaseAndGetAddressOf(), true)
        );

        CreateShaderResourceView(device, m_textureE2.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Explosion2));

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/Explosion3.png", m_textureE3.ReleaseAndGetAddressOf(), true)
        );

        CreateShaderResourceView(device, m_textureE3.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Explosion3));

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/Explosion4.png", m_textureE4.ReleaseAndGetAddressOf(), true)
        );

        CreateShaderResourceView(device, m_textureE4.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Explosion4));

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/Explosion5.png", m_textureE5.ReleaseAndGetAddressOf(), true)
        );

        CreateShaderResourceView(device, m_textureE5.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Explosion5));

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/Line.png", m_textureLine.ReleaseAndGetAddressOf(), true)
        );

        CreateShaderResourceView(device, m_textureLine.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Line));

        {
            SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::NonPremultiplied);

            m_sprites = std::make_unique<SpriteBatch>(device, resourceUpload, pd);
        }

        auto const size = m_deviceResources->GetOutputSize();

        m_font = std::make_unique<SpriteFont>(device, resourceUpload,
            L"SegoeUI_18.spritefont",
            m_resourceDescriptors->GetCpuHandle(Descriptors::SegoeFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::SegoeFont));

        m_legendFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1440) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
            m_resourceDescriptors->GetCpuHandle(Descriptors::LegendFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::LegendFont));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1440) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
            m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));

        // Upload the resources to the GPU.
        auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());

        // Wait for the upload thread to terminate
        uploadResourcesFinished.wait();
    }

    m_canRun120 = CanRun120Hz();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const viewport = m_deviceResources->GetScreenViewport();
    m_sprites->SetViewport(viewport);
}
#pragma endregion
