//--------------------------------------------------------------------------------------
// HDRCalibration.cpp
//
// HDR TVs can vary wildly with regards to HDR capabilities which effect image quality.Entry level TVs can struggle to physically reproduce HDR, the peak brightness
// might not be great, it might use whole panel illumination instead of local illumination, etc.To compensate for these differences, games should allow the consumer
// to calibrate the game's HDR image.This sample switches an HDR TV to HDR mode and then presents the user with several calibration options to adjust the visible detail
// in brights and darks, the overall brightness and color saturation of the image.Two different modes are available, subjective calibration where the user adjusts the
// image until it looks good, and objective calibration where test patterns are shown to determine the TV's capabilities.
//
// Detail in Brights - HDR values above what the TV can display will get clipped, therefore losing details in the brights.We need to determine the maximum perceived
// brightness of the TV so that the very bright HDR values in the scene can be mapped to maximum brightness of the TV.The calibration image renders a value into the
// scene which represents 10, 000 nits, the maximum brightness of the ST.2084 spec.A smaller image is then rendered with the current brightness.The user adjusts the
// current brightness value until he can no longer distinguish between the two images.For example, if the TV can display 1, 000 nits, then values rendered into the
// scene representing brighter pixels than 1, 000 nits will still be perceived as 1000 nits.Refer to the HDRDisplayMapping sample.
//
// Detail in Darks - Most games have a video option in a menu to determine display gamma.This can be used to correct contrast using a power function in a pixel shader,
// in order to see the correct amount of detail in the darks.This adjustment is very important when the TV is in HDR mode.Without the proper adjustment, the HDR image
// will looking "flat" and dull compared to the SDR image.It is important to render the calibration screen in the center of the screen, because the viewing angle on some
// LCD TVs can change the contrast of the image even from a small angle.For example, if rendering the calibration image on the side of an LCD screen, the determined
// display gamma value will be slightly different from when rendering in the center of the screen.Using a flashing image works well for this calibration, because it's
// easier for the user to determine when the center calibration block becomes invisible.Note that applying some contrast adjustment will change the maximum HDR scene
// value, which is important to do effective HDR display mapping, therefore the contrast adjustment function also need to be applied to the maximum scene value going
// into HDR display mapping.
//
// Overall Brightness - Entry level HDR TVs do not have a great peak brightness.To be able to show HDR, it must sacrifice the brightness of the SDR range of the image,
// which can make the image look worse when compared to an SDR TV.For example, a good HDR TV will output 100 nits when rendering 100 nits, but an entry level TV might
// only output 30 nits when rendering 100 nits.It is therefore necessary to allow the consumer to adjust the brightness of the SDR range in the image.This is done by
// adjusting the paper white nits value.Note that as paper white goes higher, the potential range for HDR is reduced.Therefore, it is recommended to show an HDR image
// with lots of detail in the brights so that the consumer can understand the compromise for a brighter SDR range with a smaller HDR range.
//
// Color Saturation - Display panels of HDR TVs have the capabilities to produce very bright and colorful images, since they can all produce colors outside of the Rec.709
// color space.When in SDR mode, TVs often use a color gamut expansion to make the SDR image look more colorful.When rendering the theoretically correct colors in HDR mode,
// the image can often look less colorful than when in SDR mode.A solution to this problem is to do color gamut expansion within the game.This option control how much color
// gamut expansion should be applied.
//
// Refer to the white paper "HDR on Xbox One", and the HDRDisplayMapping sample
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HDRCalibration.h"
#include "HDRCommon.h"
#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DX;
using namespace DirectX;
using namespace Colors;
using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float g_DefaultPaperWhiteNits = 200.0f; // How bright white (1,1,1) is
    const DirectX::XMVECTORF32 g_SelectedColor = { 1.000000000f * 2.0f, 1.000000000f * 2.0f, 0.000000000f * 2.0f, 1.0f };
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_bIsDisplayInHDRMode(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB /* GameDVR format */,
        DXGI_FORMAT_UNKNOWN,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD | DX::DeviceResources::c_EnableHDR);

    m_hdrScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    // Try to switch the TV into HDR mode, and retrieve the calibration data from the system
    SetDisplayMode(&m_calibrationDataFromSystem);

    m_bApplyCalibration = true;
    m_bShowCalibrationScreen = true;
	m_bUseObjectiveCalibration = false;

    m_flashContrastTimer = 0.0;
    m_currentHDRImage = 0;
    m_calibrationScreen = CalibrationScreen::MaxHDRBrightness;
    m_calibrationData.DisplayGamma = 1.0f;
    m_calibrationData.PaperWhiteNits = g_DefaultPaperWhiteNits;
    m_calibrationData.ColorGamutExpansion = 1.0f;

    // We'll use the calibration data from the Xbox system HDRGameCalibration app as a good default starting point for the sample's own calibration step.
    // Consumers can access the app from the console Settings, "Calibrate HDR for games" option
    m_calibrationData.MaxHDRBrightness = m_calibrationDataFromSystem.maxToneMapLuminance;

    memcpy(&m_savedCalibrationData, &m_calibrationData, sizeof(m_calibrationData));
  
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Render all UI at 1080p so that it's easy to swtich between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_fontBatch->SetViewport(viewportUI);
    m_colorBlockBatch->SetViewport(viewportUI);
    m_spriteBatch->SetViewport(viewportUI);

    UpdateCalibrationData();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");
   
    // Control the flashing of the center block for the contrast calibration. Flashing the center block works very well for the user
    // to determin when the block becomes invisible
    m_flashContrastTimer += timer.GetElapsedSeconds();  
    if (m_flashContrastTimer > (0.5))
    {        
        m_flashContrastTimer = 0;
    }

    auto gamepad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);

    if (gamepad.IsConnected())
    {
        if (gamepad.IsViewPressed())
        {
            ExitSample();
        }

        m_gamePadButtons.Update(gamepad);

        // Select the current calibration screen
        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
        {
            int calibrationScreen = static_cast<int>(m_calibrationScreen) - 1;
            calibrationScreen = std::max(0, calibrationScreen);
            m_calibrationScreen = static_cast<CalibrationScreen>(calibrationScreen % static_cast<int>(CalibrationScreen::NumScreens));
        }

        // Select the current calibration screen
        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            // We don't have a good test images to adjust for paper white and color gamut expansion. White always looks grey against anything brighter than white.
            int calibrationScreen = static_cast<int>(m_calibrationScreen) + 1;
            calibrationScreen = std::min(static_cast<int>(CalibrationScreen::NumScreens) - 1 - (m_bUseObjectiveCalibration ? 2 : 0), calibrationScreen);
            m_calibrationScreen = static_cast<CalibrationScreen>(calibrationScreen % static_cast<int>(CalibrationScreen::NumScreens));
        }

        if (m_bShowCalibrationScreen && m_bApplyCalibration)
        {
            // Adjust values for each specific calibration screen
            switch (m_calibrationScreen)
            {
                // Don't allow big changes, this should be in the range [100..300] nits
            case CalibrationScreen::Brightness:
                if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_calibrationData.PaperWhiteNits -= (m_calibrationData.PaperWhiteNits <= 200.0f) ? 25.0f : 50.0f;
                    m_calibrationData.PaperWhiteNits = std::max(100.0f, m_calibrationData.PaperWhiteNits);
                }
                if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_calibrationData.PaperWhiteNits += (m_calibrationData.PaperWhiteNits < 200.0f) ? 25.0f : 50.0f;
                    m_calibrationData.PaperWhiteNits = std::min(300.0f, m_calibrationData.PaperWhiteNits);
                }
                break;

                // Don't allow big changes, this should be in the range [0.85..1.25]
            case CalibrationScreen::Contrast:
                if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_calibrationData.DisplayGamma -= 0.05f;
                    m_calibrationData.DisplayGamma = std::max(0.85f, m_calibrationData.DisplayGamma);
                }
                if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_calibrationData.DisplayGamma += 0.05f;
                    m_calibrationData.DisplayGamma = std::min(1.25f, m_calibrationData.DisplayGamma);
                }
                break;

            case CalibrationScreen::MaxHDRBrightness:
                if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_calibrationData.MaxHDRBrightness -= 100;
                    m_calibrationData.MaxHDRBrightness = std::max(500.0f, m_calibrationData.MaxHDRBrightness);
                }
                if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_calibrationData.MaxHDRBrightness += 100;
                    m_calibrationData.MaxHDRBrightness = std::min(c_MaxNitsFor2084, m_calibrationData.MaxHDRBrightness);
                }
                break;

            case CalibrationScreen::Color:
                if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_calibrationData.ColorGamutExpansion -= 0.5f;
                    m_calibrationData.ColorGamutExpansion = std::max(0.0f, m_calibrationData.ColorGamutExpansion);
                }
                if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_calibrationData.ColorGamutExpansion += 0.5f;
                    m_calibrationData.ColorGamutExpansion = std::min(1.0f, m_calibrationData.ColorGamutExpansion);
                }
                break;

            default:
                break;
            }
        }

        // Toggle between showing the original image or the calibrated one
        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_bApplyCalibration = !m_bApplyCalibration;
        }

        // Change calibration method
        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_bUseObjectiveCalibration = !m_bUseObjectiveCalibration;

            // We don't have a good test images to adjust for paper white and color gamut expansion. White always looks grey against anything brighter than white.
            if (m_bUseObjectiveCalibration)
            {
                memcpy(&m_savedCalibrationData, &m_calibrationData, sizeof(m_calibrationData));
                m_calibrationData.PaperWhiteNits = 200.0f;
                m_calibrationData.ColorGamutExpansion = 0.5f;
                m_calibrationScreen = CalibrationScreen::MaxHDRBrightness;
            }
            else
            {
                // Restore the values we used when adjusting using the subjective method
                memcpy(&m_calibrationData, &m_savedCalibrationData, sizeof(m_calibrationData));
            }
        }

        // Toggle between showing the calibration screen or not
        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_bShowCalibrationScreen = !m_bShowCalibrationScreen;
        }

        if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentHDRImage--;
            if (m_currentHDRImage < 0)
            {
                m_currentHDRImage = NumImages - 1;
            }
        }

        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentHDRImage++;
            m_currentHDRImage = m_currentHDRImage % NumImages;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Update shader constants with new calibration values
    UpdateCalibrationData();
}

// Update the calibratifon data as well as the constant buffer
void Sample::UpdateCalibrationData()
{
    // Data for contrast and brightness
    m_calibrationDataCB.PaperWhiteNits = m_calibrationData.PaperWhiteNits;
    m_calibrationDataCB.DisplayGamma = m_calibrationData.DisplayGamma;
    m_calibrationDataCB.ColorGamutExpansion = m_calibrationData.ColorGamutExpansion;

    // This data is used to map the scene to the maximum potential of the HDR TV, to give a good balance between brightness and
    // details in the brights. Refer to the HDRDisplayMapping sample for details. 
    constexpr float softShoulderStart = 100.0f;
    constexpr float nitsAllowedToClip = 0.0f;

    // NOTE: Contrast adjustment on HDR values will change the peak brightness, so we need to adapt the max scene value going into the display mapping shader function
    float maxLuminanceOfScene = std::max(m_HDRImage[m_currentHDRImage].GetMaxLuminance(), powf(m_HDRImage[m_currentHDRImage].GetMaxLuminance(), m_calibrationData.DisplayGamma));

    float maxNitsOfScene = CalcNits(maxLuminanceOfScene, m_calibrationDataCB.PaperWhiteNits);
    float softShoulderNitsStart = softShoulderStart;
    float normalizedLinearValueSoftShoulderStart = CalcNormalizedLinearValue(softShoulderNitsStart);
    float normalizedLinearValueMaxNitsOfTV = CalcNormalizedLinearValue(m_calibrationData.MaxHDRBrightness + nitsAllowedToClip);
    float normalizedLinearValueMaxNitsOfScene = CalcNormalizedLinearValue(maxNitsOfScene);
    m_calibrationDataCB.SoftShoulderStart2084 = LinearToST2084(normalizedLinearValueSoftShoulderStart);
    m_calibrationDataCB.MaxBrightnessOfTV2084 = LinearToST2084(normalizedLinearValueMaxNitsOfTV);
    m_calibrationDataCB.MaxBrightnessOfHDRScene2084 = LinearToST2084(normalizedLinearValueMaxNitsOfScene);

    // When toggling to show the original uncalibrated image
    if (!m_bApplyCalibration)
    {
        m_calibrationDataCB.SoftShoulderStart2084 = 1.0f;
        m_calibrationDataCB.MaxBrightnessOfTV2084 = 1.0f;
        m_calibrationDataCB.MaxBrightnessOfHDRScene2084 = 0.1f;
        m_calibrationDataCB.PaperWhiteNits = g_DefaultPaperWhiteNits;
        m_calibrationDataCB.DisplayGamma = 1.0f;
        m_calibrationDataCB.ColorGamutExpansion = 0.0f;
    }
}

#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Set the descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    // Render
    Clear();

    // HDR images are pretty big, so load them async. Show a message when not done loading
    if (!m_HDRImage[m_currentHDRImage].HasFinishedLoading())
    {
        SimpleMath::Vector2 fontPos(300, 300);
        m_fontBatch->Begin(commandList);
        {
            m_textFont->DrawString(m_fontBatch.get(), L"Loading image ...", fontPos, White, 0.0f, g_XMZero, 1.0f);
        }
        m_fontBatch->End();
    }
    else
    {
        RenderHDRScene();
        RenderCalibrationScreen();
    }

    RenderUI();
    PrepareSwapChainBuffers();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}


// Process the HDR scene so that the swapchains can correctly be sent to HDR or SDR display
void Sample::PrepareSwapChainBuffers()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"PrepareSwapChainBuffers");

    // We need to sample from the HDR backbuffer
    m_hdrScene->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Set RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor[2] = {m_deviceResources->GetRenderTargetView(), m_deviceResources->GetGameDVRRenderTargetView() };
    commandList->OMSetRenderTargets(2, rtvDescriptor, FALSE, nullptr);

    // Update constant buffer and render
    auto calibrationDataCB = m_graphicsMemory->AllocateConstant<CalibrationDataCB>(m_calibrationDataCB);
    m_fullScreenQuad->Draw(commandList, m_d3dPrepareSwapChainBufferPSO.Get(), m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::HDRScene), calibrationDataCB.GpuAddress());

    PIXEndEvent(commandList);
}


// Render HDR scene
void Sample::RenderHDRScene()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderHDRScene");
    m_fullScreenQuad->Draw(commandList, m_d3dRenderHDRImagePSO.Get(), m_HDRImage[m_currentHDRImage].GetShaderResourceView());
    PIXEndEvent(commandList);
}


// Render the different calibration screens
void Sample::RenderCalibrationScreen()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderCalibrationScreen");

    m_calibrationDataCB.CalibrationImageBoundingBoxU1 =
    m_calibrationDataCB.CalibrationImageBoundingBoxV1 =
    m_calibrationDataCB.CalibrationImageBoundingBoxU2 =
    m_calibrationDataCB.CalibrationImageBoundingBoxV2 = 0.0f;

    if (!m_bShowCalibrationScreen)
    {
        PIXEndEvent(commandList);
        return;
    }

    if (m_bUseObjectiveCalibration)
    {
        RenderObjectiveCalibrationScreen();
    }
    else
    {
        RenderSubjectiveCalibrationScreen();
    }

    PIXEndEvent(commandList);
}


// Subjective calibration, just have the user adjust the image until he likes it, i.e. no test patterns
void Sample::RenderSubjectiveCalibrationScreen()
{
    auto commandList = m_deviceResources->GetCommandList();

    static float startX = 25;
    static float startY = 650;
    constexpr float fontScale = 0.75f;

    // SpriteBatch requires a texture, otherwise it will assert, but we just want to draw a color, so give it a dummy texture
    XMUINT2 dummyTextureSize = { 1, 1 };
    auto dummyTexture = m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::HDRScene);

    // Transparent background
    m_colorBlockBatch->Begin(commandList, SpriteSortMode_Immediate);
    DirectX::XMVECTORF32 color = { 0.0f, 0.0f, 0.0f, 0.95f };
    RECT position = { int(startX), int(startY), 480, int(startY) + 200 };
    m_colorBlockBatch->Draw(dummyTexture, dummyTextureSize, position, color);
    m_colorBlockBatch->End();

    wchar_t strText[2048];
    SimpleMath::Vector2 fontPos(startX, startY);

    // Show current calibration data
    m_fontBatch->Begin(commandList);
    fontPos.x = startX;
    fontPos.y = startY + 25;

    color = m_bApplyCalibration ? ((m_calibrationScreen == CalibrationScreen::MaxHDRBrightness) ? g_SelectedColor : ATG::ColorsHDR::OffWhite) : ATG::ColorsHDR::LightGrey;
    swprintf_s(strText, L"%sBright details: %1.0f", (m_calibrationScreen == CalibrationScreen::MaxHDRBrightness) ? L"->" : L"  ", m_calibrationData.MaxHDRBrightness);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, color, 0.0f, g_XMZero, fontScale);

    fontPos.y += 40.0f;
    color = m_bApplyCalibration ? ((m_calibrationScreen == CalibrationScreen::Contrast) ? g_SelectedColor : ATG::ColorsHDR::OffWhite) : ATG::ColorsHDR::LightGrey;
    swprintf_s(strText, L"%sDark details: %1.2f", (m_calibrationScreen == CalibrationScreen::Contrast) ? L"->" : L"  ", m_calibrationDataCB.DisplayGamma);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, color, 0.0f, g_XMZero, fontScale);

    fontPos.y += 40.0f;
    color = m_bApplyCalibration ? ((m_calibrationScreen == CalibrationScreen::Brightness) ? g_SelectedColor : ATG::ColorsHDR::OffWhite) : ATG::ColorsHDR::LightGrey;
    swprintf_s(strText, L"%sBrightness: %1.0f nits", (m_calibrationScreen == CalibrationScreen::Brightness) ? L"->" : L"  ", m_bApplyCalibration ? m_calibrationData.PaperWhiteNits : g_DefaultPaperWhiteNits);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, color, 0.0f, g_XMZero, fontScale);

    fontPos.y += 40.0f;
    color = m_bApplyCalibration ? ((m_calibrationScreen == CalibrationScreen::Color) ? g_SelectedColor : ATG::ColorsHDR::OffWhite) : ATG::ColorsHDR::LightGrey;
    swprintf_s(strText, L"%sColor: %0.2f", (m_calibrationScreen == CalibrationScreen::Color) ? L"->" : L"  ", m_calibrationDataCB.ColorGamutExpansion);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, color, 0.0f, g_XMZero, fontScale);

    fontPos.y += 65.0f;
    switch (m_calibrationScreen)
    {
    case CalibrationScreen::MaxHDRBrightness:
        DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"Very bright with details visible in clouds", fontPos, g_SelectedColor, fontScale);
        break;

    case CalibrationScreen::Contrast:
        DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"Dark shadow with only some detail visible", fontPos, g_SelectedColor, fontScale);
        break;

    case CalibrationScreen::Brightness:
        DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"Overall brightness of image", fontPos, g_SelectedColor, fontScale);
        break;

    case CalibrationScreen::Color:
        DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"Color saturation of car and break light", fontPos, g_SelectedColor, fontScale);
        break;

    default:
        break;
    }

    m_fontBatch->End();

    // Indicate where to look in the image for changes
    switch (m_calibrationScreen)
    {
    case CalibrationScreen::MaxHDRBrightness:
        RenderRedCircle(10, 210, 600, 225);
        break;

    case CalibrationScreen::Contrast:
        RenderRedCircle(1150, 870, 400, 160);
        break;

    case CalibrationScreen::Brightness:
        RenderRedCircle(10, 480, 600, 150);
        break;

    case CalibrationScreen::Color:
        RenderRedCircle(950, 525, 550, 420);        
        break;

    default:
        break;
    }
}


// Objective calibration using test patterns
void Sample::RenderObjectiveCalibrationScreen()
{
    auto commandList = m_deviceResources->GetCommandList();

    constexpr float startX = 710;
    constexpr float startY = 40.0f;
    constexpr float fontScale = 0.75f;
    constexpr int size = 500;
    constexpr int border = 75;
    constexpr int blockSize = 350;

    // SpriteBatch requires a texture, otherwise it will assert, but we just want to draw a color, so give it a dummy texture
    XMUINT2 dummyTextureSize = { 1, 1 };
    auto dummyTexture = m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::HDRScene);

    // Transparent background
    m_colorBlockBatch->Begin(commandList, SpriteSortMode_Immediate);
    DirectX::XMVECTORF32 color = { 0.0f, 0.0f, 0.0f, 0.95f };
    RECT position = { static_cast<int>(startX), 1080 - size - 100 - static_cast<int>(startY), static_cast<int>(startX) + size, 1080 - 100 - static_cast<int>(startY) };
    m_colorBlockBatch->Draw(dummyTexture, dummyTextureSize,  position, color);
    m_colorBlockBatch->End();

    // Set the bounding box where the callibration screens are rendered
    m_calibrationDataCB.CalibrationImageBoundingBoxU1 = position.left / 1920.0f;
    m_calibrationDataCB.CalibrationImageBoundingBoxV1 = position.top / 1080.0f;
    m_calibrationDataCB.CalibrationImageBoundingBoxU2 = position.right / 1920.0f;
    m_calibrationDataCB.CalibrationImageBoundingBoxV2 = position.bottom / 1080.0f;

    wchar_t strText[2048];
    SimpleMath::Vector2 fontPos(startX, startY);

    // Show current calibration data
    m_fontBatch->Begin(commandList);
    fontPos.x = startX + 18.0f;
    fontPos.y = 840.0f;

    color = m_bApplyCalibration ? ((m_calibrationScreen == CalibrationScreen::MaxHDRBrightness) ? g_SelectedColor : ATG::ColorsHDR::OffWhite) : ATG::ColorsHDR::LightGrey;
    swprintf_s(strText, L"Max HDR Brightness: %1.0f", m_calibrationData.MaxHDRBrightness);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, color, 0.0f, g_XMZero, fontScale);

    fontPos.y += 40.0f;
    color = m_bApplyCalibration ? ((m_calibrationScreen == CalibrationScreen::Contrast) ? g_SelectedColor : ATG::ColorsHDR::OffWhite) : ATG::ColorsHDR::LightGrey;
    swprintf_s(strText, L"Contrast: %1.2f", m_calibrationDataCB.DisplayGamma);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, color, 0.0f, g_XMZero, fontScale);

    m_fontBatch->End();

    // Determine the contrast adjustment, similar to display gamma for SDR
    if (m_calibrationScreen == CalibrationScreen::Contrast)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Contrast");

        m_calibrationDataCB.PaperWhiteNitsForCalibrationScreen = 100.0f;        // Use 100 nits paper white for this calibration screen
        m_calibrationDataCB.ApplyContrastToCalibrationScreen = true;            // Apply contrast adjustment to this calibration screen in the shader

        // Block size and position. It's important that the calibration block is in the center of the screen, especially for some LCDs with bad viewing angles
        const int x = static_cast<int>(startX) + border;
        const int y = 1080 - static_cast<int>(startY) - size - border + 15;
        const int w = blockSize;
        const int h = blockSize;
        
        // Render a black block with 300 nits white border. Some LCD TVs show low levels of gray brighter when only black is rendered to the screen vs. 
        // when something else is on the screen. This can cause this kind of calibration image to be seen different when only black is on the screen vs.
        // when other pixlels are lit
        RenderCalibrationBlock(x, y, w, h, 0.0f, 300.0f, true, m_calibrationDataCB.PaperWhiteNitsForCalibrationScreen);

        // Render the flashing center block with the adjustable fading out value. Flashing the block works better, becuase the user can more easily see when the block becomes invisible
        if (m_flashContrastTimer < 0.25)
        {
            // Adjust the calibration value such that it gets darker/brighter quicker than just the actual value of 1/255. We use some magic scaling
            // factors determined after testing 7 different HDR TVs with different mode settings
            float calibrationValue = 1.0f / 255.0f;
            calibrationValue = powf(calibrationValue / 3.0f, m_calibrationData.DisplayGamma) * 2.0f;

            // When the TV is in SDR mode, use a different value to determine display gamma, and still have SDR and HDR match in contrast
            if (!m_bIsDisplayInHDRMode)
            {
                calibrationValue = 0.75f / 255.0f;
            }

            RenderCalibrationBlock(x + w / 4, y + h / 4, w / 2, h / 2, calibrationValue, 0, false, m_calibrationDataCB.PaperWhiteNitsForCalibrationScreen);
        }

        m_fontBatch->Begin(commandList);
        fontPos.x = 490;
        fontPos.y = 980;
        DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"Adjust until flashing center block becomes invisible", fontPos, g_SelectedColor, fontScale);
        m_fontBatch->End();

        PIXEndEvent(commandList);
    }

    // Determine the max percieved brightness of the TV, so that we can make a good compromise between brightness and details. Without this calibration,
    // all HDR values above what the TV can reproduce, will simply be clipped, loosing detail in the brights. This is very important as paper white is 
    // set higher, becuase that will push more HDR values above what the TV can display.
    if (m_calibrationScreen == CalibrationScreen::MaxHDRBrightness)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"HDRBrightness");

        m_calibrationDataCB.PaperWhiteNitsForCalibrationScreen = 100.0f;    // Use 100 nits paper white for this calibration screen
        m_calibrationDataCB.ApplyContrastToCalibrationScreen = false;       // Do not apply any contrast adjustment for this calibration screen in shader

        // Render a block with 10,000 nits on the outside, and the current brightness value on the inside. Once the inside block appears the same as the
        // outside block, we know that's the maximum perceived brightness of the TV
        RenderCalibrationBlock(static_cast<int>(startX) + border, 1080 - static_cast<int>(startY) - size - border + 15, 350, 350, m_calibrationData.MaxHDRBrightness, c_MaxNitsFor2084, true, m_calibrationDataCB.PaperWhiteNitsForCalibrationScreen);

        m_fontBatch->Begin(commandList);
        fontPos.x = 565;
        fontPos.y = 980;
        DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"Adjust until center block is not visible", fontPos, g_SelectedColor, fontScale);
        m_fontBatch->End();

        PIXEndEvent(commandList);
    }
}


// Render the calibration block
void Sample::RenderCalibrationBlock(int startX, int startY, int width, int height, float innerValue, float outerValue, bool valueAsNits, float paperWhiteNits)
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderCalibrationBlock");

    // SpriteBatch requires a texture, otherwise it will assert, but we just want to draw a color, so give it a dummy texture
    XMUINT2 dummyTextureSize = { 1, 1 };
    auto dummyTexture = m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::HDRScene);

    m_colorBlockBatch->Begin(commandList, SpriteSortMode_Immediate);

    // Outer block
    {
        RECT position = { startX, startY, startX + width, startY + height };
        float hdrSceneValue = valueAsNits ? CalcHDRSceneValue(outerValue, paperWhiteNits) : outerValue;
        XMVECTOR hdrSceneColor = MakeColor(hdrSceneValue);
        m_colorBlockBatch->Draw(dummyTexture, dummyTextureSize, position, hdrSceneColor);
    }

    // Inner block
    {
        constexpr int size = 5;
        RECT position = { startX + width / size, startY + height / size, startX + width - width / size, startY + height - height / size };
        float hdrSceneValue = valueAsNits ? CalcHDRSceneValue(innerValue, paperWhiteNits) : innerValue;
        XMVECTOR hdrSceneColor = MakeColor(hdrSceneValue);
        m_colorBlockBatch->Draw(dummyTexture, dummyTextureSize, position, hdrSceneColor);
    }

    m_colorBlockBatch->End();

    PIXEndEvent(commandList);
}


// Render text with dark gray shadow which helps when text is on top of bright areas on the image
void Sample::DrawStringWithShadow(const wchar_t* string, DirectX::SimpleMath::Vector2& fontPos, DirectX::FXMVECTOR color, float fontScale)
{
    fontPos.x += 1.0f;
    fontPos.y += 1.0f;
    m_textFont->DrawString(m_fontBatch.get(), string, fontPos, Black, 0.0f, g_XMZero, fontScale);
    fontPos.x -= 1.0f;
    fontPos.y -= 1.0f;
    m_textFont->DrawString(m_fontBatch.get(), string, fontPos, color, 0.0f, g_XMZero, fontScale);
}


// Render the UI
void Sample::RenderUI()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderUI");

    constexpr float fontScale = 0.75f;
    SimpleMath::Vector2 fontPos;
 
    m_fontBatch->Begin(commandList);

    fontPos.x = 40.0f;
    fontPos.y = 50.0f;
    DrawStringWithShadow(L"HDR Calibration Sample", fontPos, White, fontScale);

    fontPos.y = 100.0f;
    DrawStringWithShadow(m_bIsDisplayInHDRMode ? L"TV in HDR Mode: TRUE" : L"TV in HDR Mode: FALSE", fontPos, ATG::ColorsHDR::White, fontScale);

    fontPos.y += 35;
    DrawStringWithShadow(m_bUseObjectiveCalibration ? L"Method: Objective" : L"Method: Subjective", fontPos, ATG::ColorsHDR::White, fontScale);

    fontPos.y += 35;
    DrawStringWithShadow(L"Apply Calibration:", fontPos, ATG::ColorsHDR::White, fontScale);
    fontPos.x += 375;
    DrawStringWithShadow(m_bApplyCalibration ? L"TRUE" : L"FALSE", fontPos, m_bApplyCalibration ? ATG::ColorsHDR::White : g_SelectedColor, fontScale);

    fontPos.x = 20;
    fontPos.y = 940;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[A] Toggle Apply Calibration\n[X] Show/Hide Calibration Screens\n[B] Change Calibration Method\n[DPad] Up/Down Select Calibration\n[DPad] Left/Right Adjust Calibration", fontPos, ATG::ColorsHDR::White, 0.5f);

    fontPos.x = 500;
    fontPos.y = 1000;
    wchar_t strText[2048];
    swprintf_s(strText, L"Max Brightness from Xbox System HDRGameCalibration app: %1.0f Nits", m_calibrationDataFromSystem.maxToneMapLuminance);
    DrawStringWithShadow(strText, fontPos, ATG::ColorsHDR::White, fontScale);

    m_fontBatch->End();

    PIXEndEvent(commandList);
}


// Renders a texture with a red circle
void Sample::RenderRedCircle(int x, int y, int width, int height)
{
    auto commandList = m_deviceResources->GetCommandList();
    auto textureSize = m_redCircleTexture->GetTextureSize();
    RECT rect = { x, y, x + width, y + height };

    m_spriteBatch->Begin(commandList);
    m_spriteBatch->Draw(m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::RedCircleTexture), textureSize, rect);
    m_spriteBatch->End();
}


// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    m_hdrScene->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto const rtv = m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::HDRSceneRTV);
    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    // Clear the views.
    commandList->ClearRenderTargetView(m_deviceResources->GetRenderTargetView(), Black, 0, nullptr);
    commandList->ClearRenderTargetView(m_deviceResources->GetGameDVRRenderTargetView(), Black, 0, nullptr);
    commandList->ClearRenderTargetView(rtv, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    // While a title is suspended, the console TV settings could have changed, so we need to call the display APIs when resuming
    SetDisplayMode();

    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
}

void Sample::OnConstrained()
{
}

void Sample::OnUnConstrained()
{
    // While a title is constrained, the console TV settings could have changed, so we need to call the display APIs when unconstraining
    SetDisplayMode();
}

#pragma endregion

#pragma region Direct3D Resources
void Sample::SetDisplayMode(XDisplayHdrModeInfo* hdrData)
{
    if ((m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0)
    {
        // Request HDR mode.
        auto result = XDisplayTryEnableHdrMode(XDisplayHdrModePreference::PreferHdr, hdrData);

        m_bIsDisplayInHDRMode = (result == XDisplayHdrModeResult::Enabled);

#ifdef _DEBUG
        OutputDebugStringA((m_bIsDisplayInHDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif
    }
}

// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_fullScreenQuad = std::make_unique<FullScreenQuad>();
    m_fullScreenQuad->Initialize(m_deviceResources->GetD3DDevice());

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create descriptor heaps
    m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RTVDescriptors::CountRTV);
    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::Count);

    // Init fonts
    const RenderTargetState rtState(m_hdrScene->GetFormat(), m_deviceResources->GetDepthBufferFormat());
    InitializeSpriteFonts(device, resourceUpload, rtState);
   
    // Create red circle texture
    m_redCircleTexture = std::make_unique<DX::Texture>(device, resourceUpload, m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::RedCircleTexture), L"RedCircle.dds", true);

    // SpriteBatch for rendering HDR values into the backbuffer
    {
        auto pixelShaderBlob = DX::ReadData(L"ColorPS.cso");
        SpriteBatchPipelineStateDescription pd(rtState);
        pd.customPixelShader.BytecodeLength = pixelShaderBlob.size();
        pd.customPixelShader.pShaderBytecode = pixelShaderBlob.data();
        m_colorBlockBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);       
    }

    // SpriteBatch for rendering the red circle texture
    {
        SpriteBatchPipelineStateDescription pd(rtState);
        m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);
    }

    // PSO for rendering an HDR texture into the HDR backbuffer
    {
        auto pixelShaderBlob = DX::ReadData(L"FullScreenQuadPS.cso");
        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_fullScreenQuad->GetRootSignature();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_hdrScene->GetFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dRenderHDRImagePSO.ReleaseAndGetAddressOf())));
    }

    // PSO for rendering the HDR10 and GameDVR swapchain buffers
    {
        auto pixelShaderBlob = DX::ReadData(L"PrepareSwapChainBuffersPS.cso");
        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_fullScreenQuad->GetRootSignature();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 2;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.RTVFormats[1] = m_deviceResources->GetGameDVRFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dPrepareSwapChainBufferPSO.ReleaseAndGetAddressOf())));
    }  

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());   
    uploadResourcesFinished.wait();     // Wait for resources to upload  

    // Set views for HDR textures
    for (int i = 0; i < NumImages; i++)
    {
        auto cpuHandle = m_resourceDescriptorHeap->GetCpuHandle(size_t(ResourceDescriptors::HDRTexture + i));
        auto gpuHandle = m_resourceDescriptorHeap->GetGpuHandle(size_t(ResourceDescriptors::HDRTexture + i));
        m_HDRImage[i].SetShaderResourceView(cpuHandle, gpuHandle);
    }

    // Load HDR images async
    concurrency::task<bool> t([this]()
    {
        for (int i = 0; i < NumImages; i++)
        {
            m_HDRImage[i].Load(m_HDRImageFiles[i], m_deviceResources->GetD3DDevice(), m_deviceResources->GetCommandQueue());
        }

        return true;
    });

    // Setup HDR render target.
    m_hdrScene->SetDevice(device, m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::HDRScene), m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::HDRSceneRTV));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Create HDR backbuffer resources.
    auto outputSize = m_deviceResources->GetOutputSize();
    auto width = size_t(outputSize.right - outputSize.left);
    auto height = size_t(outputSize.bottom - outputSize.top);
    m_hdrScene->SizeResources(width, height);
}

// Initialize all the fonts used
void Sample::InitializeSpriteFonts(ID3D12Device* d3dDevice, ResourceUploadBatch& resourceUpload, const RenderTargetState& rtState)
{
    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
    m_fontBatch = std::make_unique<SpriteBatch>(d3dDevice, resourceUpload, pd);

    auto index = static_cast<size_t>(ResourceDescriptors::TextFont);
    m_textFont = std::make_unique<SpriteFont>(d3dDevice, resourceUpload, L"Courier_36.spritefont", m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));

    index = static_cast<size_t>(ResourceDescriptors::ControllerFont);
    m_controllerFont = std::make_unique<SpriteFont>(d3dDevice, resourceUpload, L"XboxOneControllerSmall.spritefont", m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));
}

#pragma endregion
