//--------------------------------------------------------------------------------------
// HWDisplayPlaneCompositing.h
//
// Xbox Series consoles provide two display planes for titles. One example of using them is to composite UI onto the final scene.
// This is done in the display hardware without any performance or latency costs to the title.
//
// The hardware display planes can be set up to use different resolutions, refresh rates, a mix of SDR and HDR, using different
// color spaces and different gamma spaces. For example, UI could be rendered at 4K 30Hz in SDR with the sRGB gamma curve, while
// the scene could be rendered at 1440p 60Hz in HDR in linear space.
// 
// Note that HDR compositing is only supported in the March 2024 GDK and onwards. When compositing in HDR, the system level
// auto tone mapping will be used to generate an SDR image for GameDVR. HDR compositing is done using 200 nits as paper white
// for the UI. I.e. a value of 1.0 in UI will be displayed at 200 nits. Scene values in the display plane with the scene
// will have to be normalized as usual, therefore a value of 1.0 in the scene display plane will be displayed at 10,000 nits.
//
// The code snippet to see how to setup the display planes for compositing can be found in the file DeviceResources.cpp, when
// looking at the setup for:
// 
//      D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters[2] = {};
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
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
    void RenderScene(ID3D12GraphicsCommandList* commandList);
    void RenderUI(ID3D12GraphicsCommandList* commandList);

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::DescriptorPile>    m_rtvPile;
    std::unique_ptr<DirectX::DescriptorPile>    m_srvPile;
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_textFont;
    std::unique_ptr<DirectX::SpriteFont>        m_controllerFont;

    std::unique_ptr<DX::FullScreenQuad>         m_fullScreenQuad;

    // D3D12 objects
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_d3dHDRScenePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_d3dSDRScenePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_d3dRenderUITexturePSO;

    std::unique_ptr<DX::HDRImage>               m_hdrSceneImage;
    std::unique_ptr<DX::Texture>                m_uiTexture;

};
