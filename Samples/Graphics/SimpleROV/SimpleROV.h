//--------------------------------------------------------------------------------------
// SimpleROV.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Shaders/SharedDefinitions.h"

class SampleObject;

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
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void DrawHUD();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void CreatePipelineStateObjects(ID3D12Device* device);

    // These methods will create the resources and views used in both custom blend passes.
    void CreateBlendMLABResourcesAndViews();
    void CreateBlendPPLLResourcesAndViews();

    // Sort the geometry w.r.t camera's distance for fixed HW blend.
    void SortGeometryByViewZ();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;

    // Sample configuration variables.
    BlendMode                                       m_blendMode;
    TranslucentChoice                               m_translucentChoice;
    float                                           m_gpuTimerMeasuresMS[TIMER_PASS_COUNT];
    uint32_t                                        m_MLABSpaceRequired;
    uint32_t                                        m_PPLLSpaceRequired;
    uint32_t                                        m_screenWidth;
    uint32_t                                        m_screenHeight;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;
    std::unique_ptr<DirectX::Keyboard>              m_keyboard;
    DirectX::Keyboard::KeyboardStateTracker         m_keyboardButtons;
    std::unique_ptr<DirectX::Mouse>                 m_mouse;

    // Opaque pass PSO and RS.
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_opaquePassPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_opaquePassRS;

    // Traditional blend pass PSO.
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_regularBlendPassPSO;

    // PSO and RS for custom blend techniques (PPLL and MLAB).
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_PPLLTranslucencyPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_MLABTranslucencyPassPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_CustomBlendRS;

    // Translucent pass PSO and RS.
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_translucentPassPSO;

    // Composite pass PSO and RS.
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_compositePassPPLL_PSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_compositePassMLAB_PSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_compositePassRS;

    // UAVs (ROVs) for programmable blending pass.
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_PPLLHeadPointer;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_perPixelLinkedListBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_PPLLBufferCounter;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_intermediateRT;

    // UAVs (ROVs) for programmable blending pass.
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_MLABClearMaskRes;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_MLABNodeListRes; 

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>        m_RTVDescHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_HUDDescriptorHeap;
    std::unique_ptr<DirectX::DescriptorPile>        m_ShaderVisibleHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_NonShaderVisibleHeap;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_texFactory;
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFontBold;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;

    // Scene model and transforms.
    SampleObject*                                   m_OpaqueObjs[OPAQUE_COUNT];
    SampleObject*                                   m_translucentObjs[MODEL_COUNT][TRANSLUCENT_COUNT];

    // Camera
    DX::FlyCamera                                   m_camera;
};
