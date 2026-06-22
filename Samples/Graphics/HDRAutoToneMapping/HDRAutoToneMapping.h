//--------------------------------------------------------------------------------------
// HDRAutoToneMapping.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "DeviceResources.h"
#include "StepTimer.h"
#include "FullScreenQuad.h"
#include "HDRImage.h"
#include "RenderTexture.h"
#include "Render3DTexture.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
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

#pragma region Auto Tone Mapping
#ifdef _GAMING_XBOX_SCARLETT
#else
    // The tone mapper used by the driver during auto tone mapping
    enum AutoToneMapMethod
    {
        Default,        // Driver's default auto tone mapper
        Reinhard,       // SetHDRToneMapper with a Reinhard LUT
        Filmic,         // SetHDRToneMapper with a Filmic LUT
        NumAutoToneMapMethods
    };

    AutoToneMapMethod m_currentAutoToneMapMethod;

    const WCHAR* m_AutoToneMapMethodStrings[NumAutoToneMapMethods] =
    {
        L"Default auto tone mapper",
        L"Auto tone map with Reinhard LUT",
        L"Auto tone map with Filmmic LUT"
    };

    const WCHAR* m_AutoToneMapMethodFileNames[NumAutoToneMapMethods] =
    {
        nullptr,
        L"HDRToneMapLUT_Reinhard.dds",
        L"HDRToneMapLUT_Filmic.dds"
    };

    // Tone mapper LUTs. The LUT has to be of size 32x32x32 with format 10:10:10:2
    static constexpr uint32_t m_LUTSize = 32;
    static constexpr DXGI_FORMAT m_LUTFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
    Microsoft::WRL::ComPtr<ID3D12Resource>  m_d3dToneMapLUT[NumAutoToneMapMethods];

    void LoadToneMapLUT(AutoToneMapMethod method);
    void SaveToneMapLUT(AutoToneMapMethod method);
    void RenderToneMapperLUT(AutoToneMapMethod method);

    // This sample shows how to save/load a tone mapper LUT as a DDS file, but also how to render a LUT at runtime
    static const AutoToneMapMethod m_ToneMapperToLoad = AutoToneMapMethod::Reinhard;
    static const AutoToneMapMethod m_ToneMapperToRender = AutoToneMapMethod::Filmic;
#endif

#pragma endregion

    bool m_bIsTVInHDRMode;

    void RenderHDRScene();
    void RenderUI();
    void ConvertToHDR10();

    void Update(DX::StepTimer const& timer);
    void Render();
    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void InitializeSpriteFonts(ID3D12Device* d3dDevice, DirectX::ResourceUploadBatch& resourceUpload, const DirectX::RenderTargetState& rtState);
	void DrawStringWithShadow(const wchar_t* string, DirectX::SimpleMath::Vector2& fontPos, DirectX::FXMVECTOR color, float fontScale);
    void SetDisplayMode();

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
    std::unique_ptr<DX::FullScreenQuad>         m_fullScreenQuad;
    std::unique_ptr<DX::RenderTexture>          m_hdrScene;
    std::unique_ptr<Render3DTexture>            m_render3DTexture;
    std::unique_ptr<DirectX::SpriteFont>        m_textFont;
    std::unique_ptr<DirectX::SpriteFont>        m_controllerFont;
    std::unique_ptr<DirectX::SpriteBatch>       m_fontBatch;
    std::unique_ptr<DirectX::CommonStates>      m_states;

#pragma region Load .HDR image
    static constexpr int m_NumImages = 2;
    int                 m_currentHDRImage;
    DX::HDRImage        m_HDRImage[m_NumImages];
    const wchar_t*      m_HDRImageFiles[m_NumImages] =
    {
        L"HDR_029_Sky_Cloudy_Ref.hdr",
        L"graffiti_shelter_2k.hdr",
    };
#pragma endregion

#pragma region D3D12 Defines
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_d3dRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_d3dRenderHDRImagePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_d3dRender3DTexturePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_d3dConvertToHDR10PSO;
    std::unique_ptr<DirectX::DescriptorHeap>    m_rtvDescriptorHeap;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptorHeap;

    // RTV descriptors for m_rtvDescriptorHeap
#ifdef _GAMING_XBOX_SCARLETT
    enum RTVDescriptors
    {
        HDRSceneRTV,
        NumRTVs
    };
#else
    enum RTVDescriptors
    {
        HDRSceneRTV,
        ToneMapLUTRTV,
        NumRTVs = ToneMapLUTRTV + NumAutoToneMapMethods
    };
#endif

#ifdef _GAMING_XBOX_SCARLETT
    // SRV desriptors for m_resourceDescriptorHeap
    enum ResourceDescriptors
    {
        HDRScene,
        TextFont,
        ControllerFont,
        HDRTexture,
        NumSRVs = HDRTexture + m_NumImages
    };
#else
    // SRV desriptors for m_resourceDescriptorHeap
    enum ResourceDescriptors
    {
        HDRScene,
        TextFont,
        ControllerFont,
        HDRTexture, 
        ToneMapLUT = HDRTexture + m_NumImages,
        NumSRVs = ToneMapLUT + NumAutoToneMapMethods 
    };
#endif
#pragma endregion

};
