//--------------------------------------------------------------------------------------
// SimplePBR.h
// 
// Demonstrates PBRModel and PBREffect in DirectX 12 on Xbox One, Scarlett and PC devices
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "OrbitCamera.h"
#include "PBRModel.h"
#include "RenderTexture.h"
#include "Skybox/Skybox.h"
#include "StepTimer.h"

using VertexType = DirectX::VertexPositionColor;
using DebugVert = DirectX::VertexPositionColor;

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

    // Basic Sample loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderHUD(ID3D12GraphicsCommandList* commandList);

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                    m_deviceResources;

    // Rendering loop timer.
    uint64_t                                                m_frame;
    DX::StepTimer                                           m_timer;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>                m_graphicsMemory;

    // Hud
    std::unique_ptr<DirectX::SpriteBatch>                   m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>                    m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>                    m_ctrlFont;

    // UI Rectangles to improve readability of Sample font.
    std::unique_ptr<DirectX::BasicEffect>                   m_basicEffect;
    std::unique_ptr<DirectX::PrimitiveBatch<VertexType>>    m_vertexBatch;

    // Input and Camera
    std::unique_ptr<DirectX::GamePad>                       m_gamePad;
    DirectX::GamePad::ButtonStateTracker                    m_gamePadButtons;
    std::unique_ptr<DX::OrbitCamera>                        m_camera;
    bool                                                    m_gamepadConnected;

#ifdef _GAMING_DESKTOP
    // Additional Input for PC
    std::unique_ptr<DirectX::Keyboard>                      m_keyboard;
    DirectX::Keyboard::KeyboardStateTracker                 m_keyboardButtons;
    std::unique_ptr<DirectX::Mouse>                         m_mouse;
#endif

    // Render states
    std::unique_ptr<DirectX::CommonStates>                  m_commonStates;

    // All SRV descriptors for sample
    std::unique_ptr<DirectX::DescriptorPile>                m_srvPile;

    enum class StaticDescriptors
    {
        Font = 0,
        CtrlFont = 1,
        SceneTex = 2,
        RadianceTex = 3,
        IrradianceTex = 4,
        Reserve = 5
    };

    std::unique_ptr<DirectX::SpriteBatch>                   m_spriteBatch;
    std::unique_ptr<DirectX::ToneMapPostProcess>            m_toneMap;
    std::unique_ptr<DirectX::ToneMapPostProcess>            m_HDR10;

    // Render target view for tonemapping
    std::unique_ptr<DX::RenderTexture>                      m_hdrScene;
    std::unique_ptr<DirectX::DescriptorHeap>                m_rtvHeap;

    // Sky/Environment textures
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_radianceTexture;

    // Irradiance texture
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_irradianceTexture;

    // Model
    std::vector< std::unique_ptr<ATG::PBRModel>>            m_pbrModels;

    // Skybox
    std::unique_ptr<DX::Skybox>                             m_skybox;
};
