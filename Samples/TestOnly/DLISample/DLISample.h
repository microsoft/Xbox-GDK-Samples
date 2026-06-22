//--------------------------------------------------------------------------------------
// DLISample.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                               m_timer;
    DX::StepTimer								m_renderTimer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::SpriteFont>		m_font;
    std::unique_ptr<DirectX::CommonStates>      m_states;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::BasicEffect>       m_batchEffect;
    Microsoft::WRL::ComPtr<ID3D12Resource>		m_background;

    std::list<double>		m_timeList;
    float                   m_queryRate;
    float                   m_queryVariance;
    bool                    m_updateRate;
    bool                    m_holdLS;
    bool                    m_holdRS;

    enum Descriptors
    {
        TextFont,
        Background,
        Count,
    };
};
