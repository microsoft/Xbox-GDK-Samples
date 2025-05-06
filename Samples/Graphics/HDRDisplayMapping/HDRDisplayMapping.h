//--------------------------------------------------------------------------------------
// HDRDisplayMapping.h
//
// This sample shows that even when rendering a HDR scene on a HDR capable TV, some tone mapping is still needed, referred to as "HDR display mapping".
// HDR display mapping maps values that are brighter than what a HDR TV can display, into the upper brightness range of the TV's capabilities, so that
// details in the very bright areas of the scene won't get clipped.
//
// The sample implements the following visualizations
//  -HDR display mapping
//  -HDR to SDR tone mapping
//  -Highlight values brighter than the TV max brightness, i.e. those that will naturally be clipped by TV
//
// Note, these shaders are not optimized, the goal is simply to explore different methods of HDR display mappings
//
// Refer to the white paper "HDR Display Mapping", http://aka.ms/hdr-display-mapping 
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "DeviceResources.h"
#include "StepTimer.h"
#include "HDR\HDRImage.h"

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
    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void InitializeSpriteFonts(ID3D12Device* d3dDevice, DirectX::ResourceUploadBatch& resourceUpload, const DirectX::RenderTargetState& rtState);
	void DrawStringWithShadow(const wchar_t* string, DirectX::SimpleMath::Vector2& fontPos, DirectX::FXMVECTOR color, float fontScale);

#pragma region Standard Sample Defines

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
    std::unique_ptr<DirectX::SpriteFont>        m_textFont;
    std::unique_ptr<DirectX::SpriteFont>        m_controllerFont;
    std::unique_ptr<DirectX::SpriteBatch>       m_fontBatch;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::CommonStates>      m_states;
    std::unique_ptr<DirectX::BasicEffect>       m_lineEffect;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_primitiveBatch;


#pragma endregion

#pragma region HDR Display Mapping Defines

    void SetDisplayMode(XDisplayHdrModeInfo* hdrData = nullptr);

    enum class TonemappingMode
    {
        None = 0,                                           // HDR values above 1.0 is simply clipped
        Reinhard = 1,                                       // Simple Reinhard tone mapping
        NumModes
    };

    enum class DisplayMappingMode
    {
        None = 0,                                           // No display mapping, values brighter than max TV brightness will naturally be clipped
        NoClipping = 1,                                     // Map HDR values brighter than max TV brightness using a 3 point Bezier curve, using a soft shoulder, TV max brightness and max HDR scene brightness
        SomeClipping = 2,                                   // Same as NoClipping, but allow for some clipping to retain brightness, a compromise between brightness and details
        NumModes
    };

    TonemappingMode     m_tonemappingMode;
    DisplayMappingMode  m_displayMappingMode;
    
    // Constant buffer data for shader
    struct DisplayMappingData
    {
        // Display mapping data
        float PaperWhiteNits;                               // Defining how bright paper white is, can be used to control the overall brightness of the scene
        float SoftShoulderStart2084;                        // Threshold at beginning of soft shoulder for HDR display mapping, normalized non-linear ST.2084 value
        float MaxBrightnessOfTV2084;                        // Max perceived brightness of TV, nits converted to normalized non-linear ST.2084 value 
        float MaxBrightnessOfHDRScene2084;                  // Max perceived brightness of HDR scene, nits converted to normalized non-linear ST.2084 value 

        // Visualization data
        int   RenderAsSDR;                                  // Display the HDR image as if it were rendered on a SDR TV
        int   TonemappingMode;                              // Which HDR to SDR tone mapping method to use
        int   DisplayMappingMode;                           // Which HDR display mapping method to use
        int   IndicateValuesBrighterThanTV;                 // Highlight which values are brighter than TV max brightness, i.e. those that will naturally be clipped by your eye
        float MaxBrightnessOfTV2084ForVisualization;        // Visualization needs to know the actual max nits and the max nits used with clipping
    };

    DisplayMappingData m_displayMappingData;

    void UpdateDisplayMappingData();

    float   m_maxBrightnessOfTV;                            // The current max brightness perceived by consumer, user adjusts this value
    float   m_maxBrightnessReturnedBySystemCalibration;     // The max brightness returned from the Xbox system HDRGameCalibration app
    float   m_currentPaperWhiteNits;                        // Current brightness for paper white
    
    float   m_softShoulderStartNits;                        // Where the soft shoulder starts
    float   m_nitsAllowedToBeClipped;                       // Compromise between brightness and details, so allow for some clipping to retain brightness

    bool    m_bRenderAsSDR;                                 // Display the HDR image as if it were rendered on a SDR TV
    bool    m_bIndicateValuesBrighterThanTV;                // Highlight which values are brighter than TV max brightness, i.e. those that will naturally be clipped by your eye

    float ApplyHDRDisplayMapping(float normalizedLinearValue);

#pragma endregion

#pragma region HDR Defines

    // Output HDR10
    bool m_bIsTVInHDRMode;                                  // This will be set to TRUE if the attached display is in HDR mode
    void PrepareSwapChainBuffers();                         // Takes as input the HDR scene values and outputs an HDR and SDR signal to two seperate swapchains
#pragma endregion

#pragma region HDR defines to render UI and HDR scene

    // Other HDR defines, mainly for UI and displaying graphs on screen
    bool    m_bRenderGraphs;                               // Render curves / graphs
    bool    m_bCalibrateTV;                                // Find max brightness of TV
   
	// Rendering the HDR scene
    std::unique_ptr<DX::RenderTexture>  m_hdrScene;

    void RenderCalibrationBlock(int startX, int startY, int size, float nits);
    void RenderHDRScene();
    void RenderGraphs();
    void RenderSDRGraphs(const int x, const float viewportWidth, const float viewportHeight);
    void RenderReinhardTonemap(const int x, const float viewportWidth, const float viewportHeight);
    void RenderHDRGraphs(const int x, const float viewportWidth, const float viewportHeight);
    void Render2084Curve(const int x, const float viewportWidth, const float viewportHeight);
    void RenderHDRDisplayMapping(const int x, const float viewportWidth, const float viewportHeight);
    void RenderUI();

#pragma endregion

#pragma region Load .HDR image
    static constexpr int NumImages = 5;
    int                  m_currentHDRImage;
    DX::HDRImage         m_HDRImage[NumImages];
    const wchar_t*       m_HDRImageFiles[NumImages] =
    {
        L"HDR_029_Sky_Cloudy_Ref.hdr",
        L"georgentor_2k.hdr",
        L"HDR_112_River_Road_2_Ref.hdr",
        L"blender_institute_2k.hdr",
        L"graffiti_shelter_2k.hdr",
    };

#pragma endregion

#pragma region D3D12 Defines

    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_d3dRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_d3dRenderHDRImagePSO; 
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_d3dPrepareSwapChainBufferPSO;
    std::unique_ptr<DirectX::DescriptorHeap>        m_rtvDescriptorHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorHeap;

    // Descriptors for m_rtvDescriptorHeap 
    enum RTVDescriptors
    {
        HDRSceneRTV,
        CountRTV
    };

    // Desriptors for m_resourceDescriptorHeap
    enum ResourceDescriptors
    {
        HDRScene,
        TextFont,
        ControllerFont,
        HDRTexture,
        Count = HDRTexture + NumImages
    };

#pragma endregion

};

