//--------------------------------------------------------------------------------------
// ComputeParticles.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "ParticleSystem.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    // Properties
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:
    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

private:
    // Represents an instance of a scene object.
    struct ObjectInstance
    {
        using EffectList = std::vector<std::shared_ptr<DirectX::IEffect>>;

        DirectX::XMFLOAT4X4 World;
        DirectX::Model*     Model;
        EffectList          Effects;
    };

    enum DescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_Count
    };

private:
    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    int                                             m_displayWidth;
    int                                             m_displayHeight;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::CommonStates>          m_commonStates;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;

    // Scene transform data
    DirectX::SimpleMath::Matrix                     m_proj;

    // Scene meshes.
    std::vector<std::unique_ptr<DirectX::Model>>    m_models;
    std::vector<ObjectInstance>                     m_scene;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;

    ATG::ParticleSystem                             m_particles;

    // Scene update/animation data
    float                                           m_orbitRadius;
    float                                           m_orbitAngle;
    float                                           m_cameraHeight;

    // Framerate info.
    int                                             m_frameCounter;
    float                                           m_frameTime;
    float                                           m_frameRate;
};