//--------------------------------------------------------------------------------------
// MemoryStatistics.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "DescriptorHeap.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    static constexpr unsigned                   c_MaxTeapots = 1000;

private:

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;


    // Hold data from XMemGetWorkingSetStatistics
    XMEM_WORKING_SET_STATISTICS                 m_frameXMemStats[5];
    XMEM_WORKING_SET_STATISTICS                 m_preRunXMemStats;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;


    wchar_t                                     m_temporaryTextBuffer[1024];
    float                                       m_temporaryTextTime;

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void CreateNewTeapot();
    void DestroyTeapot();
    float FloatRand(float lowerBound, float upperBound);
    void PercentageStats();

    struct TeapotData
    {
        TeapotData() {}
        TeapotData(const TeapotData& other) :
            m_location(other.m_location),
            m_lifeFrameCount(other.m_lifeFrameCount)
        {
        }

        std::unique_ptr<DirectX::GeometricPrimitive>    m_teapot;
        DirectX::SimpleMath::Matrix                     m_location;
        unsigned int                                    m_lifeFrameCount = 0;
    };

    DirectX::SimpleMath::Matrix                 m_projection;
    DirectX::SimpleMath::Matrix                 m_world;
    DirectX::SimpleMath::Matrix                 m_view;
    DirectX::SimpleMath::Vector3                m_eye;
    DirectX::SimpleMath::Vector3                m_at;
    std::vector<TeapotData>                     m_teapots;

    std::default_random_engine                  m_randomEngine;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;


    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    bool                                        m_gamepadPresent;
    std::unique_ptr<DirectX::BasicEffect>       m_effect;


    enum Descriptors
    {
        Font,
        ControllerFont,
        Count,
    };

    enum XMemQueries
    {
        Title,
        Tools,
        Stack,
        Kernel,
        All,
    };
};
