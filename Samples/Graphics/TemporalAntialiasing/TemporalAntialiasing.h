//--------------------------------------------------------------------------------------
// TemporalAntialiasing.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SharedDataTypes.h"

class GameObject;

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

    // Initialization and management.
    void Initialize(HWND window, int width, int height);

    // Basic render loop.
    void Tick();

    // IDeviceNotify.
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages.
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties.
    void GetDefaultSize(int& width, int& height) const noexcept;
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void DrawHUD();
    void RenderMagnifyingGlassRegion(ID3D12GraphicsCommandList* commandList, ID3D12Resource* zoomTarget);

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void CreateModelFromPath(ID3D12Device* device, DirectX::ResourceUploadBatch& upload, std::unique_ptr<DirectX::EffectTextureFactory>& texFactory,
        std::wstring const& meshName, std::unique_ptr<DirectX::Model>& outModel);

    uint32_t LogBase2(unsigned long mask);
    void InitializeGameObjectsPositions();

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    uint32_t                                        m_displayWidth;
    uint32_t                                        m_displayHeight;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;
    float                                           m_gpuTimerMeasuresMS[TIMER_COUNT];

    // Sample settings.
    bool                                            m_useTAA;
    bool                                            m_performSharpenPass;
    bool                                            m_hideZoomRegion;
    DilationMode                                    m_dilationMode;
    ClipMode                                        m_clipMode;
    FilterMode                                      m_filterMode;
    float                                           m_samplingBias;
    float                                           m_jitterScale;
    JitterQRSequence                                m_jitterSequence;
    uint32_t                                        m_currentHistoryBufferIndex;

    // Jitter related variables.
    DirectX::SimpleMath::Vector2                    m_currentJitter;
    DirectX::SimpleMath::Vector2                    m_previousJitter;
    float                                           m_JitterDeltaX;
    float                                           m_JitterDeltaY;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    std::unique_ptr<DirectX::Keyboard>              m_keyboard;
    std::unique_ptr<DirectX::Mouse>                 m_mouse;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker         m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>        m_uploadedAssetsDescHeap;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_texFactory;

    // Models and their transforms.
    std::unique_ptr<DirectX::Model>                 m_sceneModel;
    DirectX::SimpleMath::Matrix                     m_sceneWorldMatrix;
    DirectX::SimpleMath::Matrix                     m_sceneNormalMatrix;

    // ATGTK objects
    DX::FlyCamera                                   m_sceneCamera;

    // Dynamic objects.
    std::vector<GameObject*>                        m_gameobjects;

    // Matrices.
    DirectX::SimpleMath::Matrix                     m_jitteredProjMatrix;
    DirectX::SimpleMath::Matrix                     m_previousFrameView;
    DirectX::SimpleMath::Matrix                     m_previousFrameProj;

    // PSO and RS for temporal resolve pass.
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_temporalResolvePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_temporalResolveRS;

    // Descriptor heaps (shader visible and rendertargets).
    std::unique_ptr<DirectX::DescriptorHeap>        m_shaderVisibleDescHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_gBufferAndColorRTVDescHeap;

    // GBuffer resources, PSO and RS.
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_gbufferMotionResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_gbufferAlbedoResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_gbufferNormalsResource;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_rtvHandleAlbedo;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_rtvHandleNormals;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_rtvHandleMotion;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_gbufferPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_gbufferRS;

    // Optional sharpening pass.
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_sharpenPassPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_sharpenPassRS;

    // Magnifying glass pass.
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_magnifyingGlassPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_magnifyingGlassRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_ZoomRegionPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_ZoomRegionRS;

    // Fullscreen pass PSO and RS.
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_fullscreenPassPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_fullscreenPassRS;

    // Targets for the different steps in the sample.
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_colorBufferResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_intermediateCopyResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_historyBufferResource[2];

    // HUD related objects.
    std::unique_ptr<DirectX::DescriptorHeap>        m_HUDDescriptorHeap;
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
};
