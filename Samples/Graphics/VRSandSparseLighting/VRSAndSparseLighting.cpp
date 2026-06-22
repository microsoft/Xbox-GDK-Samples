//--------------------------------------------------------------------------------------
// VRSAndSparseLighting.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "VRSAndSparseLighting.h"

#include "ATGColors.h"
#include "FindMedia.h"


#pragma warning(disable : 5038)
#pragma warning(disable : 4061)


extern void ExitSample() noexcept;

namespace
{
    constexpr float c_nearZ = 0.1f;
    constexpr float c_farZ = 1000.0f;
    constexpr float c_depthRange = c_farZ / (c_farZ - c_nearZ);
    constexpr float c_depthRangeNegNearZ = -c_nearZ * c_depthRange;
}

using namespace DirectX;
using namespace SimpleMath;

using Microsoft::WRL::ComPtr;
using DirectX::Colors::Black;
using DirectX::Colors::White;



Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_position(-56.8390427f, 12.7207355f, -16.0951595f)
    , m_pitch(-0.0675464123f)
    , m_yaw(-1.82958961f)
    , m_sunSpeed(SunSpeed::Stopped)
    , m_sunHeight(0.1f)
    , m_viewUI(true)
    , m_shadingRateTolerance(0.075f)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
#ifdef _GAMING_XBOX_SCARLETT
        DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
#else
        DXGI_FORMAT_B8G8R8A8_UNORM,
#endif
        DXGI_FORMAT_D32_FLOAT,
        2);
    m_deviceResources->RegisterDeviceNotify(this);

    m_dpadUI.Initialize(DPADMenu::Count, 7);
    m_dpadUI.AddRow(L"       Force Ground Truth", 0, true, 0);
    m_dpadUI.AddRow(L"    Variable Rate Shading", 0, true, 1);
    m_dpadUI.AddRow(L"          Sparse Lighting", 0, true, 1);
    m_dpadUI.AddRow(L" Sparse Buffer Generation", Terrain::SparseBufferGeneration::Count);
    m_dpadUI.SetFieldNameLastAddedRow(L"Combined With Depth Decompress", Terrain::SparseBufferGeneration::CombinedDepthDecompress);
    m_dpadUI.SetFieldNameLastAddedRow(L"Stand Alone", Terrain::SparseBufferGeneration::StandAlone);

    m_dpadUI.AddRow(L"Sparse Lighting Tile Size", Terrain::SparseLightingTileSize::Count, false, Terrain::SparseLightingTileSize::TileSize8x8fromHTile);
    m_dpadUI.SetFieldNameLastAddedRow(L"2x2", Terrain::SparseLightingTileSize::TileSize2x2);
    m_dpadUI.SetFieldNameLastAddedRow(L"8x8 pulled from HTile", Terrain::SparseLightingTileSize::TileSize8x8fromHTile);

    m_dpadUI.AddRow(L"Depth Discontinuity Check", 0, true, 1);

    m_dpadUI.AddRow(L"  Post Process De-blocker", Terrain::DeblockerOptions::Count, false, Terrain::DeblockerOptions::DeblockerOff);
    m_dpadUI.SetFieldNameLastAddedRow(L"Off", Terrain::DeblockerOptions::DeblockerOff);
    m_dpadUI.SetFieldNameLastAddedRow(L"One pass Deblock and Tonemap (With Sharpen)", Terrain::DeblockerOptions::DeblockAndToneMap);
    m_dpadUI.SetFieldNameLastAddedRow(L"Deblock Standalone Pass (No Sharpen)", Terrain::DeblockerOptions::DeblockStandAlonePass);

    m_dpadUI.AddRow(L"         Rotate Lit Pixel", Terrain::RotateLitPixelOptions::Count, false, Terrain::RotateLitPixelOptions::Off);
    m_dpadUI.SetFieldNameLastAddedRow(L"Off", Terrain::RotateLitPixelOptions::Off);
    m_dpadUI.SetFieldNameLastAddedRow(L"On", Terrain::RotateLitPixelOptions::On);
    m_dpadUI.SetFieldNameLastAddedRow(L"Advanced Once Per Second", Terrain::RotateLitPixelOptions::OncePerSecond);

    m_dpadUI.AddRow(L"      Debug Visualization", Terrain::Visualise::Count);
    m_dpadUI.SetFieldNameLastAddedRow(L"None", Terrain::Visualise::Normal);
    m_dpadUI.SetFieldNameLastAddedRow(L"VRS Shading Rate Overlay", Terrain::Visualise::VRSShadingRateOverlay);
    m_dpadUI.SetFieldNameLastAddedRow(L"Sparse Lighting Shading Rate Overlay", Terrain::Visualise::SparseLightingShadingRateOverlay);
    m_dpadUI.SetFieldNameLastAddedRow(L"No Hole Filling", Terrain::Visualise::NoHoleFilling);
    m_dpadUI.SetFieldNameLastAddedRow(L"Lighting Count Buffer", Terrain::Visualise::LightingCountBuffer);
    m_dpadUI.SetFieldNameLastAddedRow(L"Coverage", Terrain::Visualise::Coverage);

    m_dpadUI.AddRow(L"               Zoom Level", Terrain::ZoomLevel::Count);
    m_dpadUI.SetFieldNameLastAddedRow(L"None", Terrain::ZoomLevel::None);
    m_dpadUI.SetFieldNameLastAddedRow(L"x2", Terrain::ZoomLevel::x2);
    m_dpadUI.SetFieldNameLastAddedRow(L"x4", Terrain::ZoomLevel::x4);
    m_dpadUI.SetFieldNameLastAddedRow(L"x8", Terrain::ZoomLevel::x8);
    m_dpadUI.SetFieldNameLastAddedRow(L"x16", Terrain::ZoomLevel::x16);
}


Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}


// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_terrain.Initialize(m_deviceResources, m_graphicsMemory);
    m_gpuTimer.RestoreDevice(m_deviceResources->GetD3DDevice(), m_deviceResources->GetCommandQueue());
}


#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());
    float timeScale = elapsedTime / float(Terrain::ZoomLevel::Enum(m_dpadUI.GetRowSelection(DPADMenu::ZoomLevel)) + 1);

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
              ExitSample();
        }
        m_yaw += pad.thumbSticks.rightX * timeScale;
        m_pitch += pad.thumbSticks.rightY * timeScale;
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
          ExitSample();
    }
    // Limit to avoid looking directly up or down
    float limit = XM_PI / 2.0f - 0.01f;
    m_pitch = std::max(-limit, m_pitch);
    m_pitch = std::min(+limit, m_pitch);

    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }
    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMVECTOR camForwardVector = XMVectorSet(
        sinf(m_yaw),
        m_pitch,
        cosf(m_yaw),
        0);

    XMMATRIX camera = XMMatrixLookToLH(pos, camForwardVector, g_XMIdentityR1);

    if (pad.IsConnected())
    {
        if (!pad.IsLeftStickPressed())
        {
            float speed = 10.0f * timeScale;

            XMMATRIX invCamera = XMMatrixTranspose(camera);
            float tempF = pad.thumbSticks.leftX * speed * 1.5f;     pos = XMVectorAdd(pos, XMVectorMultiply(invCamera.r[0], XMVectorSet(tempF, tempF, tempF, 1.0)));
            tempF = pad.thumbSticks.leftY * speed * 1.5f;           pos = XMVectorAdd(pos, XMVectorMultiply(invCamera.r[2], XMVectorSet(tempF, tempF, tempF, 1.0)));
/*
            float y = pad.IsDPadDownPressed() ? -speed : 0.0f;
            y += pad.IsDPadUpPressed() ? speed : 0.0f;
            XMVECTOR yOffset = XMVectorSet(0.0f, y, 0.0f, 0.0f);
            m_position = XMVectorAdd(pos, yOffset);
*/
            m_position = pos;
        }
        if (pad.IsLeftShoulderPressed())
        {
            m_sunHeight -= elapsedTime * 0.33f;
            m_sunHeight = std::max(m_sunHeight, -0.05f);
        }
        if (pad.IsRightShoulderPressed())
        {
            m_sunHeight += elapsedTime * 0.33f;
            m_sunHeight = std::min(m_sunHeight, 0.4f);
        }
        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_sunSpeed = SunSpeed::Enum((UINT(m_sunSpeed) + 1) % SunSpeed::Count);
        }
        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_viewUI = !m_viewUI;
        }
        static float toleranceSpeed = 0.001f;

        float &shadingRateTolerance = m_shadingRateTolerance;
        shadingRateTolerance -= pad.triggers.right * toleranceSpeed;
        shadingRateTolerance += pad.triggers.left * toleranceSpeed;
        shadingRateTolerance = std::min(shadingRateTolerance, 1.0f);
        shadingRateTolerance = std::max(shadingRateTolerance, 0.0f);

        m_dpadUI.Update(
            m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::ButtonState::PRESSED,
            m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::ButtonState::PRESSED,
            m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::ButtonState::PRESSED,
            m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::ButtonState::PRESSED
        );
    }
    // have to recalculate matrix with new camera position, otherwise camera position and matrix don't match, which is bad!
    // is is likely possible to patch the matrices with the new camera position at less cost
    camera = XMMatrixLookToLH(m_position, camForwardVector, g_XMIdentityR1);
    m_viewProj = XMMatrixMultiply(camera, m_proj);
    m_view = camera;
}
#pragma endregion


#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    auto commandList = m_deviceResources->GetCommandList();
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Terrain Render");
    m_gpuTimer.BeginFrame(commandList);

    static float sunSpeed[SunSpeed::Count] = { 0.0f, 1.0f, 10.0f };

    bool forceGroundTruth = bool(m_dpadUI.GetRowSelection(DPADMenu::GroundTruth));
    bool vrsEnabled                                                 = forceGroundTruth ? false : bool(m_dpadUI.GetRowSelection(DPADMenu::VRS));
    bool sparseLightingEnabled                                      = forceGroundTruth ? false : bool(m_dpadUI.GetRowSelection(DPADMenu::SparseLighting));
    Terrain::Visualise::Enum debugVisualisation                     = forceGroundTruth ? Terrain::Visualise::Normal : Terrain::Visualise::Enum(m_dpadUI.GetRowSelection(DPADMenu::DebugVisualiation));
    Terrain::SparseLightingTileSize::Enum sparseLightingTileSize    = vrsEnabled       ? Terrain::SparseLightingTileSize::Enum(m_dpadUI.GetRowSelection(DPADMenu::SparseLightingTileSize)) : Terrain::SparseLightingTileSize::TileSize2x2;
    Terrain::DeblockerOptions::Enum deblockerOptions                = forceGroundTruth ? Terrain::DeblockerOptions::DeblockerOff : Terrain::DeblockerOptions::Enum(m_dpadUI.GetRowSelection(DPADMenu::DeblockerOptions));
    Terrain::RotateLitPixelOptions::Enum rotateLitPixelOptions      = forceGroundTruth ? Terrain::RotateLitPixelOptions::Off : Terrain::RotateLitPixelOptions::Enum(m_dpadUI.GetRowSelection(DPADMenu::RotateLitPixelOptions));

    if(!forceGroundTruth)
        m_dpadUI.SetRowSelection(DPADMenu::SparseLightingTileSize, UINT8(sparseLightingTileSize));

    m_terrain.Render(
        m_proj,
        m_view,
        m_viewProj,
        c_farZ,
        c_depthRange,
        c_depthRangeNegNearZ,
        m_position,
        m_deviceResources,
        m_graphicsMemory,
        float(m_timer.GetElapsedSeconds()) * sunSpeed[m_sunSpeed],
        m_gpuTimer,
        m_shadingRateTolerance,
        m_sunHeight,
        vrsEnabled,
        sparseLightingEnabled,
        Terrain::SparseBufferGeneration::Enum(m_dpadUI.GetRowSelection(DPADMenu::SparseBufferGeneration)),
        sparseLightingTileSize,
        bool(m_dpadUI.GetRowSelection(DPADMenu::DepthDiscontinuityCheck)),
        deblockerOptions,
        rotateLitPixelOptions,
        debugVisualisation,
        Terrain::ZoomLevel::Enum(m_dpadUI.GetRowSelection(DPADMenu::ZoomLevel))
        );

    m_gpuTimer.EndFrame(commandList);
    PIXEndEvent(commandList);

    if (m_viewUI)
    {
        // Switch to back buffer and UI rendering
        auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
        auto dsvDescriptor = m_deviceResources->GetDepthStencilView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

        ID3D12DescriptorHeap* heaps[] = { m_uiDescriptorHeap->Heap() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);

        auto viewportUI = m_deviceResources->GetScreenViewport();
        m_fontBatch->SetViewport(viewportUI);
        m_fontBatch->Begin(commandList);

        // Render the text
        wchar_t strText[2048] = {};
        float fontScale = 1.0f;

        switch (XSystemGetDeviceType())
        {
        default:
            break;
        case XSystemDeviceType::XboxScarlettLockhart:
            fontScale = 0.75f;      // smaller render target
            break;
        }
        float ySpace = 40.0f * fontScale;
        float ySpace2 = ySpace * 2.0f;

        Vector2 fontPos(50.0f, 40.0f);
        Vector2 fontPosTab(60.0f, 40.0f);

        const XMVECTORF32& colour = DirectX::Colors::PapayaWhip;

        swprintf_s(strText, L"Resolution %ux%u, 2048x2048 Heightmap, Raymarched Soft Shadows", uint32_t(viewport.Width), uint32_t(viewport.Height));
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
        m_textFont->DrawString(m_fontBatch.get(), L"Shading rate calculated from last frame, no motion vectors", fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;

        swprintf_s(strText, L"<Triggers> Shading Rate Tolerance: %1.3f", m_shadingRateTolerance);
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;

        swprintf_s(strText, L"<Y><Shoulders> Control Sun");
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;

        swprintf_s(strText, L"<X> Toggle UI");
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
        fontPos.y += ySpace; fontPosTab.y += ySpace;

        // wait until we have some valid timing data
        if (m_frame > 3)
        {
            float clearDepthMs = m_gpuTimer.GetAverageMS(Terrain::GPUPasses::ClearDepth);
            float setShadingRateMs = m_gpuTimer.GetAverageMS(Terrain::GPUPasses::VRSSetShadingRate);
            float gbufferMs = m_gpuTimer.GetAverageMS(Terrain::GPUPasses::GBuffer);
            float depthDecompressMs = m_gpuTimer.GetAverageMS(Terrain::GPUPasses::DepthDecompressAndSparseBufferGeneration);
            float deferredLightingMs = m_gpuTimer.GetAverageMS(Terrain::GPUPasses::DeferredLighting);
            float skyMs = m_gpuTimer.GetAverageMS(Terrain::GPUPasses::Sky);
            float calcShadingRateMs = m_gpuTimer.GetAverageMS(Terrain::GPUPasses::CalcShadingRate);
            float deblockerMs = m_gpuTimer.GetAverageMS(Terrain::GPUPasses::Deblocker);
            float toneMapMs = m_gpuTimer.GetAverageMS(Terrain::GPUPasses::ToneMap);

            if (deblockerOptions != Terrain::DeblockerOptions::DeblockStandAlonePass)
            {
                deblockerMs = 0.0f;
            }
            if (!vrsEnabled)
            {
                setShadingRateMs = 0.0;
            }
            if (!(vrsEnabled || sparseLightingEnabled))
            {
                calcShadingRateMs = 0.0;
            }
            float totalTimeMs = clearDepthMs + setShadingRateMs + gbufferMs + depthDecompressMs + deferredLightingMs + skyMs + calcShadingRateMs;

            swprintf_s(strText, L"Total time: %3.3fms", totalTimeMs);                                                                                                       m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
            swprintf_s(strText, L"Clear Depth buffer: %3.3fms", clearDepthMs);                                                                                              m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
            swprintf_s(strText, L"Set Shading Rate: %3.3fms", setShadingRateMs);                                                                                            m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
            swprintf_s(strText, L"Render GBuffer: %3.3fms", gbufferMs);                                                                                                     m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
            swprintf_s(strText, sparseLightingEnabled ? L"Depth Decompress & Build Sparse Buffers: %3.3fms" : L"Depth Decompress: %3.3fms", depthDecompressMs);             m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
            swprintf_s(strText, L"Deferred Lighting: %3.3fms", deferredLightingMs);                                                                                         m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
            swprintf_s(strText, L"Sky Render: %3.3fms", skyMs);                                                                                                             m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
            swprintf_s(strText, L"Stand alone de-blocker pass: %3.3fms", deblockerMs);                                                                                      m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
            swprintf_s(strText, deblockerOptions == Terrain::DeblockerOptions::DeblockAndToneMap ? L"De-block and Tone Map: %3.3fms" : L"Tone Map: %3.3fms", toneMapMs);    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
            swprintf_s(strText, L"Calculate Shading Rate: %3.3fms", calcShadingRateMs);                                                                                     m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += ySpace; fontPosTab.y += ySpace;
        }
        fontPos.y += ySpace2; fontPosTab.y += ySpace2;

        m_textFont->DrawString(m_fontBatch.get(), "DPAD Menu", fontPos, colour, 0.0f, g_XMZero, fontScale);
        fontPos.y += ySpace; fontPosTab.y += ySpace;
        {
            UINT numRows = m_dpadUI.GetNumRows();
            UINT selectedRow = m_dpadUI.GetSelectedRow();
            for (UINT i = 0; i < numRows; ++i)
            {
                if (i == selectedRow)
                    swprintf_s(strText, L"%s: [%s]", m_dpadUI.GetRowName(i), m_dpadUI.GetRowSelectionFieldName(i));
                else
                    swprintf_s(strText, L"%s: %s", m_dpadUI.GetRowName(i), m_dpadUI.GetRowSelectionFieldName(i));

                m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale);
                fontPos.y += ySpace; fontPosTab.y += ySpace;
            }
        }
        if ((debugVisualisation == Terrain::Visualise::VRSShadingRateOverlay) || (debugVisualisation == Terrain::Visualise::SparseLightingShadingRateOverlay))
        {
            fontPos.y += ySpace2; fontPosTab.y += ySpace2;
            m_textFont->DrawString(m_fontBatch.get(), L"Shading Rate Key:", fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
            m_textFont->DrawString(m_fontBatch.get(), L"Red: 1x1, full rate", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
            m_textFont->DrawString(m_fontBatch.get(), L"Cyan: 2x1, half rate, more detail vertically", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
            m_textFont->DrawString(m_fontBatch.get(), L"Yellow: 1x2, half rate, more detail horizontally", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
            m_textFont->DrawString(m_fontBatch.get(), L"No tint: 2x2, quarter rate", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
        }
        if (debugVisualisation == Terrain::Visualise::LightingCountBuffer)
        {
            fontPos.y += ySpace2; fontPosTab.y += ySpace2;
            m_textFont->DrawString(m_fontBatch.get(), L"Count Map Key:", fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
            m_textFont->DrawString(m_fontBatch.get(), L"Black                - 0 lighting calculations :)", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
            m_textFont->DrawString(m_fontBatch.get(), L"Bright Green         - 1x wave32 (1  - 32 calculations)", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
            m_textFont->DrawString(m_fontBatch.get(), L"Dark Green           - 2x wave32 (33 - 64 calculations)", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
            m_textFont->DrawString(m_fontBatch.get(), L"Light Blue -> Yellow - 3-7 waves (65 - 224 calculations)", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
            m_textFont->DrawString(m_fontBatch.get(), L"Red                  - 8x wave32 (256 calculations, regular deferred, no sparse coordinates)", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
        }
        m_fontBatch->End();
    }
    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
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
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}


void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}


void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}


// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1280;
    height = 720;
}
#pragma endregion


#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    // Create descriptor heap for resources
    m_uiDescriptorHeap = std::make_unique<DescriptorHeap>(device, static_cast<UINT>(UIDescriptors::Count));

    // Init fonts
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();
    {
        const SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
        m_fontBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

        auto cpuDescHandleText = m_uiDescriptorHeap->GetCpuHandle(static_cast<int>(UIDescriptors::TextFont));
        auto gpuDescHandleText = m_uiDescriptorHeap->GetGpuHandle(static_cast<int>(UIDescriptors::TextFont));
        m_textFont = std::make_unique<SpriteFont>(device, resourceUpload, L"Courier_36.spritefont", cpuDescHandleText, gpuDescHandleText);
    }
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}


// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Initialize the projection matrix.
    auto const size = m_deviceResources->GetOutputSize();
    const float aspect = float(size.right) / float(size.bottom);

    // note swapping the near and far parameters to generate a reverseZ matrix
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, c_farZ, c_nearZ);
}


void Sample::OnDeviceLost()
{
    // Terrain class needs a ReleaseDevice() method.
    m_textFont.reset();
    m_fontBatch.reset();
    m_uiDescriptorHeap.reset();
    m_graphicsMemory.reset();
    m_gpuTimer.ReleaseDevice();
}


void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
