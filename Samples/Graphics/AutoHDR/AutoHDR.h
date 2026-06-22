//--------------------------------------------------------------------------------------
// AutoHDR.h
//
// Auto HDR is a Scarlett feature that can visually enhance an SDR title by automatically
// adding HDR to it at a system level. The feature uses Scarlett hardware, so there is no
// performance cost to the title or system. I.e. no extra CPU or GPU, no extra memory or
// bandwidth, and no extra latency is added. The feature is applied to the majority of
// back compat ERA titles, but it can also be used by native Scarlett GDK titles. This
// sample shows how GDK titles can use Auto HDR as its HDR implementation.
//
// 1) Title renders as SDR
// 2) Use high precision swap buffer, e.g. DXGI_FORMAT_R9G9B9E5_SHAREDEXP
// 3) Switch TV to HDR mode
// 4) Create D3D device with D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR
// 5) Switch TV to HDR after resume/unconstrained
// 6) Optionally, but recommended, dim down the UI brightness when in HDR mode
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "FullScreenQuad.h"
#include "Texture.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
{
    void RenderSDRScene();

    // Dim down the UI so that it's not overbright at 1000 nits
    bool                            m_bAdjustUIBrighness;
    bool                            m_bIsTVInHDRMode;
    float                           m_UIBrightnessScale;

    // Load SDR game images
    static constexpr int            c_NumImages = 8;
    int                             m_currentSDRTexture;
    std::unique_ptr<DX::Texture>    m_sdrTexture[c_NumImages];
    std::atomic_bool                m_sdrTextureFinishedLoading[c_NumImages];
    const wchar_t*                  m_sdrTextureFiles[c_NumImages] =
    {
        L"GOW4_1.DDS", L"GOW4_2.DDS", L"GOW4_3.DDS", L"GOW4_4.DDS",
        L"Halo_1.DDS", L"Halo_2.DDS", L"Halo_3.DDS", L"Halo_4.DDS",
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_d3dRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_d3dRenderSDRTexturePSO; 
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorHeap;
    
    // Desriptors for m_resourceDescriptorHeap
    enum ResourceDescriptors
    {
        TextFont,
        ControllerFont,
        SDRTexture,
        Count = SDRTexture + c_NumImages
    };

public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained();
    void OnUnConstrained();

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderUI();
    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void SetDisplayMode();
    void InitializeSpriteFonts(ID3D12Device* d3dDevice, DirectX::ResourceUploadBatch& resourceUpload, const DirectX::RenderTargetState& rtState);
    void DrawStringWithShadow(const wchar_t* string, DirectX::SimpleMath::Vector2& fontPos, DirectX::FXMVECTOR color, float fontScale);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::SpriteFont>        m_textFont;
    std::unique_ptr<DirectX::SpriteFont>        m_controllerFont;
    std::unique_ptr<DirectX::SpriteBatch>       m_fontBatch;
    std::unique_ptr<DX::FullScreenQuad>         m_fullScreenQuad;
};
