//--------------------------------------------------------------------------------------
// HDRPrecision.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "HDRImage.h"
#include "RenderTexture.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

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
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderImage();
    void RenderRamp();
    void RenderUI(ID3D12GraphicsCommandList* commandList);
    void PrepareSwapBuffer(ID3D12GraphicsCommandList* commandList);

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void CreateFormatDependantResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    uint32_t                                    m_displayWidth;
    uint32_t                                    m_displayHeight;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // UI
    std::unique_ptr<DirectX::DescriptorPile>    m_rtvPile;
    std::unique_ptr<DirectX::DescriptorPile>    m_srvPile;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DX::FullScreenQuad>         m_fullScreenQuad;
    std::unique_ptr<DX::RenderTexture>          m_hdrScene;
    std::unique_ptr<DirectX::CommonStates>      m_states;

    // D3D12 objects
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_renderHDRImagePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_prepareSwapBufferRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_prepareSwapBufferPSO;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_renderTexture;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_d3dRampRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_d3dRampPSO;

    std::unique_ptr<DirectX::DescriptorHeap>    m_rtvDescriptorHeap;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptorHeap;

    bool    m_bIsDisplayInHDRMode;
    bool    m_bRenderRamp;
    int     m_currentColorSpace;
    int     m_currentGammaCurve;
    int     m_currentBackBufferFormat;
    int     m_currentNitsValuesIndex;

    static constexpr int g_NumNitsValues = 10;
    const int g_NitsValues[g_NumNitsValues] = { 1, 5, 10, 80, 100, 200, 1000, 2000, 4000, 10000 };

    // HDR images	
    static constexpr int m_NumImages = 2;
    int                  m_currentHDRImage;
    DX::HDRImage         m_HDRImage[m_NumImages];
    const wchar_t*       m_HDRImageFiles[m_NumImages] =
    {
        L"HDR_029_Sky_Cloudy_Ref.hdr",
        L"graffiti_shelter_2k.hdr"
    };

    // RTVs
    enum RTVDescriptors
    {
        HDRSceneRTV,
        NumRTVs
    };

    // SRV desriptors for m_resourceDescriptorHeap
    enum ResourceDescriptors
    {
        HDRScene,
        TextFont,
        ControllerFont,
        HDRTexture,
        NumSRVs = HDRTexture + m_NumImages
    };
};
