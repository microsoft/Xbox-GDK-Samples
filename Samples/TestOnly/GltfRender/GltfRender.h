//--------------------------------------------------------------------------------------
// GltfRender.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

struct Scene;


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
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

#ifndef _GAMING_XBOX_XBOXONE
    void BuildTLASFromGLTF();
    void BuildBLASesFromGLTF();
#endif

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                m_deviceResources;

    // Rendering loop timer.
    uint64_t                                            m_frame;
    DX::StepTimer                                       m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>                   m_gamePad;
    std::unique_ptr<DirectX::Keyboard>                  m_keyboard;
    std::unique_ptr<DirectX::Mouse>                     m_mouse;

    DirectX::GamePad::ButtonStateTracker                m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker             m_keyboardButtons;

    // D3D12 Objects
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_meshInfoBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_sceneConstants;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_rootSignatureRT;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_pipelineStateRT;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_rootSignatureRaster;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_pipelineStateRaster;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_rtOutput;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TLASBuildScratch;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TLAS;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>            m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>            m_srvHeap;
    std::unique_ptr<DirectX::DescriptorHeap>            m_samplerHeap;
    std::unique_ptr<DirectX::EffectTextureFactory>      m_textureFactory;

    // Camera
    DX::FlyCamera                                       m_camera;

    // Scene
    std::vector <std::unique_ptr<Scene>>                m_scenes;

    // Options
    bool                                                m_buildTLAS;
    bool                                                m_buildBLAS;
    bool                                                m_useRaytracing;
    bool                                                m_supportsRaytracing;
};
