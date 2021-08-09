//--------------------------------------------------------------------------------------
// MeshletInstancing.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Shared.h"

struct MeshletModel
{
    std::unique_ptr<DirectX::Model> Model;
    std::vector<ATG::MeshletSet>    MeshletData;
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:
    Sample() noexcept(false);
    ~Sample();

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
    void GetDefaultSize(int& width, int& height) const;

private:
    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();
    void UpdateConstants(ID3D12GraphicsCommandList* commandList);
    void DrawHUD(ID3D12GraphicsCommandList* commandList);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void RegenerateInstances();

private:
    enum RenderMode
    {
        RM_Flat = 0,
        RM_Meshlets,
        RM_Count
    };

    enum InstanceMode
    {
        IM_Line,
        IM_Circles,
        IM_Cube,
        IM_Count
    };

private:
    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    int                                             m_displayWidth;
    int                                             m_displayHeight;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;
    std::unique_ptr<ATG::Help>                      m_controlHelp;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    std::unique_ptr<DirectX::Keyboard>              m_keyboard;
    std::unique_ptr<DirectX::Mouse>                 m_mouse;

    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker         m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_meshShaderPSO;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_instanceBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_whiteTexture;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;

    std::vector<MeshletModel>                       m_lods;
    std::vector<uint32_t>                           m_primCounts;

    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;

    DX::FlyCamera                                   m_camera;

    std::vector<Instance>                           m_instances;
    InstanceMode                                    m_instMode;
    RenderMode                                      m_renderMode;
    uint32_t                                        m_lodIndex;
    int                                             m_instanceLevel;
    bool                                            m_updateInstances;
    bool                                            m_renderHelp;
};
