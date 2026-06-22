//--------------------------------------------------------------------------------------
// HDRReconstruction.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "DeviceResources.h"
#include "StepTimer.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{

#pragma region HDR Reconstruction

    void SetDisplayMode();

    // Represents the final tonemapped SDR backbuffer of a game. We simulate this by rendering an SDR tonemapped texture into this buffer. This could even be a photo.
    // Note that these images are 8-bit compressed, so you might see compression artifacts and banding in the sample, which is not caused by the HDR reconstruction technique
    std::unique_ptr<DX::RenderTexture>  m_sdrScene;
    void RenderSDRScene();

    // Constant buffer data for pixel shader
    struct HDRData
    {
        // Values will be reconstructed as follows:
        //      SDR             HDR
        //      0.0     ->      0 nits
        //      0.5     ->      Paper white nits, e.g. 200 nits
        //      1.0     ->      MaxReconstructedNits, e.g. 1000 nits

        // HDR reconstruction constants
        float   MaxReconstructedNits;               // How bright (in nits) should the tonemapped SDR value of 1.0 be
        float   ReconstructedColorSaturation;       // Lerping between per luma and per color channel reconstruction, where 0 = only use per luma reconstruction, 1 = use only per color channel reconstruction
        int     bUseGamutExpansion;                 // When TRUE, use a custom color space that is slightly bigger than Rec.709 to produce more saturated colors, if FALSE, just use Rec.709
        int     bApplyReconstruction;               // Toggle between SDR/HDR

        // HDR10 constants
        float   DisplayGamma;                       // Simple pow() adjustment for display gamma / contrast
        float   PaperWhiteNits;                     // Defines how bright white is (in nits), which controls how bright the SDR range in the image will be

    } m_HDRData;

    void UpdateHDRData();
    void ReconstructHDRAndConvertToHDR10();         // Reconstruct the final SDR image to HDR and convert to HDR10

    float m_UIBrightnessScale;                      // In this sample the UI will be rendered on top of the final tonemapped SDR image, i.e. white text will be the value of (1.0f, 1.0f, 1.0f), which
                                                    // as a tonemapped value, is just as bright as a tonemapped sun. Because the value of (1.0f, 1.0f, 1.0f) is reconstructed to the max nits, e.g. 1000 nits,
                                                    // the white UI text will be much too bright and fatiguing to the consumer. We therefore calculate a linear scale to dim down the UI rendering so
                                                    // that when reconstructed, white will be g_PaperWhiteNitsUI.
#pragma endregion

#pragma region SDR Textures

    static constexpr int            c_NumImages = 8;
    int                             m_currentSDRTexture;
    std::unique_ptr<DX::Texture>    m_sdrTexture[c_NumImages];
    std::atomic_bool                m_sdrTextureFinishedLoading[c_NumImages];
    const wchar_t*                  m_sdrTextureFiles[c_NumImages] =
    {
        L"GOW4_1.DDS",
        L"GOW4_2.DDS",
        L"GOW4_3.DDS",
        L"GOW4_4.DDS",
        L"Halo_1.DDS",
        L"Halo_2.DDS",
        L"Halo_3.DDS",
        L"Halo_4.DDS",
    };

#pragma endregion

#pragma region D3D12 Defines

    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_d3dRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_d3dRenderSDRTexturePSO; 
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_d3dReconstructHDRAndConvertToHDR10PSO;
    std::unique_ptr<DirectX::DescriptorHeap>        m_rtvDescriptorHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorHeap;
    
    // Descriptors for m_rtvDescriptorHeap 
    enum RTVDescriptors
    {
        SDRSceneRTV,
        CountRTV
    };

    // Desriptors for m_resourceDescriptorHeap
    enum ResourceDescriptors
    {
        SDRScene,
        TextFont,
        ControllerFont,
        SDRTexture,
        Count = SDRTexture + c_NumImages
    };

#pragma endregion

#pragma region Standard Sample Defines

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

    bool                                        m_bIsDisplayInHDRMode;
    int                                         m_savedUseGamutExpansion;

#pragma endregion

};
