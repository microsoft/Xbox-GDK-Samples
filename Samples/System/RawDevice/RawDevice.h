//--------------------------------------------------------------------------------------
// RawDevice.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include <GameInput.h>
#include "RawWheelLibrary/RawWheelLibrary.h"

constexpr size_t c_maxDevices = 4;

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    std::vector<Xbox::RawWheel>                 m_devices;
    std::mutex                                  m_deviceLock;
    Microsoft::WRL::ComPtr<IGameInput>          m_gameInput;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    //Input states
    Microsoft::WRL::ComPtr<IGameInputReading>   m_reading;
    GameInputCallbackToken                      m_deviceToken = {};

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;
    float                                       m_scale;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum Descriptors
    {
        PrintFont,
        ControllerFont,
        Background,
        Count,
    };
};
