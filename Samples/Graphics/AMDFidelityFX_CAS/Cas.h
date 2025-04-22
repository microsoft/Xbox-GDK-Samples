//--------------------------------------------------------------------------------------
// Cas.h
//
// Contrast Adaptive Sharpening (CAS)
//
// Modifications Copyright (C) 2020. Advanced Micro Devices, Inc. All Rights Reserved.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "MSAAHelper.h"
#include "RenderTexture.h"
#include "FullScreenQuad\FullScreenQuad.h"

#include <FidelityFX/host/ffx_cas.h>

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

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

    void GetDefaultSize(int& width, int& height) const noexcept;

    // thread-safe constant buffer allocator to pass to FidelityFX backend contexts for allocations
    static FfxConstantAllocation ffxAllocateConstantBuffer(void* data, FfxUInt64 dataSize);
private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void UpdateFfxCasContext(bool enabled);

    void Clear();

    void RenderScene(ID3D12GraphicsCommandList* commandList);
    void RenderZoom(ID3D12GraphicsCommandList* commandList);
    void RenderUI(ID3D12GraphicsCommandList* commandList);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptors;
    std::unique_ptr<DirectX::DescriptorHeap>        m_renderDescriptors;
    std::unique_ptr<DirectX::CommonStates>          m_states;

    enum Descriptors
    {
        SceneTex,
        Font,
        CtrlFont,
        ColorCtrlFont,
        CAS_CB,
        CAS_InputSRV,
        CAS_OutputUAV,
        CAS_OutputSRV,
        Count
    };

    enum RTDescriptors
    {
        SceneRTV,
        RTCount
    };

    // Controls
    float                                           m_pitch;
    float                                           m_yaw;

    DirectX::SimpleMath::Matrix                     m_proj;
    DirectX::SimpleMath::Matrix                     m_view;

    // Scene rendering
    std::unique_ptr<DirectX::EffectFactory>         m_fxFactory;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_modelResources;
    std::unique_ptr<DirectX::Model>                 m_model;
    DirectX::Model::EffectCollection                m_modelEffect;

    // Render Target resources
    std::unique_ptr<DX::RenderTexture>              m_scene;

    // Zoom view
    D3D12_VIEWPORT                                  m_zoomViewport;
    D3D12_VIEWPORT                                  m_zoomViewportCas;
    D3D12_RECT                                      m_zoomRect;
    D3D12_RECT                                      m_zoomRectCas;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_zoomPSO;
    std::unique_ptr<DirectX::BasicEffect>           m_lineEFfect;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_lines;

    // UI rendering
    std::unique_ptr<DirectX::SpriteBatch>           m_batch;
    std::unique_ptr<DirectX::SpriteFont>            m_font;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
    std::unique_ptr<DirectX::SpriteFont>            m_colorCtrlFont;

    // Post-processing
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_passthroughPSO;

    // CAS Context members
    FfxCasContextDescription m_InitializationParameters = {};
    FfxCasContext            m_CasContext;

    enum class CASMode: unsigned int
    {
        Sharpen = 0,
#ifdef _GAMING_XBOX_SCARLETT
        Sharpen_Wave32,
#endif 
        Disabled,
        Count
    };

    enum class UpscaleMode: unsigned int
    {
        NativeRender = 0,
        Upscaled,
        Count
    };

    // CAS
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_casPSOs[2][(unsigned int)CASMode::Count];
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_computeRootSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_casOutput;
    D3D12_RESOURCE_STATES                           m_casOutputState;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;
    CASMode                                         m_casMode;
    float                                           m_sharpness;
    bool                                            m_renderScaleChanged;
    unsigned int                                    m_renderScale; // ==0 implies native
    const unsigned int                              m_numRenderScales = 4;
    const float                                     m_renderScales[4] = { 1.0f, 0.8f, 0.66f, 0.5f };

    const wchar_t* GetCasModeDescription();

    Microsoft::WRL::ComPtr<ID3D12PipelineState>& GetCasPipelineState(UpscaleMode upscaleMode, CASMode casMode)
    { 
        return m_casPSOs[(unsigned int)upscaleMode][(unsigned int)casMode];
    }

    unsigned int GetRenderScale() const
    {
        if (m_casMode == CASMode::Disabled)
        {
            return (unsigned int)UpscaleMode::NativeRender;
        }
        return m_renderScale;
    }

    UpscaleMode GetUpscaleMode() const
    {
        if (m_casMode == CASMode::Disabled)
        {
            return UpscaleMode::NativeRender;
        }
        else if (m_renderScale != (unsigned int)UpscaleMode::NativeRender)
        {
            return UpscaleMode::Upscaled;
        }

        return UpscaleMode::NativeRender;
    }

    void TransitionCasUAVTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES afterState);
};
