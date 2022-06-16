//--------------------------------------------------------------------------------------
// DynamicLOD.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Shared.h"

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
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

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
    void DrawHUD(ID3D12GraphicsCommandList* commandList);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void UpdateConstants(ID3D12GraphicsCommandList* commandList);
    void RegenerateInstances();

private:
    enum InstanceMode
    {
        IM_Line,
        IM_Circles,
        IM_Cube,
        IM_Count
    };

    enum RenderMode
    {
        RM_Flat,
        RM_Meshlets,
        RM_LodLevel,
        RM_Count
    };

    struct MeshletModel
    {
        std::unique_ptr<DirectX::Model> Model;
        std::vector<ATG::MeshletSet>    MeshletData;
    };

private:
    // Device resources.
    std::unique_ptr<DX::DeviceResources>           m_deviceResources;
    int                                            m_displayWidth;
    int                                            m_displayHeight;

    // Rendering loop timer.
    uint64_t                                       m_frame;
    DX::StepTimer                                  m_timer;
    std::unique_ptr<DX::GPUTimer>                  m_gpuTimer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>              m_gamePad;
    std::unique_ptr<DirectX::Keyboard>             m_keyboard;
    std::unique_ptr<DirectX::Mouse>                m_mouse;

    DirectX::GamePad::ButtonStateTracker           m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker        m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>       m_graphicsMemory;
    std::unique_ptr<ATG::Help>                     m_controlHelp;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>    m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_msPso;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_instanceBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_whiteTexture;

    // Resources to read back selected LOD counts from the GPU
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_lodCountsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_lodCountsReadback;
    uint32_t*                                      m_lodCounts;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>          m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>           m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>           m_ctrlFont;
                                                   
    DX::FlyCamera                                  m_camera;
    std::vector<MeshletModel>                      m_lods;
    std::unique_ptr<DirectX::DescriptorPile>       m_srvPile;
                                                   
    std::vector<Instance>                          m_instances;
    uint32_t                                       m_primCounts[MAX_LOD_LEVELS];
    bool                                           m_uploadInstances;
                                                   
    // App settings                                
    InstanceMode                                   m_instMode;
    RenderMode                                     m_renderMode;
    uint32_t                                       m_instanceLevel;
    bool                                           m_forceVisible;
    bool                                           m_forceLod0;
    bool                                           m_renderHelp;
};
