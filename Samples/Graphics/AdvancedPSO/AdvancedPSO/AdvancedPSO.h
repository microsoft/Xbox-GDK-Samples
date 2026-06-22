//--------------------------------------------------------------------------------------
// AdvancedPSO.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "Scene.h"
#include "..\Shared\PsoSet.h"

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
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

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

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
        
    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // Descriptors
    enum Descriptors
    {
        TitleFont,
        UiFont,
        DefaultTexture,
        Count
    };
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // Shape texture
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_texture;

    // Fonts
    std::unique_ptr<DirectX::SpriteFont>        m_titleFont;
    std::unique_ptr<DirectX::SpriteFont>        m_UiFont;
    std::unique_ptr<DirectX::SpriteBatch>       m_fontBatch;
    
    // Shapes
    ATG::Scene                                  m_base;
    ATG::Scene                                  m_deserialized;
    ATG::Scene                                  m_derived;

    // Deduplicated PSOs
    std::unique_ptr<ATG::PsoSet>                m_psoSet;   // when this goes out of scope, all PSOs are released

    // Stats
    std::wstring                                m_deserializedPsoStatString;
    std::wstring                                m_derivedPsoStatString;
    std::wstring                                m_createPsoStatString;
};
