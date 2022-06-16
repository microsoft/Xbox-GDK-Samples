//--------------------------------------------------------------------------------------
// SimpleMeshlet.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "DrawFrustum.h"

struct MeshletModel
{
    std::unique_ptr<DirectX::Model> Model;
    std::vector<ATG::MeshletSet>    MeshletData;
};

struct SceneObject
{
    using EffectList = DirectX::Model::EffectCollection;

    DirectX::SimpleMath::Matrix             World;
    uint32_t                                ModelIndex;
    Microsoft::WRL::ComPtr<ID3D12Resource>  ConstantBuffer;
    EffectList                              Effects;
};


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
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:
    void Update(DX::StepTimer const& timer);
    void Render();

    void UpdateConstants(ID3D12GraphicsCommandList* commandList);
    void Clear();
    void DrawHUD(ID3D12GraphicsCommandList* commandList);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    
private:
    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    int                                             m_displayWidth;
    int                                             m_displayHeight;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    std::unique_ptr<DirectX::Keyboard>              m_keyboard;
    std::unique_ptr<DirectX::Mouse>                 m_mouse;

    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker         m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::CommonStates>          m_commonStates;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;

    // D3D pipelines & resources
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_basicMeshletPso;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_cullMeshletPso;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_viewCB;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;

    // Scene resources
    std::vector<MeshletModel>                       m_models;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;

    // Scene objects
    std::vector<SceneObject>                        m_scene;
    DX::OrbitCamera                                 m_camera;
    DX::OrbitCamera                                 m_debugCamera;
    ATG::DrawFrustumEffect                          m_frustumDraw;

    // Sample variables
    bool                                            m_cull;
    bool                                            m_debugCamCull;
    bool                                            m_drawMeshlets;
    uint32_t                                        m_lodIndex;
};
