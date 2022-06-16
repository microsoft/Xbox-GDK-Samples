//--------------------------------------------------------------------------------------
// MeshletCull.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "DrawFrustum.h"
#include "DrawCullData.h"

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
    void UpdateConstants(ID3D12GraphicsCommandList* commandList);
    void DrawHUD(ID3D12GraphicsCommandList* commandList);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    DirectX::XMVECTOR GetSamplePoint() const;
    void Pick();

private:
    enum RenderMode
    {
        RM_Flat = 0,
        RM_Meshlets,
        RM_Count
    };

    struct MeshletModel
    {
        std::unique_ptr<DirectX::Model>     Model;
        std::vector<ATG::MeshletSet>        MeshletData;
    };

    struct SceneObject
    {
        using EffectList = DirectX::Model::EffectCollection;

        VQS                                     World;
        uint32_t                                ModelIndex;
        Microsoft::WRL::ComPtr<ID3D12Resource>  ConstantBuffer;
        EffectList                              Effects;
    };

private:
    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    uint32_t                                        m_displayWidth;
    uint32_t                                        m_displayHeight;

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
    std::unique_ptr<DirectX::CommonStates>          m_commonStates;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_msPso;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_constants;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_crosshair;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_countsUAV;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_countsReadback;
    uint32_t*                                       m_countsData;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;

    std::vector<MeshletModel>                       m_models;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;

    std::vector<SceneObject>                        m_scene;
    DX::OrbitCamera                                 m_camera;
    DX::OrbitCamera                                 m_debugCamera;

    ATG::DrawFrustumEffect                          m_frustumDraw;
    ATG::DrawCullDataEffect                         m_cullDataDraw;

    // Sample variables
    bool                                            m_renderHelp;
    bool                                            m_cull;
    bool                                            m_debugCull;
    RenderMode                                      m_renderMode;
    uint32_t                                        m_lodIndex;
    uint32_t                                        m_highlightedIndex;
    uint32_t                                        m_selectedIndex;
};
