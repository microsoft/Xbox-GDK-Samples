//--------------------------------------------------------------------------------------
// HDRDisplayMapping12.cpp
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

#include "pch.h"
#include "HDRDisplayMapping.h"
#include "HDR\HDRCommon.h"
#include "ReadData.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DX;
using namespace DirectX;
using namespace SimpleMath;
using namespace Colors;
using Microsoft::WRL::ComPtr;
using DirectX::Colors::Black;
using DirectX::Colors::White;

namespace
{
    constexpr float g_DefaultPaperWhiteNits = 200.0f;   // How bright white (1,1,1) is
    constexpr float g_SoftShoulderStartNits = 750.0f;   // A good default value for m_softShoulderStartNits
    constexpr float g_NitsAllowedToBeClipped = 200.0f;  // A good default value for m_nitsAllowedToBeClipped

    // Clamp value between 0 and 1
    inline float Clamp(float value) noexcept
    {
        return std::min(std::max(value, 0.0f), 1.0f);
    }
}

// Constructor
Sample::Sample() noexcept(false) :
    m_frame(0),
    m_tonemappingMode(TonemappingMode::Reinhard),
    m_displayMappingMode(DisplayMappingMode::SomeClipping),
    m_displayMappingData{},
    m_maxBrightnessOfTV(0),
    m_maxBrightnessReturnedBySystemCalibration(0),
    m_currentPaperWhiteNits(g_DefaultPaperWhiteNits),
    m_softShoulderStartNits(g_SoftShoulderStartNits),
    m_nitsAllowedToBeClipped(g_NitsAllowedToBeClipped),
    m_bRenderAsSDR(false),
    m_bIndicateValuesBrighterThanTV(false),
    m_bIsTVInHDRMode(false),
    m_bRenderGraphs(false),
    m_bCalibrateTV(false),
    m_currentHDRImage(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB /* GameDVR format */,
        DXGI_FORMAT_UNKNOWN,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD | DX::DeviceResources::c_EnableHDR);

    m_hdrScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_hdrScene->SetClearColor(Black);
}


Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}


// Initialize the Direct3D resources required to run, and setting TV to HDR mode
void Sample::Initialize(HWND window) 
{
    // Determine if attached display is HDR or SDR, if HDR, also set the TV in HDR mode.
    XDisplayHdrModeInfo hdrCalibrationDataFromSystem = {};
    SetDisplayMode(&hdrCalibrationDataFromSystem);

    m_bCalibrateTV = m_bIsTVInHDRMode ? true : false;
    m_maxBrightnessReturnedBySystemCalibration = hdrCalibrationDataFromSystem.maxToneMapLuminance;

    // We'll use the calibration data from the Xbox system HDRGameCalibration app as a good default starting point for the sample's own calibration step.
    // Consumers can access the app from the console Settings, "Calibrate HDR for games" option
    m_maxBrightnessOfTV = hdrCalibrationDataFromSystem.maxToneMapLuminance;

#ifdef _DEBUG
    OutputDebugStringA((m_bIsTVInHDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif    

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
    m_spriteBatch->SetViewport(viewportUI);

    UpdateDisplayMappingData();
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


// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto gamepad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);

    if (gamepad.IsConnected())
    {
        if (gamepad.IsViewPressed())
        {
            ExitSample();
        }

        m_gamePadButtons.Update(gamepad);

        if (m_bCalibrateTV && m_bIsTVInHDRMode)
        {
            // When rendering calibration image, don't use any shoulders and use 100 nits for paper white
            m_displayMappingMode = DisplayMappingMode::None;
            m_tonemappingMode = TonemappingMode::None;
            m_currentPaperWhiteNits = 100.0f;

            if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
            {
                m_bCalibrateTV = false;

                // Restore values
                m_currentPaperWhiteNits = g_DefaultPaperWhiteNits;
                m_tonemappingMode = TonemappingMode::Reinhard;
                m_displayMappingMode = DisplayMappingMode::SomeClipping;
            }

            if ((m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED) ||
                (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED))
            {
                m_maxBrightnessOfTV -= 100.0f;
                m_maxBrightnessOfTV = std::max(m_maxBrightnessOfTV, 200.0f);
            }

            if ((m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED) ||
                (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED))
            {
                m_maxBrightnessOfTV += 100.0f;
                m_maxBrightnessOfTV = std::min(m_maxBrightnessOfTV, DX::c_MaxNitsFor2084);
            }
        }
        else
        {
            if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
            {
                if (m_bIsTVInHDRMode && !m_bRenderAsSDR)
                {
                    int mode = static_cast<int>(m_displayMappingMode) + 1;
                    m_displayMappingMode = static_cast<DisplayMappingMode>(mode % static_cast<int>(DisplayMappingMode::NumModes));
                }
                else
                {
                    int mode = static_cast<int>(m_tonemappingMode) + 1;
                    m_tonemappingMode = static_cast<TonemappingMode>(mode % static_cast<int>(TonemappingMode::NumModes));
                }
            }

            if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
            {
                m_bRenderGraphs = !m_bRenderGraphs;

                if (!m_bRenderGraphs)
                {
                    m_bIndicateValuesBrighterThanTV = false;
                }
            }

            if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
            {
                m_bIndicateValuesBrighterThanTV = !m_bIndicateValuesBrighterThanTV;
                m_bRenderGraphs = false;
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

            if (m_bIsTVInHDRMode)
            {
                if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_bRenderAsSDR = !m_bRenderAsSDR;
                }

                if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_nitsAllowedToBeClipped -= 50;
                    m_nitsAllowedToBeClipped = std::max(0.0f, m_nitsAllowedToBeClipped);
                }

                if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_nitsAllowedToBeClipped += 50;
                    m_nitsAllowedToBeClipped = std::min(1000.0f, m_nitsAllowedToBeClipped);
                }

                if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_softShoulderStartNits -= 50.0f;
                    m_softShoulderStartNits = std::max(m_softShoulderStartNits, 100.0f);
                }

                if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_softShoulderStartNits += 50.0f;
                    m_softShoulderStartNits = std::min(m_softShoulderStartNits, m_maxBrightnessOfTV);
                }
            }
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    UpdateDisplayMappingData();
}


// Update the display mapping data as well as the constant buffer
void Sample::UpdateDisplayMappingData()
{
    float maxNitsOfScene = CalcNits(m_HDRImage[m_currentHDRImage].GetMaxLuminance(), m_currentPaperWhiteNits); 
    float normalizedLinearValueSoftShoulderStart = CalcNormalizedLinearValue(m_softShoulderStartNits);

    float maxNitsToMapTo = (m_displayMappingMode == DisplayMappingMode::SomeClipping) ? (m_maxBrightnessOfTV + m_nitsAllowedToBeClipped) : m_maxBrightnessOfTV;
    float normalizedLinearValueMaxNitsOfTV = CalcNormalizedLinearValue(maxNitsToMapTo);
    float normalizedLinearValueMaxNitsOfScene = CalcNormalizedLinearValue(maxNitsOfScene);

    m_displayMappingData.PaperWhiteNits = m_currentPaperWhiteNits;
    m_displayMappingData.SoftShoulderStart2084 = LinearToST2084(normalizedLinearValueSoftShoulderStart);
    m_displayMappingData.MaxBrightnessOfTV2084 = LinearToST2084(normalizedLinearValueMaxNitsOfTV);
    m_displayMappingData.MaxBrightnessOfHDRScene2084 = LinearToST2084(normalizedLinearValueMaxNitsOfScene);

    // Visualization data
    m_displayMappingData.RenderAsSDR = m_bRenderAsSDR;
    m_displayMappingData.TonemappingMode = static_cast<int>(m_tonemappingMode);
    m_displayMappingData.DisplayMappingMode = static_cast<int>(m_displayMappingMode);

    // Don't show clipped values in graph mode
    m_displayMappingData.IndicateValuesBrighterThanTV = m_bRenderGraphs ? false : m_bIndicateValuesBrighterThanTV;

    // Use the actual max brightness of TV during visualization, not including m_nitsAllowedToBeClipped
    m_displayMappingData.MaxBrightnessOfTV2084ForVisualization = LinearToST2084(CalcNormalizedLinearValue(m_maxBrightnessOfTV));
}

#pragma endregion

#pragma region Render

// Render
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"Render");

    // Set the descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptorHeap->Heap() };
    d3dCommandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    Clear();

    // Render
    if (m_bCalibrateTV)
    {
        // Determine max brightness of TV
        RenderCalibrationBlock(1000, 100, 800, m_maxBrightnessOfTV);
    }
    else if (m_bRenderGraphs)
    {
        // Render curves / graphs
        RenderGraphs();
    }
    else
    {
        // Render the HDR images
        RenderHDRScene();
    }

    // Render the UI with values of 1.0f, which will be perceived as white
    RenderUI();

    // Process the HDR scene so that the swapchains can correctly be sent to HDR or SDR display
    PrepareSwapChainBuffers();

    PIXEndEvent(d3dCommandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}


// Process the HDR scene so that the swapchains can correctly be sent to HDR or SDR display
void Sample::PrepareSwapChainBuffers()
{
    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"PrepareSwapChainBuffers");

    // We need to sample from the HDR backbuffer
    m_hdrScene->TransitionTo(d3dCommandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Set RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor[2] = { m_deviceResources->GetRenderTargetView(), m_deviceResources->GetGameDVRRenderTargetView() };
    d3dCommandList->OMSetRenderTargets(2, rtvDescriptor, FALSE, nullptr);

    // Update constant buffer and render
    auto calibrationDataCB = m_graphicsMemory->AllocateConstant<DisplayMappingData>(m_displayMappingData);
    m_fullScreenQuad->Draw(d3dCommandList, m_d3dPrepareSwapChainBufferPSO.Get(), m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::HDRScene), calibrationDataCB.GpuAddress());

    PIXEndEvent(d3dCommandList);
}


// Render HDR images
void Sample::RenderHDRScene()
{
    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"RenderHDRScene");

    // HDR images are pretty big, so load them async. Show a message when not done loading
    if (m_HDRImage[m_currentHDRImage].HasFinishedLoading())
    {
        m_fullScreenQuad->Draw(d3dCommandList, m_d3dRenderHDRImagePSO.Get(), m_HDRImage[m_currentHDRImage].GetShaderResourceView());
    }
    else
    {
        Vector2 fontPos(300, 300);
        m_fontBatch->Begin(d3dCommandList);
        {
            m_textFont->DrawString(m_fontBatch.get(), L"Loading image ...", fontPos, White, 0.0f, g_XMZero, 1.0f);
        }
        m_fontBatch->End();
    }

    PIXEndEvent(d3dCommandList);
}

namespace
{
    inline XMVECTOR MakeColor(float value) noexcept
    {
        XMVECTORF32 color = { value, value, value, 1.0f };
        return color;
    }
}

// Render the ST.2084 curve
void Sample::Render2084Curve(const int x, const float viewportWidth, const float viewportHeight)
{
    float x1 = static_cast<float>(x);
    float normalizedLinearValue = static_cast<float>(x) / viewportWidth;
    float normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
    float y1 = viewportHeight - (normalizedNonLinearValue * viewportHeight);

    float nits = CalcNits(normalizedLinearValue);

    float x2 = x1 + 1;
    normalizedLinearValue = static_cast<float>(x2) / viewportWidth;
    normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
    float y2 = viewportHeight - (normalizedNonLinearValue * viewportHeight);

    m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x1, y1, 0), MakeColor(0.2f)), VertexPositionColor(Vector3(x2, y2, 0), MakeColor(0.2f)));

    // Flatten out graph at clipped values
    if (nits > m_maxBrightnessOfTV)
    {
        normalizedLinearValue = CalcNormalizedLinearValue(m_maxBrightnessOfTV);
        normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
        y1 = viewportHeight - (normalizedNonLinearValue * viewportHeight);
        y2 = y1;
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x1, y1, 0), Red), VertexPositionColor(Vector3(x2, y2, 0), Red));
    }
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

// Render SDR graphs
void Sample::RenderSDRGraphs(const int x, const float viewportWidth, const float viewportHeight)
{
    switch (m_tonemappingMode)
    {
    case TonemappingMode::Reinhard:
        RenderReinhardTonemap(x, viewportWidth, viewportHeight);
        break;

    default:
        break;
    }
}


// Render HDR graphs
void Sample::RenderHDRGraphs(const int x, const float viewportWidth, const float viewportHeight)
{
    Render2084Curve(x, viewportWidth, viewportHeight);

    if (m_displayMappingMode != DisplayMappingMode::None)
    {
        RenderHDRDisplayMapping(x, viewportWidth, viewportHeight);
    }
}


namespace
{
    // Apply a simple Reinhard tonemap operator
    inline float ApplyReinhardTonemapping(float hdrSceneValue) noexcept
    {
        return hdrSceneValue / (1.0f + hdrSceneValue);
    }
}


// Render Reinhard
void Sample::RenderReinhardTonemap(int x, const float viewportWidth, const float viewportHeight)
{
    float x1 = static_cast<float>(x);
    float normalizedLinearValue = static_cast<float>(x) / viewportWidth;
    float linearHDRSceneValue = CalcHDRSceneValueFromNormalizedValue(normalizedLinearValue, m_currentPaperWhiteNits);
    float normalizedNonLinearValue = ApplyReinhardTonemapping(linearHDRSceneValue);
    float y1 = viewportHeight - (normalizedNonLinearValue * viewportHeight);

    float x2 = x1 + 1;
    normalizedLinearValue = static_cast<float>(x2) / viewportWidth;
    linearHDRSceneValue = CalcHDRSceneValueFromNormalizedValue(normalizedLinearValue, m_currentPaperWhiteNits);
    normalizedNonLinearValue = ApplyReinhardTonemapping(linearHDRSceneValue);
    float y2 = viewportHeight - (normalizedNonLinearValue * viewportHeight);

    m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x1, y1, 0), Magenta), VertexPositionColor(Vector3(x2, y2, 0), Magenta));
}


namespace
{
    // Calc Bezier
    inline float CalcBezier(float p0, float p1, float p2, float t) noexcept
    {
        float b0 = (p0 * (1 - t)) + (p1 * t);
        float b1 = (p1 * (1 - t)) + (p2 * t);
        float bezier = (b0 * (1 - t)) + (b1 * t);
        return bezier;
    }
}


// Apply display mapping on the CPU, used for visualizations
float Sample::ApplyHDRDisplayMapping(float normalizedLinearValue)
{
    float ST2084 = LinearToST2084(normalizedLinearValue);

    // If value smaller than max TV brightness, then simply return the ST.2084 value, since these values won't be clipped
    if (m_displayMappingMode == DisplayMappingMode::None ||
        (m_displayMappingData.MaxBrightnessOfHDRScene2084 < m_displayMappingData.MaxBrightnessOfTV2084))
    {
        return ST2084;
    }

    // Use a Bezier curve to apply the soft shoulder
    const float p0 = m_displayMappingData.SoftShoulderStart2084;
    const float p1 = m_displayMappingData.MaxBrightnessOfTV2084;
    const float p2 = m_displayMappingData.MaxBrightnessOfTV2084;
    const float max = m_displayMappingData.MaxBrightnessOfHDRScene2084;

    float mappedValue = ST2084;
    float t;

    t = Clamp((ST2084 - p0) / (max - p0));
    mappedValue = CalcBezier(p0, p1, p2, t);

    // If HDR scene max luminance is too close to shoulders, then it could end up producing a higher value than the ST.2084 curve
    // which will saturate colors, i.e. the opposite of what HDR display mapping should do, therefore always take minimum of the two
    return std::min(mappedValue, ST2084);
}


// Render the HDR display mapping values
void Sample::RenderHDRDisplayMapping(const int x, const float viewportWidth, const float viewportHeight)
{
    float x1 = static_cast<float>(x);
    float normalizedLinearValue = static_cast<float>(x) / viewportWidth;
    float normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
    float y1 = viewportHeight - (normalizedNonLinearValue * viewportHeight);

    float nits = CalcNits(normalizedLinearValue);

    float x2 = x1 + 1;
    normalizedLinearValue = static_cast<float>(x2) / viewportWidth;
    normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
    float y2 = viewportHeight - (normalizedNonLinearValue * viewportHeight);

    float maxNitsOfScene = CalcNits(m_HDRImage[m_currentHDRImage].GetMaxLuminance(), m_currentPaperWhiteNits);

    // Don't render anything after max brightness of scene, just show the flat red line
    if (nits > maxNitsOfScene)
    {
        return;
    }

    if (nits > m_softShoulderStartNits)
    {
        normalizedNonLinearValue = ApplyHDRDisplayMapping(normalizedLinearValue);
        y1 = viewportHeight - (normalizedNonLinearValue * viewportHeight);
        normalizedLinearValue = static_cast<float>(x2) / viewportWidth;
        normalizedNonLinearValue = ApplyHDRDisplayMapping(normalizedLinearValue);
        y2 = viewportHeight - (normalizedNonLinearValue * viewportHeight);
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x1, y1, 0), Lime), VertexPositionColor(Vector3(x2, y2, 0), Lime));
    }
}


// Render the graphs / curves
void Sample::RenderGraphs()
{
    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"RenderGraphs");

    auto oldViewport = m_deviceResources->GetScreenViewport();
	auto oldScissorRect = m_deviceResources->GetScissorRect();

    auto const outputSize = m_deviceResources->GetOutputSize();
    float scale = (outputSize.bottom - outputSize.top) / 1080.0f;

    float viewportWidth = (1920 - 100) * scale;
    float viewportHeight = (1080 - 500) * scale;
    float startX = 50.0f * scale;
    float startY = 100.0f * scale;

    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();

    viewport.TopLeftX = startX;
    viewport.TopLeftY = startY;
    viewport.Width = viewportWidth;
    viewport.Height = viewportHeight;

    scissorRect.left = static_cast<LONG>(viewport.TopLeftX);
    scissorRect.top = static_cast<LONG>(viewport.TopLeftY);
    scissorRect.right = static_cast<LONG>(scissorRect.left + viewport.Width);
    scissorRect.bottom = static_cast<LONG>(scissorRect.top + viewport.Height);

    d3dCommandList->RSSetViewports(1, &viewport);
    d3dCommandList->RSSetScissorRects(1, &scissorRect);

    Matrix proj = Matrix::CreateOrthographicOffCenter(0.0f, viewportWidth, viewportHeight, 0.0f, 0.0f, 1.0f);

    m_lineEffect->SetProjection(proj);
    m_lineEffect->Apply(d3dCommandList);   
    m_primitiveBatch->Begin(d3dCommandList);

    float maxSceneNits = CalcNits(m_HDRImage[m_currentHDRImage].GetMaxLuminance(), m_currentPaperWhiteNits);
    float maxNitsToMapTo = (m_displayMappingMode == DisplayMappingMode::SomeClipping) ? (m_maxBrightnessOfTV + m_nitsAllowedToBeClipped) : m_maxBrightnessOfTV;
    float softShoulderNitsStop = maxNitsToMapTo + ((maxSceneNits - maxNitsToMapTo) / 2);
    
    if (m_bIsTVInHDRMode && !m_bRenderAsSDR)
    {
        // Render line at paper white nits
        float normalizedLinearValue = CalcNormalizedLinearValue(m_currentPaperWhiteNits);
        float normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
        float x = normalizedLinearValue * viewportWidth;
        float y = viewportHeight - (normalizedNonLinearValue * viewportHeight);
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x, viewportHeight, 0), White), VertexPositionColor(Vector3(x, y, 0), White));

        // Render line at TV max brightness
        normalizedLinearValue = CalcNormalizedLinearValue(m_maxBrightnessOfTV);
        normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
        x = normalizedLinearValue * viewportWidth;
        y = viewportHeight - (normalizedNonLinearValue * viewportHeight);
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x, viewportHeight, 0), Red), VertexPositionColor(Vector3(x, y, 0), Red));

        // Render line at start shoulder nits
        normalizedLinearValue = CalcNormalizedLinearValue(m_softShoulderStartNits);
        normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
        x = normalizedLinearValue * viewportWidth;
        y = viewportHeight - (normalizedNonLinearValue * viewportHeight);
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x, viewportHeight, 0), Cyan), VertexPositionColor(Vector3(x, y, 0), Cyan));

        // Render line at  mid point between TV max brightness and scene max brightness
        normalizedLinearValue = CalcNormalizedLinearValue(softShoulderNitsStop);
        normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
        x = normalizedLinearValue * viewportWidth;
        y = viewportHeight - (normalizedNonLinearValue * viewportHeight);
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x, viewportHeight, 0), Magenta), VertexPositionColor(Vector3(x, y, 0), Magenta));

        // Render line at max brightness of scene
        float maxNitsOfScene = CalcNits(m_HDRImage[m_currentHDRImage].GetMaxLuminance(), m_currentPaperWhiteNits);
        normalizedLinearValue = CalcNormalizedLinearValue(maxNitsOfScene);
        normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
        x = normalizedLinearValue * viewportWidth;
        y = viewportHeight - (normalizedNonLinearValue * viewportHeight);
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x, viewportHeight, 0), Yellow), VertexPositionColor(Vector3(x, y, 0), Yellow));

        // Render the lines indication the current selection
        normalizedLinearValue = CalcNormalizedLinearValue(m_maxBrightnessOfTV);
        normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
        x = normalizedLinearValue * viewportWidth;
        y = viewportHeight - (normalizedNonLinearValue * viewportHeight);
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(0.5f, y, 0), White), VertexPositionColor(Vector3(10, y, 0), White));
    }

    // Render the outline
    m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(0.5f, 0.5f, 0), White), VertexPositionColor(Vector3(viewportWidth, 0.5f, 0), White));
    m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(0.5f, viewportHeight, 0), White), VertexPositionColor(Vector3(viewportWidth, viewportHeight, 0), White));
    m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(0.5f, 0.5f, 0), White), VertexPositionColor(Vector3(0.5f, viewportHeight, 0), White));
    m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(viewportWidth, 0.5f, 0), White), VertexPositionColor(Vector3(viewportWidth, viewportHeight, 0), White));

    // Render horizontal tick marks
    constexpr int numSteps = 4;
    for (int i = 0; i < numSteps; i++)
    {
        float x = (i * (viewportWidth / float(numSteps))) + 0.5f;
        float y = viewportHeight;
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x, y, 0), White), VertexPositionColor(Vector3(x, y - 10, 0), White));
    }

    // Render the curve
    for (int x = 0; x < viewportWidth; x++)
    {
        if (m_bIsTVInHDRMode && !m_bRenderAsSDR)
        {
            RenderHDRGraphs(x, viewportWidth, viewportHeight);
        }
        else
        {
            RenderSDRGraphs(x, viewportWidth, viewportHeight);
        }
    }

    m_primitiveBatch->End();
    
    // Restore viewport
    d3dCommandList->RSSetViewports(1, &oldViewport);
    d3dCommandList->RSSetScissorRects(1, &oldScissorRect);

    if (!m_bIsTVInHDRMode || m_bRenderAsSDR)
    {
        PIXEndEvent(d3dCommandList);
        return;
    }

    // Render text
    viewportWidth /= scale;
    viewportHeight /= scale;
    startX /= scale;
    startY /= scale;

    wchar_t strText[2048] = {};
    Vector2 fontPos;
    m_fontBatch->Begin(d3dCommandList);

    fontPos.x = startX - 25;
    fontPos.y = startY + viewportHeight + 5;
    m_textFont->DrawString(m_fontBatch.get(), L"0", fontPos, White, 0.0f, g_XMZero, 0.4f);

    fontPos.x = startX + viewportWidth - 5;
    m_textFont->DrawString(m_fontBatch.get(), L"10K", fontPos, White, 0.0f, g_XMZero, 0.4f);

    float normalizedLinearValue = CalcNormalizedLinearValue(m_maxBrightnessOfTV);
    float normalizedNonLinearValue = LinearToST2084(normalizedLinearValue);
    float x = (normalizedLinearValue * viewportWidth) + 1;
    float y = viewportHeight - (normalizedNonLinearValue * viewportHeight);

    fontPos.x = startX + x - 21;
    fontPos.y = startY + viewportHeight + 5;
    swprintf_s(strText, L"%1.0f", m_maxBrightnessOfTV);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, Red, 0.0f, g_XMZero, 0.4f);
    
    fontPos.x = startX - 25;
    fontPos.y = y + startY + 21;
    swprintf_s(strText, L"%1.0f", m_maxBrightnessOfTV);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, Red, -XM_PIDIV2, g_XMZero, 0.4f);

    fontPos.x = startX + viewportWidth - 80;
    fontPos.y = startY + viewportHeight - 25;
    m_textFont->DrawString(m_fontBatch.get(), L"Input", fontPos, Gray, 0.0f, g_XMZero, 0.4f);

    fontPos.x = 55;
    fontPos.y = 180;
    m_textFont->DrawString(m_fontBatch.get(), L"Output", fontPos, Gray, -XM_PIDIV2, g_XMZero, 0.4f);

    normalizedLinearValue = CalcNormalizedLinearValue(m_HDRImage[m_currentHDRImage].GetMaxLuminance(), m_currentPaperWhiteNits);
    x = (normalizedLinearValue * viewportWidth) + 1;

    fontPos.x = startX + x - 21;
    fontPos.y = startY + viewportHeight + 5;
    swprintf_s(strText, L"%1.0f", CalcNits(normalizedLinearValue));
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, Yellow, 0.0f, g_XMZero, 0.4f);

    m_fontBatch->End();

    constexpr int c_numInputs = 7;
    const wchar_t* inputName[c_numInputs] = {
        L" Black",
        L" White",
        L"",
        L"Shoulder",
        L"Max Nits\n of TV",
        L"",
        L"Max Nits\nHDR Scene", };

    float inputNits[c_numInputs] =
    {
        CalcNits(0.0f, m_currentPaperWhiteNits),
        CalcNits(1.0f, m_currentPaperWhiteNits),
        (m_currentPaperWhiteNits < m_softShoulderStartNits) ? (m_softShoulderStartNits - CalcNits(1.0f, m_currentPaperWhiteNits)) / 2 : (CalcNits(1.0f, m_currentPaperWhiteNits) - m_softShoulderStartNits) / 2,
        m_softShoulderStartNits,
        m_maxBrightnessOfTV,
        (CalcNits(m_HDRImage[m_currentHDRImage].GetMaxLuminance(), m_currentPaperWhiteNits) - m_maxBrightnessOfTV) / 2,
        CalcNits(m_HDRImage[m_currentHDRImage].GetMaxLuminance(), m_currentPaperWhiteNits)
    };

    XMVECTOR colors[c_numInputs] = { Gray, White, White, Cyan, Red, Magenta, Yellow };

    viewport.TopLeftX += 125 * scale;
    viewport.TopLeftY = viewport.Height + 140 * scale;
    viewport.Width -= 150 * scale;
    viewport.Height = 110 * scale;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    scissorRect.left = static_cast<LONG>(viewport.TopLeftX);
    scissorRect.top = static_cast<LONG>(viewport.TopLeftY);
    scissorRect.right = static_cast<LONG>(scissorRect.left + viewport.Width);
    scissorRect.bottom = static_cast<LONG>(scissorRect.top + viewport.Height);

    // Render brightness colors at bottom
    d3dCommandList->RSSetViewports(1, &viewport);
    d3dCommandList->RSSetScissorRects(1, &scissorRect);

    float height = viewport.Height;
    int step = static_cast<int>(viewport.Width / 6);
    float tickMarkLength = 10;
    x = 1;

    proj = Matrix::CreateOrthographicOffCenter(0.0f, viewport.Width, viewport.Height, 0.0f, 0.0f, 1.0f);
    m_lineEffect->SetProjection(proj);
    m_lineEffect->Apply(d3dCommandList);
    m_primitiveBatch->Begin(d3dCommandList);

    for (size_t i = 0; i < c_numInputs - 1; i++)
    {
        float hdrSceneValue = CalcHDRSceneValue(inputNits[i], m_currentPaperWhiteNits);
        XMVECTOR startColor = MakeColor(hdrSceneValue);
        hdrSceneValue = CalcHDRSceneValue(inputNits[i + 1], m_currentPaperWhiteNits);
        XMVECTOR stopColor = MakeColor(hdrSceneValue);
        for (y = 0; y < viewport.Height - tickMarkLength; y++)
        {
            m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x, y, 0), startColor), VertexPositionColor(Vector3(x + step, y, 0), stopColor));
        }
        m_primitiveBatch->DrawLine(VertexPositionColor(Vector3(x, height - 10, 0), White), VertexPositionColor(Vector3(x, height - 1, 0), White));

        x += step;
    }

    m_primitiveBatch->End();

    // Restore viewport
    d3dCommandList->RSSetViewports(1, &oldViewport);
    d3dCommandList->RSSetScissorRects(1, &oldScissorRect);

    constexpr float fontScale = 0.4f;
    step /= int(scale);

    m_fontBatch->Begin(d3dCommandList);

    fontPos.x = startX + 85;
    fontPos.y = startY + viewportHeight + 155;

    for (size_t i = 0; i < c_numInputs; i++)
    {
        normalizedLinearValue = CalcNormalizedLinearValue(inputNits[i]);
        normalizedNonLinearValue = ApplyHDRDisplayMapping(normalizedLinearValue);
        normalizedLinearValue = ST2084ToLinear(normalizedNonLinearValue);
        swprintf_s(strText, L"%s", inputName[i]);
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, colors[i], 0.0f, g_XMZero, fontScale);
        fontPos.x += step;
    }

    fontPos.x = startX - 30;
    fontPos.y = startY + viewportHeight + 200;
    m_textFont->DrawString(m_fontBatch.get(), L"Input Nits\nOutput Nits", fontPos, White, 0.0f, g_XMZero, fontScale);

    fontPos.x = startX + 110;

    for (size_t i = 0; i < c_numInputs; i++)
    {
        normalizedLinearValue = CalcNormalizedLinearValue(inputNits[i]);
        normalizedNonLinearValue = ApplyHDRDisplayMapping(normalizedLinearValue);
        normalizedLinearValue = ST2084ToLinear(normalizedNonLinearValue);
        float outputNits = CalcNits(normalizedLinearValue);
        swprintf_s(strText, L"%1.0f\n%1.0f", inputNits[i], outputNits);
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, colors[i], 0.0f, g_XMZero, fontScale);
        fontPos.x += step;
    }

    m_fontBatch->End();

    int size = 10;

    viewport.Width /= scale;
    viewport.Height /= scale;
    viewport.TopLeftX /= scale;
    viewport.TopLeftY /= scale;
    tickMarkLength /= int(scale);

    // SpriteBatch requires a texture, otherwise it will assert, but we just want to draw a color, so give it a dummy texture
    XMUINT2 dummyTextureSize = { 1, 1 };
    auto dummyTexture = m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::HDRScene);

    m_spriteBatch->Begin(d3dCommandList);

    // Render blocks at 10,000 nits nits
    for (int i = 1; i <= viewport.Width; i += step)
    {
        x = viewport.TopLeftX + i + 1;
        RECT position = { static_cast<long>(x - size), static_cast<long>(viewport.TopLeftY), static_cast<long>(x), static_cast<long>(viewport.TopLeftY + viewport.Height - tickMarkLength - 1) };
        float hdrSceneValue = CalcHDRSceneValue(c_MaxNitsFor2084, m_currentPaperWhiteNits);
        XMVECTOR hdrSceneColor = MakeColor(hdrSceneValue);
        m_spriteBatch->Draw(dummyTexture, dummyTextureSize, position, hdrSceneColor);
    }

    m_spriteBatch->End();

    PIXEndEvent(d3dCommandList); 
}


// Render the calibration block with 10,000 nits on the outside, and the current max brightness
// of the display on the inside
void Sample::RenderCalibrationBlock(int startX, int startY, int size, float nits)
{
    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"RenderCalibrationBlock");

    // SpriteBatch requires a texture, otherwise it will assert, but we just want to draw a color, so give it a dummy texture
    XMUINT2 dummyTextureSize = { 1, 1 };
    auto dummyTexture = m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::HDRScene);

    m_spriteBatch->Begin(d3dCommandList);

    // Render block at 10,000 nits nits
    {
        RECT position = { startX, startY, startX + size, startY + size };
        float hdrSceneValue = CalcHDRSceneValue(c_MaxNitsFor2084, m_currentPaperWhiteNits);
        XMVECTOR hdrSceneColor = MakeColor(hdrSceneValue);
        m_spriteBatch->Draw(dummyTexture, dummyTextureSize, position, hdrSceneColor);
    }

    // Render block at current display brightness
    {
        RECT position = { startX + size / 3, startY + size / 3, startX + size - size / 3, startY + size - size / 3 };
        float hdrSceneValue = CalcHDRSceneValue(nits, m_currentPaperWhiteNits);
        XMVECTOR hdrSceneColor = MakeColor(hdrSceneValue);
        m_spriteBatch->Draw(dummyTexture, dummyTextureSize, position, hdrSceneColor);
    }

    m_spriteBatch->End();

    startX = 50;
    startY = 40;
    const float fontScale = 0.75f;
    wchar_t strText[2048];
    Vector2 fontPos(static_cast<float>(startX), static_cast<float>(startY));

    m_fontBatch->Begin(d3dCommandList);
    fontPos.y = startY + 290.0f;
    m_textFont->DrawString(m_fontBatch.get(), L"Adjust brightness of inside block to\nmatch brightness of outside block to\ndetermine TV max brightness", fontPos, White, 0.0f, g_XMZero, fontScale);
    fontPos.y += 200;
    swprintf_s(strText, L"Max Brightness: %1.0f Nits\n\nMax Brightness Returned by the\nXbox System HDRGameCalibration app: %1.0f Nits", m_maxBrightnessOfTV, m_maxBrightnessReturnedBySystemCalibration);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, fontScale);
    fontPos.y += 200;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"Press [A] when done", fontPos, White, 0.65f);
    m_fontBatch->End();

    PIXEndEvent(d3dCommandList);
}


// Render text with dark gray shadow which helps when text is on top of bright areas on the image
void Sample::DrawStringWithShadow(const wchar_t* string, Vector2& fontPos, FXMVECTOR color, float fontScale)
{
    fontPos.x += 1;
    fontPos.y += 1.0f;
    m_textFont->DrawString(m_fontBatch.get(), string, fontPos, DarkSlateGray, 0.0f, g_XMZero, fontScale);
    fontPos.x -= 1;
    fontPos.y -= 1.0f;
    m_textFont->DrawString(m_fontBatch.get(), string, fontPos, color, 0.0f, g_XMZero, fontScale);
}


// Render the UI
void Sample::RenderUI()
{
    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"RenderUI");

    constexpr float startX = 50.0f;
    constexpr float startY = 40.0f;
    float fontScale = 0.75f;

    wchar_t strText[2048] = {};

    Vector2 fontPos(startX, startY);

    m_fontBatch->Begin(d3dCommandList);
    DrawStringWithShadow(L"HDR Display Mapping Sample", fontPos, White, 1.0f);

    if (!m_bRenderGraphs)
    {
        fontPos.y = startY + 100.0f;
        if (m_bIsTVInHDRMode)
        {
            DrawStringWithShadow(L"TV in HDR Mode: TRUE", fontPos, White, fontScale);
        }
        else
        {
            DrawStringWithShadow(L"TV in HDR Mode: FALSE", fontPos, White, fontScale);
        }
    }

    if (!m_bCalibrateTV)
    {
        if (m_bRenderGraphs)
        {
            fontScale = 0.5f;
            fontPos.x = 120.0f;
            fontPos.y = 120.0f;
        }
        else
        {
            fontPos.y += 40.0f;
        }

        if (m_bIsTVInHDRMode && !m_bRenderAsSDR)
        {
            switch (m_displayMappingMode)
            {
            case DisplayMappingMode::None:
                swprintf_s(strText, L"Paper White: %1.0f nits\nDisplay Mapping: None\n", m_currentPaperWhiteNits);
                break;

            case DisplayMappingMode::NoClipping:
                swprintf_s(strText, L"Paper White: %1.0f nits\nDisplay Mapping: No Clipping\nSoft shoulder start: %1.0f nits", m_currentPaperWhiteNits, m_softShoulderStartNits);
                break;

            case DisplayMappingMode::SomeClipping:
                swprintf_s(strText, L"Paper White: %1.0f nits\nDisplay Mapping: With %1.0f nits Clipping\nSoft shoulder start: %1.0f nits", m_currentPaperWhiteNits, m_nitsAllowedToBeClipped, m_softShoulderStartNits);
                break;

            default:
                break;
            }
            DrawStringWithShadow(strText, fontPos, White, fontScale);
        }
        else
        {
            switch (m_tonemappingMode)
            {
            case TonemappingMode::None:
                DrawStringWithShadow(L"Tone Mapping: None", fontPos, White, fontScale);
                break;

            case TonemappingMode::Reinhard:
                DrawStringWithShadow(L"Tone Mapping: Reinhard", fontPos, White, fontScale);
                break;

            default:
                break;
            }
        }

        int step = static_cast<int>(((1920.0f - 2 * startX) / 4.0f));
        fontPos.x = startX;
        fontPos.y = 990;
        DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[A] - Mapping methods", fontPos, White, 0.65f); fontPos.y += 35;

        if (m_bIsTVInHDRMode)
        {
            if (m_bRenderAsSDR)
            {
                DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[B] - Render as HDR", fontPos, White, 0.65f);
            }
            else
            {
                DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[B] - Render as SDR", fontPos, White, 0.65f);
            }
        }

        fontPos.x = startX + step;
        fontPos.y = 990;
        if (m_bIndicateValuesBrighterThanTV)
        {
            DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[Y] - Show image", fontPos, White, 0.65f); fontPos.y += 35;
        }
        else
        {
            DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[Y] - Show clipped", fontPos, White, 0.65f); fontPos.y += 35;
        }

        if (m_bRenderGraphs)
        {
            DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[X] - Show image", fontPos, White, 0.65f);
        }
        else
        {
            DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[X] - Show graphs", fontPos, White, 0.65f);
        }

        if (m_bIsTVInHDRMode && !m_bRenderAsSDR)
        {
            fontPos.x = startX + step + step - 50;
            fontPos.y = 955 + 35;
            DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[DPad] - L/R Adjust shoulder", fontPos, White, 0.65f); fontPos.y += 35;
            DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[DPad] - U/D Adjust clipped nits", fontPos, White, 0.65f); fontPos.y += 35;
        }

        fontPos.x = startX + step + step + step;
        fontPos.y = 990;
        DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[RB] - Next image", fontPos, White, 0.65f); fontPos.y += 35;
        DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[LB] - Prev image", fontPos, White, 0.65f);
    }

    m_fontBatch->End();

    PIXEndEvent(d3dCommandList);
}


// Clear scene
void Sample::Clear()
{
    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"Clear");

    m_hdrScene->TransitionTo(d3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto const rtv = m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::HDRSceneRTV);
    d3dCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    // Use linear clear color for gamma-correct rendering.
    d3dCommandList->ClearRenderTargetView(m_deviceResources->GetRenderTargetView(), Black, 0, nullptr);
    d3dCommandList->ClearRenderTargetView(m_deviceResources->GetGameDVRRenderTargetView(), Black, 0, nullptr);
    d3dCommandList->ClearRenderTargetView(rtv, Black, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    d3dCommandList->RSSetViewports(1, &viewport);
    d3dCommandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(d3dCommandList);
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

        m_bIsTVInHDRMode = (result == XDisplayHdrModeResult::Enabled);

#ifdef _DEBUG
        OutputDebugStringA((m_bIsTVInHDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif
    }
}

// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto d3dDevice = m_deviceResources->GetD3DDevice();
    m_graphicsMemory = std::make_unique<GraphicsMemory>(d3dDevice);

    m_fullScreenQuad = std::make_unique<FullScreenQuad>();
    m_fullScreenQuad->Initialize(m_deviceResources->GetD3DDevice());

    ResourceUploadBatch resourceUpload(d3dDevice);
    resourceUpload.Begin();

    // Create descriptor heaps
    m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RTVDescriptors::CountRTV);
    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(d3dDevice, ResourceDescriptors::Count);

    // Init fonts
    const RenderTargetState rtState(m_hdrScene->GetFormat(), m_deviceResources->GetDepthBufferFormat());
    InitializeSpriteFonts(d3dDevice, resourceUpload, rtState);

    // SpriteBatch for rendering HDR values into the backbuffer
    {
        auto pixelShaderBlob = DX::ReadData(L"ColorPS.cso");
        SpriteBatchPipelineStateDescription pd(rtState);
        pd.customPixelShader.BytecodeLength = pixelShaderBlob.size();
        pd.customPixelShader.pShaderBytecode = pixelShaderBlob.data();
        m_spriteBatch = std::make_unique<SpriteBatch>(d3dDevice, resourceUpload, pd);
    }

    // PrimitiveBatch for rendering lines into the backbuffer
    {       
        D3D12_RASTERIZER_DESC state = CommonStates::CullNone;
        state.MultisampleEnable = FALSE;
        EffectPipelineStateDescription pd(&VertexPositionColor::InputLayout, CommonStates::Opaque, CommonStates::DepthNone, state, rtState, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
        m_lineEffect = std::make_unique<BasicEffect>(d3dDevice, EffectFlags::VertexColor, pd);
        m_primitiveBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(d3dDevice);
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
        DX::ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dRenderHDRImagePSO.ReleaseAndGetAddressOf())));
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
        DX::ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dPrepareSwapChainBufferPSO.ReleaseAndGetAddressOf())));
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
    m_hdrScene->SetDevice(d3dDevice, m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::HDRScene), m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::HDRSceneRTV));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Create HDR backbuffer resources.
    auto const outputSize = m_deviceResources->GetOutputSize();
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
