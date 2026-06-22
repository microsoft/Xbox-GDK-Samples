//--------------------------------------------------------------------------------------
// PixelBinning.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PixelBinning.h"

#include "PixelBinner.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"

#include "CompiledShaders\FullScreenNoise.csh"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t * const kBinningTypeName[] =
    {
        L"Global (within Image)",
        L"Local (within Macro Tile)"
    };

    const wchar_t* GetDeviceName()
    {
        switch (XSystemGetDeviceType())
        {
        case XSystemDeviceType::Pc: return L"PC";
        case XSystemDeviceType::XboxOne: return L"Xbox One";
        case XSystemDeviceType::XboxOneS: return L"Xbox One S";
        case XSystemDeviceType::XboxOneX: return L"Xbox One X";
        case XSystemDeviceType::XboxOneXDevkit: return L"Xbox One X Devkit";
        case XSystemDeviceType::XboxScarlettLockhart: return L"Xbox Series S";
        case XSystemDeviceType::XboxScarlettAnaconda: return L"Xbox Series X";
        case XSystemDeviceType::XboxScarlettDevkit: return L"Xbox Series X Devkit";
        case XSystemDeviceType::Unknown:
        default: return L"Unknown";
        }
    }

    const DirectX::XMVECTORF32 FONT_COLORS[] =
    {

        { 1, 1, 0, 1 },
        { 1, 0, 0, 1 },
        { 0, 1, 0, 1 },
    };

    PixelBinnerResources GPixelBinnerRes = {};
}

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_currentColour(_countof(FONT_COLORS) - 1)
    , m_currentResolutionIndex(0)
    , m_binningType(kPixelBinnerPackingType_Local)
    , m_dbgviewType(kPixelBinnerDbgVisPassId_Count)
    , m_numResolutionIndices(0)
    , m_numBinsUsed(8)
    , m_numCountersUsed(4 * 1024)
    , m_hideHUD(false)
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_Enable4K_UHD |  DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);

    // Computed minimal supported number of bins and counters
    pixelBinnerApplyLimits(&m_numBinsUsed, &m_numCountersUsed);
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
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    auto rect = m_deviceResources->GetOutputSize();
    m_numResolutionIndices = (uint16_t)((rect.right - 1280) / (8 * 16) + 1);

    m_currentResolutionIndex = m_numResolutionIndices - 1u;
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
void Sample::Update(DX::StepTimer const& timer)
{
    (void)timer;
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        // Switch debug visualization mode: color ids, divergency heat map, indirection buffer visualization
        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_dbgviewType = (m_dbgviewType + 1) % kPixelBinnerDbgVisPassId_Count;
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_currentColour = (m_currentColour + 1) % _countof(FONT_COLORS);
        }

        // Toggle binning type (global or local)
        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_binningType ^= 1;
        }

        if (m_gamePadButtons.start == GamePad::ButtonStateTracker::PRESSED)
        {
            m_hideHUD = !m_hideHUD;
        }

        // Tweak resolution of the texture. Supported range 720p - 2160p(1080p) XboxOneX/Scarlett (XBoxOne)
        bool incRes = m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::ButtonState::PRESSED;
        bool decRes = m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::ButtonState::PRESSED;
        if (incRes || decRes)
        {
            if (incRes)
            {
                m_currentResolutionIndex++;
            }
            if (decRes)
            {
                m_currentResolutionIndex += (m_numResolutionIndices - 1);
            }
            m_currentResolutionIndex = (m_currentResolutionIndex % m_numResolutionIndices);
        }

        // Tweak the number of unique bins that each pixel can be assigned to
        // or, in other words, the maximal bit width of keys/ids each pixel can contain
        uint32_t prevNumCountersUsed = m_numCountersUsed;
        bool incBinsEvent = m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED;
        bool decBinsEvent = m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED;
        if (incBinsEvent || decBinsEvent)
        {
            uint16_t prevNumBinsUsed = m_numBinsUsed;

            if (incBinsEvent)
            {
                m_numBinsUsed <<= 1;
                pixelBinnerApplyLimits(&m_numBinsUsed, &m_numCountersUsed);
                m_numBinsUsed = (m_numBinsUsed == prevNumBinsUsed) ? 0u : m_numBinsUsed;
            }

            if (decBinsEvent)
            {
                m_numBinsUsed >>= 1;
                pixelBinnerApplyLimits(&m_numBinsUsed, &m_numCountersUsed);
                m_numBinsUsed = (m_numBinsUsed == prevNumBinsUsed) ? 0xffffu : m_numBinsUsed;
            }
            pixelBinnerApplyLimits(&m_numBinsUsed, &m_numCountersUsed);
        }

        // Tweak the budget of intermediate memory used for binning as the number of 4-byte elements
        bool incEvent = m_gamePadButtons.rightTrigger == GamePad::ButtonStateTracker::ButtonState::PRESSED;
        bool decEvent = m_gamePadButtons.leftTrigger == GamePad::ButtonStateTracker::ButtonState::PRESSED;
        if (incEvent || decEvent)
        {
            if (decEvent)
                m_numCountersUsed >>= 1u;
            else if (incEvent)
                m_numCountersUsed <<= 1u;

            pixelBinnerApplyLimits(&m_numBinsUsed, &m_numCountersUsed);
            if (m_numCountersUsed == prevNumCountersUsed)
            {
                if (decEvent)
                    m_numCountersUsed = ~0u;
                else if (incEvent)
                    m_numCountersUsed = 0;

                pixelBinnerApplyLimits(&m_numBinsUsed, &m_numCountersUsed);
            }
        }

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
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

    RECT currentResolution;
    currentResolution.left = 0;
    currentResolution.top = 0;
    currentResolution.right = 1280 + (LONG)m_currentResolutionIndex * (8 * 16);
    currentResolution.bottom = 720 + (LONG)m_currentResolutionIndex * (8 * 9);

    m_deviceResources->SetPresentSize(currentResolution);
    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
    //    Clear();
    auto commandList = m_deviceResources->GetCommandList();

    m_gpuTimer.BeginFrame(commandList);

    auto heap = m_csuHeap->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    wchar_t renderStr[256];
    swprintf_s(renderStr, L"Render %u (%u by %u)", m_timer.GetFrameCount() - 1, currentResolution.right, currentResolution.bottom);
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, renderStr);

    // Initialize the structure storing internal parameters for the pixel binning
    // by supplying resolution for the current frame and the number of bins/ids used used for binning/classification
    PixelBinnerConfig pbConfig;
    pixelBinnerInitConfig(&pbConfig, (uint16_t)currentResolution.right, (uint16_t)currentResolution.bottom, m_numBinsUsed, (PixelBinnerPackingType)m_binningType);

    // Constrain the amount of intermediate memory used for binning
    pixelBinnerConstrainNumCounters(&pbConfig, m_numCountersUsed);

    // Change the state of the color buffer to Uav to make it writeable from compute shaders
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_outputTex.Get(),
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );
        commandList->ResourceBarrier(1, &barrier);
    }

    // Setup the state and dispatch the pass which generates full screen procedural noise to create some ids for every pixel
    {
        commandList->SetComputeRootSignature(m_fullScreenNoiseRootSig.Get());
        commandList->SetPipelineState(m_fullScreenNoisePSO.Get());
        commandList->SetComputeRootDescriptorTable(0, {GPixelBinnerRes.m_gpuUavDescs[kPixelBinnerResId_BinIdTex2d]});
        commandList->SetComputeRootDescriptorTable(1, {GPixelBinnerRes.m_gpuUavDescs[kPixelBinnerResId_OutputTex2d]});

        struct Params
        {
            uint32_t resX;
            uint32_t resY;
            uint32_t numBins;
        } params = { pbConfig.m_resX, pbConfig.m_resY, pbConfig.m_numBinsRequired } ;

        commandList->SetComputeRoot32BitConstants(2, sizeof(params) / sizeof(UINT), (void*)&params, 0);

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, "GenerateDummyIds (Noise)");

        const uint32_t numTGX = (uint32_t(currentResolution.right) + 7) >> 3;
        const uint32_t numTGY = (uint32_t(currentResolution.bottom) + 7) >> 3;

        commandList->Dispatch(numTGX, numTGY, 1);
        PIXEndEvent(commandList);
    }
    // Change the state of the texture containing per pixel ids to be read-only for future reads by compute shaders
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            GPixelBinnerRes.m_d3dPtrs[kPixelBinnerResId_BinIdTex2d],
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);
    }
    m_gpuTimer.Start(commandList, 0);
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, "PixelBinner :: Submit");

    // Submit all passes necessary to complete binning
    pixelBinnerSubmit(commandList, &pbConfig, &GPixelBinnerRes);

    PIXEndEvent(commandList);
    m_gpuTimer.Stop(commandList, 0);

    // Optionally, draw requested debug visualization
    if (m_dbgviewType != 0)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, "PixelBinner :: DebugVis");
        pixelBinnerSubmitDbgVisPass(commandList, &pbConfig, &GPixelBinnerRes, (PixelBinnerDbgVisPassId)m_dbgviewType);
        PIXEndEvent(commandList);
    }

    // Submit the pass which checks results of binning for correctness, if something is wrong, pink color is displayed
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, "PixelBinner :: Check");
    pixelBinnerSubmitCheckPass(commandList, &pbConfig, &GPixelBinnerRes);
    PIXEndEvent(commandList);

    // Changes states of
    //      a) the color buffer to copy data from it
    //      b) the texture with pixel ids to make it writeable in the next frame
    {
        D3D12_RESOURCE_BARRIER barriers[2];
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
            GPixelBinnerRes.m_d3dPtrs[kPixelBinnerResId_BinIdTex2d],
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
            m_outputTex.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->ResourceBarrier(2, barriers);
    }
    commandList->CopyResource(m_deviceResources->GetRenderTarget(), m_outputTex.Get());
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_deviceResources->GetRenderTarget(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrier);
    }

    if (!m_hideHUD)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"HUD");
        RenderHUD(commandList);
        PIXEndEvent(commandList);
    }

    m_gpuTimer.EndFrame(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
}
#pragma endregion

void Sample::RenderHUD(ID3D12GraphicsCommandList* cl)
{
    RECT size;
    size.left   = 0;
    size.top    = 0;
    size.right  = 1280 + (LONG)m_currentResolutionIndex * (8 * 16);
    size.bottom =  720 + (LONG)m_currentResolutionIndex * (8 *  9);

    const D3D12_VIEWPORT viewport = { 0, 0, float(size.right), float(size.bottom), 0, 1 };

    m_hudBatch->SetViewport(viewport);
    m_hudBatch->Begin(cl);

    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    cl->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    cl->RSSetViewports(1, &viewport);
    cl->RSSetScissorRects(1, &size);

    auto& font = size.bottom <= 1080 ? m_smallFont : m_bigFont;

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea((UINT)size.right, (UINT)size.bottom);

    wchar_t textBuffer[256] = {};
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = FONT_COLORS[m_currentColour];

    float totalTime = 0.0;
    float averageRenderTime = m_gpuTimer.GetAverageMS(0);

    totalTime += averageRenderTime;

    swprintf_s(textBuffer, L"TOTAL: %0.2fms (%u FPS) at %ux%u", totalTime, uint32_t(1000.0f / totalTime), uint32_t(viewport.Width), uint32_t(viewport.Height));
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    swprintf_s(textBuffer, L"Pixel Binning: %s", kBinningTypeName[m_binningType]);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    PixelBinnerConfig pbConfig;
    pixelBinnerInitConfig(&pbConfig, uint16_t(size.right), uint16_t(size.bottom), m_numBinsUsed, (PixelBinnerPackingType)m_binningType);

    pixelBinnerConstrainNumCounters(&pbConfig, m_numCountersUsed);

    uint32_t pixelsPerMicroTileX = 1u << pbConfig.m_log2MicroTileSizeX;
    uint32_t pixelsPerMicroTileY = 1u << pbConfig.m_log2MicroTileSizeY;

    uint32_t pixelsPerMacroTileX = 1u << pbConfig.m_log2MacroTileSizeX;
    uint32_t pixelsPerMacroTileY = 1u << pbConfig.m_log2MacroTileSizeY;

    uint32_t microTilesPerMacroTilesX = 1u << (pbConfig.m_log2MacroTileSizeX - pbConfig.m_log2MicroTileSizeX);
    uint32_t microTilesPerMacroTilesY = 1u << (pbConfig.m_log2MacroTileSizeY - pbConfig.m_log2MicroTileSizeY);

    swprintf_s(textBuffer, L"Macro Tile Size: %ux%u pixels (%ux%u micro tiles of size %ux%u)", pixelsPerMacroTileX, pixelsPerMacroTileY, microTilesPerMacroTilesX, microTilesPerMacroTilesY, pixelsPerMicroTileX, pixelsPerMicroTileY);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    swprintf_s(textBuffer, L"%ux%u macro tiles (%ux%u micro tiles) per image", pbConfig.m_numMacroTilesX, pbConfig.m_numMacroTilesY, pbConfig.m_numMicroTilesX, pbConfig.m_numMicroTilesY);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    swprintf_s(textBuffer, L"# of bins: %u", pbConfig.m_numBinsRequired);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    swprintf_s(textBuffer, L"Reserved # of counters: %u", pbConfig.m_numU32CountersReserved);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    swprintf_s(textBuffer, L"Requried # of counters: %u", pbConfig.m_numU32CountersRequired);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    if (pbConfig.m_dispatchSizeL1 != 1u)
        swprintf_s(textBuffer, L"Prefix Group Sizes: L1=%u, L2=%u", 1u << pbConfig.m_log2PrefixSizeL1, 1u << pbConfig.m_log2PrefixSizeL2);
    else
        swprintf_s(textBuffer, L"Prefix Group Sizes: L1=%u", 1u << pbConfig.m_log2PrefixSizeL1);

    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    swprintf_s(textBuffer, L"Device Type: %ls", GetDeviceName());
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    // Draw controls legend
    {
        textPos = XMFLOAT2(float(safe.left), float(safe.bottom));

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[Menu] : Toggle HUD", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[B] : Change colour", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[A] : Switch debug mode", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[Y] : Switch binning type", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[LT][RT] : Decrease/Increase # of counters", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[LB][RB] : Decrease/Increase # of bins", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        swprintf_s(textBuffer, L"[DPAD] : Left/Right - Change Resolution");
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), textBuffer, textPos, textColor);

        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), textBuffer, textPos, textColor);

    }

    m_hudBatch->End();
}

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_csuHeap = std::make_unique<DescriptorHeap>(device, SRVUAVDescriptors::EnumCount);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();
    const RenderTargetState backBufferRts(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_csuHeap->GetCpuHandle(SRVUAVDescriptors::FontSRVSmall),
        m_csuHeap->GetGpuHandle(SRVUAVDescriptors::FontSRVSmall));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_36.spritefont");
    m_bigFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_csuHeap->GetCpuHandle(SRVUAVDescriptors::FontSRVBig),
        m_csuHeap->GetGpuHandle(SRVUAVDescriptors::FontSRVBig));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegend.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_csuHeap->GetCpuHandle(SRVUAVDescriptors::ControllerFontSRV),
        m_csuHeap->GetGpuHandle(SRVUAVDescriptors::ControllerFontSRV));

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
    {
        DX::ThrowIfFailed(device->CreateRootSignature(0, g_FullScreenNoise, sizeof(g_FullScreenNoise), IID_GRAPHICS_PPV_ARGS(m_fullScreenNoiseRootSig.GetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.CS = { g_FullScreenNoise, sizeof(g_FullScreenNoise) };
        psoDesc.pRootSignature = m_fullScreenNoiseRootSig.Get();

        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_fullScreenNoisePSO.GetAddressOf())));
    }

    pixelBinnerInit(device);

    // get the maximal supported number of bins
    uint16_t maxBinsSupported = 0;
    pixelBinnerGetLimits(&maxBinsSupported, 0);

    // initialize temporal pixel binner config to determine maximal possible resource usage
    PixelBinnerConfig pbConfigMaxResUsage;
    pixelBinnerInitConfig(&pbConfigMaxResUsage, 3840u, 2160u, maxBinsSupported, (PixelBinnerPackingType)m_binningType);

    pixelBinnerCreateCommittedResources(&GPixelBinnerRes, kPixelBinnerResId_MaskAll, device, &pbConfigMaxResUsage);

    // Setup addresses where Srv descriptors should be placed
    GPixelBinnerRes.m_cpuSrvDescs[kPixelBinnerResId_BinIdTex2d] = m_csuHeap->GetCpuHandle(SRVUAVDescriptors::FragmentIDsSRV).ptr;
    GPixelBinnerRes.m_gpuSrvDescs[kPixelBinnerResId_BinIdTex2d] = m_csuHeap->GetGpuHandle(SRVUAVDescriptors::FragmentIDsSRV).ptr;

    pixelBinnerCreateSrvDescriptors(&GPixelBinnerRes, kPixelBinnerResId_MaskBinIdTex2d, device, &pbConfigMaxResUsage);

    // Setup addresses where Uav descriptors should be placed
    GPixelBinnerRes.m_cpuUavDescs[kPixelBinnerResId_BinIdTex2d] = m_csuHeap->GetCpuHandle(SRVUAVDescriptors::FragmentIDsUAV).ptr;
    GPixelBinnerRes.m_gpuUavDescs[kPixelBinnerResId_BinIdTex2d] = m_csuHeap->GetGpuHandle(SRVUAVDescriptors::FragmentIDsUAV).ptr;

    GPixelBinnerRes.m_cpuUavDescs[kPixelBinnerResId_CheckTex2d] = m_csuHeap->GetCpuHandle(SRVUAVDescriptors::VerificationKeysUAV).ptr;
    GPixelBinnerRes.m_gpuUavDescs[kPixelBinnerResId_CheckTex2d] = m_csuHeap->GetGpuHandle(SRVUAVDescriptors::VerificationKeysUAV).ptr;

    pixelBinnerCreateUavDescriptors(&GPixelBinnerRes, kPixelBinnerResId_MaskTexOnly, device, &pbConfigMaxResUsage);

    // Setup address of the uav descriptor of the color buffer. It's created externally
    GPixelBinnerRes.m_gpuUavDescs[kPixelBinnerResId_OutputTex2d] = m_csuHeap->GetGpuHandle(SRVUAVDescriptors::OutputUAV).ptr;

    m_gpuTimer.RestoreDevice(device, m_deviceResources->GetCommandQueue());
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto const outputSize = m_deviceResources->GetOutputSize();

    D3D12_RESOURCE_DESC rwTex2DDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            m_deviceResources->GetBackBufferFormat(),
            UINT(outputSize.right), UINT(outputSize.bottom),
            1, 1, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    D3D12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DX::ThrowIfFailed(device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &rwTex2DDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_GRAPHICS_PPV_ARGS(m_outputTex.GetAddressOf())));

    device->CreateUnorderedAccessView(m_outputTex.Get(), nullptr, nullptr, m_csuHeap->GetCpuHandle(SRVUAVDescriptors::OutputUAV));
}

#pragma endregion
