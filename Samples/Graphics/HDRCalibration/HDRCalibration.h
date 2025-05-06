//--------------------------------------------------------------------------------------
// HDRCalibration.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "DeviceResources.h"
#include "StepTimer.h"
#include "HDRImage.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{

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
    void Clear();

    void SetDisplayMode(XDisplayHdrModeInfo* hdrData = nullptr);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void InitializeSpriteFonts(ID3D12Device* d3dDevice, DirectX::ResourceUploadBatch& resourceUpload, const DirectX::RenderTargetState& rtState);
	void DrawStringWithShadow(const wchar_t* string, DirectX::SimpleMath::Vector2& fontPos, DirectX::FXMVECTOR color, float fontScale);
    void RenderRedCircle(int x, int y, int width, int height);

    // Standard sample defines
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
    std::unique_ptr<DirectX::SpriteBatch>       m_colorBlockBatch;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DX::FullScreenQuad>         m_fullScreenQuad;
    std::unique_ptr<DX::Texture>                m_redCircleTexture;

#pragma endregion

#pragma region HDR Calibration

    bool m_bUseObjectiveCalibration;    // Display test patterns to objectively calibrate, otherwise just adjust the image until it looks great
    bool m_bShowCalibrationScreen;      // Toggle to show/hide the calibration option screens
    bool m_bApplyCalibration;           // Toggle to show the original or calibrated image

    enum class CalibrationScreen
    {
        MaxHDRBrightness = 0,   // Controls how much detail is visible in the bright areas of the image, details above what the TV can display will be clipped
        Contrast = 1,           // Controls how much detail is visible in the dark areas of the image, e.g. details in shadows
        Brightness = 2,         // Controls overall brightness of image, useful to adjust for entry level TVs and different viewing environments, e.g. playing during night/day time
        Color = 3,              // Controls the extra color saturation intruduced from color gamut expansion, useful to control when TV also does color gamut expansion
        NumScreens
    };
    CalibrationScreen m_calibrationScreen;

    struct CalibrationData
    {
        float   MaxHDRBrightness;       // Max perceived brightness of TV, i.e. the max value of the TV's own tone mapper
        float   DisplayGamma;           // Simple pow() adjustment for display gamma / contrast
        float   PaperWhiteNits;         // Paper white nits
        float   ColorGamutExpansion;    // Lerp between Rec.709 and expanded color gamut
    };
    CalibrationData m_calibrationData;
    CalibrationData m_savedCalibrationData;

    // Constant buffer data for pixel shader
    struct CalibrationDataCB
    {       
        float   PaperWhiteNits;                             // Defines how bright white is (in nits), which controls how bright the SDR range in the image will be
        float   DisplayGamma;                               // Simple pow() adjustment for display gamma / contrast
        float   ColorGamutExpansion;                        // Lerp between Rec.709 and expanded color gamut
        float   SoftShoulderStart2084;                      // Threshold at beginning of soft shoulder for HDR display mapping, normalized non-linear ST.2084 value
        float   MaxBrightnessOfTV2084;                      // Max perceived brightness of TV, nits converted to normalized non-linear ST.2084 value 
        float   MaxBrightnessOfHDRScene2084;                // Max perceived brightness of HDR scene, nits converted to normalized non-linear ST.2084 value 

        // When determining the max preceived brightness of the TV, we shouldn't apply display gamma or display mapping adjustments. These define the bounding box of where
        // that calibration image is rendered. E.g. when trying to establish the max perceived brightness of the TV with a high contrast setting, you will end up with the wrong value
        float   CalibrationImageBoundingBoxU1;              // Bounding box for calibration screens
        float   CalibrationImageBoundingBoxV1;
        float   CalibrationImageBoundingBoxU2;
        float   CalibrationImageBoundingBoxV2;
        float   PaperWhiteNitsForCalibrationScreen;         // Paperwhite nits to use within the calibration screens bounding box
        int     ApplyContrastToCalibrationScreen;           // Contrast adjustment should not be applied when determining the max perceived brightness of the TV
    };

    CalibrationDataCB   m_calibrationDataCB;

    // Calibration data determined by the Xbox system HDRGameCalibration app. Consumers can access the app from the console Settings, "Calibrate HDR for games" option
    XDisplayHdrModeInfo m_calibrationDataFromSystem;

    void UpdateCalibrationData();
 
#pragma endregion

#pragma region HDR Defines                                                                                                  
    bool                                m_bIsDisplayInHDRMode;          // This will be set to TRUE if the attached display is in HDR mode
    std::unique_ptr<DX::RenderTexture>  m_hdrScene;

    void RenderCalibrationBlock(int startX, int startY, int width, int height, float innerValue, float outerValue, bool valueAsNits, float paperWhiteNits);
    void RenderHDRScene();
    void RenderCalibrationScreen();
    void RenderObjectiveCalibrationScreen();
    void RenderSubjectiveCalibrationScreen();
    void RenderUI();
    void PrepareSwapChainBuffers();                         // Takes as input the HDR scene values and outputs an HDR and SDR signal to two seperate swapchains

    inline DirectX::XMVECTOR MakeColor(float value) { DirectX::XMVECTORF32 color = { value, value, value, 1.0f }; return color; }

    double m_flashContrastTimer;                            // Timer to control the flashing of the center block of the contrast calibration screen

#pragma endregion


#pragma region Load .HDR image

    static constexpr int NumImages = 1;
    int                  m_currentHDRImage;
    DX::HDRImage         m_HDRImage[NumImages];
    const wchar_t*       m_HDRImageFiles[NumImages] =
    {
        L"Assets\\FH3.hdr",
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
		RedCircleTexture,
        HDRTexture,
        Count = HDRTexture + NumImages
    };

#pragma endregion
};
