//--------------------------------------------------------------------------------------
// ScatteringFp16.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "ScatteringFp16.h"

#include "CompiledShaders\Scattering_32bit_1ppt.csh"
#include "CompiledShaders\Scattering_32bit_2ppt_nopack.csh"
#include "CompiledShaders\Scattering_32bit_2ppt_withpack.csh"

#include "CompiledShaders\Scattering_16bit_1ppt.csh"
#include "CompiledShaders\Scattering_16bit_2ppt_nopack.csh"
#include "CompiledShaders\Scattering_16bit_2ppt_withpack.csh"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t * const kScatteringModeName[] =
    {
        L"1 pixel per thread (float)",
        L"2 pixels per thread, instruction duplication via two function calls (float)",
        L"2 pixels per thread, instruction duplication via data parallel computation (float{1,2,3,4} -> float{1,2,3,4}x2)",
        L"1 pixel per thread (half)",
        L"2 pixels per thread, instruction duplication via two function calls (half)",
        L"2 pixels per thread, instruction duplication via data parallel computation (half{1,2,3,4} -> half{1,2,3,4}x2)",
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
}

static inline void checkHRESULT(HRESULT hr)
{
    if (hr != S_OK)
    {
        __debugbreak();
    }
}

#define ASSERT_MSG(cond, msg)                    \
    do                                                  \
    {                                                   \
        if (!(cond))                                    \
        {                                               \
            OutputDebugStringA("ScatteringFp16 :: " msg);  \
            __debugbreak();                             \
        }                                               \
    }                                                   \
    while (0);

#define ASSERT(cond) ASSERT_MSG(cond, #cond)

static inline uint32_t shiftRightAndRoundUp(uint32_t x, uint8_t shift)
{
    ASSERT_MSG(shift < 32, "shiftRightAndRoundUp receives too big 'shift' parameter");
    return (x + ((1u << shift) - 1u)) >> shift;
}

typedef enum ScatteringMode
{
    kScatteringMode1PixPerThreadFp32            = 0,
    kScatteringMode2PixPerThreadFp32NoPack      = 1,
    kScatteringMode2PixPerThreadFp32WithPack    = 2,
    kScatteringMode1PixPerThreadFp16            = 3,
    kScatteringMode2PixPerThreadFp16NoPack      = 4,
    kScatteringMode2PixPerThreadFp16WithPack    = 5,
    kScatteringModeCount                        = 6
} ScatteringMode;

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_currentColour(_countof(FONT_COLORS) - 1)
    , m_currentResolutionIndex(0)
    , m_scatteringMode(kScatteringMode1PixPerThreadFp32)
    , m_flags(0x4)
    , m_numResolutionIndices(0)
    , m_leftStickX(0.5)
    , m_leftStickY(0.5)
    , m_hideHUD(false)
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_Enable4K_UHD |  DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
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

    auto const rect = m_deviceResources->GetOutputSize();
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
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        // Switch debug visualization mode: color ids, divergency heat map, indirection buffer visualization
        if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_scatteringMode = (m_scatteringMode + kScatteringModeCount - 1) % kScatteringModeCount;
        }
        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_scatteringMode = (m_scatteringMode + 1) % kScatteringModeCount;
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_flags += 0x4u;
            if ((m_flags & 0xcu) == 0xcu)
            {
                m_flags &= ~0xcu;
            }
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_flags ^= 0x1u;
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_flags ^= 0x2u;
        }

        m_leftStickX += 0.01f * pad.thumbSticks.leftX;
        m_leftStickX = m_leftStickX < 0.0f ? 0.0f : m_leftStickX;
        m_leftStickX = m_leftStickX > 1.0f ? 1.0f : m_leftStickX;

        m_leftStickY += 0.01f * pad.thumbSticks.leftY;
        m_leftStickY = m_leftStickY < 0.0f ? 0.0f : m_leftStickY;
        m_leftStickY = m_leftStickY > 1.0f ? 1.0f : m_leftStickY;

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_currentColour = (m_currentColour + 1) % _countof(FONT_COLORS);
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

    wchar_t renderStr[256] = {};
    swprintf_s(renderStr, L"Render %u (%u by %u)", m_timer.GetFrameCount() - 1, currentResolution.right, currentResolution.bottom);
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, renderStr);

    // Change the state of the color buffer to Uav to make it writeable from compute shaders
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_outputTex.Get(),
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );
        commandList->ResourceBarrier(1, &barrier);
    }

    // Optionally, draw requested debug visualization
    //if (m_scatteringMode != 0)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, "Scattering Pass");

        commandList->SetComputeRootSignature(m_scatteringRootSig.Get());
        
        switch (m_scatteringMode)
        {
            case kScatteringMode1PixPerThreadFp32:
                commandList->SetPipelineState(m_scattering32Bit1pptPSO.Get());
            break;

            case kScatteringMode2PixPerThreadFp32NoPack:
                commandList->SetPipelineState(m_scattering32Bit2pptNoPackPSO.Get());
            break;

            case kScatteringMode2PixPerThreadFp32WithPack:
                commandList->SetPipelineState(m_scattering32Bit2pptWithPackPSO.Get());
            break;

            case kScatteringMode1PixPerThreadFp16:
                commandList->SetPipelineState(m_scattering16Bit1pptPSO.Get());
                break;

            case kScatteringMode2PixPerThreadFp16NoPack:
                commandList->SetPipelineState(m_scattering16Bit2pptNoPackPSO.Get());
                break;

            case kScatteringMode2PixPerThreadFp16WithPack:
                commandList->SetPipelineState(m_scattering16Bit2pptWithPackPSO.Get());
                break;

            default:
            break;
        };
        commandList->SetComputeRootDescriptorTable(0, m_csuHeap->GetGpuHandle(SRVUAVDescriptors::OutputUAV));

        auto const w = static_cast<uint32_t>(currentResolution.right);
        auto const h = static_cast<uint32_t>(currentResolution.bottom);

        struct ShaderConstants
        {
            uint32_t x;
            uint32_t y;
            uint32_t flags;
            float sunDiskX;
            float sunDiskY;
        } shaderConstants = { w, h, m_flags, m_leftStickX, m_leftStickY };

        commandList->SetComputeRoot32BitConstants(1, sizeof(shaderConstants) / sizeof(uint32_t), reinterpret_cast<void *>(&shaderConstants), 0);

        m_gpuTimer.Start(commandList, 0);
        uint8_t threadGroupLog2SizeX = m_scatteringMode == kScatteringMode1PixPerThreadFp32 || m_scatteringMode == kScatteringMode1PixPerThreadFp16
                                     ? 2u
                                     : 3u;

        commandList->Dispatch(shiftRightAndRoundUp(w, threadGroupLog2SizeX), shiftRightAndRoundUp(h, 3u), 1);
        m_gpuTimer.Stop(commandList, 0);

        PIXEndEvent(commandList);
    }

    // Changes states of
    //      a) the color buffer to copy data from it
    //      b) the texture with pixel ids to make it writeable in the next frame
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_outputTex.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->ResourceBarrier(1, &barrier);
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

    swprintf_s(textBuffer, L"Scattering Mode: %s", kScatteringModeName[m_scatteringMode]);
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
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[A] : Switch Tonemapping mode", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[Y] : Switch Projection mode", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[X] : Debug Transmittance Curves", textPos, textColor);

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
    auto backBufferRts = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
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
        DX::ThrowIfFailed(device->CreateRootSignature(0, g_Scattering_32bit_1ppt, sizeof(g_Scattering_32bit_1ppt), IID_GRAPHICS_PPV_ARGS(m_scatteringRootSig.GetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_scatteringRootSig.Get();

        psoDesc.CS = { g_Scattering_32bit_1ppt, sizeof(g_Scattering_32bit_1ppt) };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_scattering32Bit1pptPSO.GetAddressOf())));

        psoDesc.CS = { g_Scattering_32bit_2ppt_nopack, sizeof(g_Scattering_32bit_2ppt_nopack) };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_scattering32Bit2pptNoPackPSO.GetAddressOf())));

        psoDesc.CS = { g_Scattering_32bit_2ppt_withpack, sizeof(g_Scattering_32bit_2ppt_withpack) };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_scattering32Bit2pptWithPackPSO.GetAddressOf())));

        psoDesc.CS = { g_Scattering_16bit_1ppt, sizeof(g_Scattering_16bit_1ppt) };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_scattering16Bit1pptPSO.GetAddressOf())));

        psoDesc.CS = { g_Scattering_16bit_2ppt_nopack, sizeof(g_Scattering_16bit_2ppt_nopack) };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_scattering16Bit2pptNoPackPSO.GetAddressOf())));

        psoDesc.CS = { g_Scattering_16bit_2ppt_withpack, sizeof(g_Scattering_16bit_2ppt_withpack) };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_scattering16Bit2pptWithPackPSO.GetAddressOf())));
    }
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
