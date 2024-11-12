//--------------------------------------------------------------------------------------
// PIXCircularCaptureCombo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PIXCircularCaptureCombo.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

void Sample::CaptureThreadProc()
{
    // first version only supports xbox, will support Desktop before shipping
#ifdef _GAMING_XBOX
    PIXCaptureParameters params{};

    params.TimingCaptureParameters.CpuSamplesPerSecond = 10000;
    params.TimingCaptureParameters.CaptureCallstacks = TRUE;
    params.TimingCaptureParameters.CaptureCpuSamples = TRUE;
    params.TimingCaptureParameters.MaximumToolingMemorySizeMb = 256;
    params.TimingCaptureParameters.CaptureStorage = PIXCaptureParameters::MemoryCircular;
    params.TimingCaptureParameters.CaptureVirtualAllocEvents = TRUE;

    for (;;)
    {
        WCHAR fileBuf[256];
        swprintf(fileBuf, 256, L"s:\\profiling\\pixCapture_%i.pevt", m_captureIdx++);
        params.TimingCaptureParameters.FileName = fileBuf;

        DX::ThrowIfFailed(PIXBeginCapture(PIX_CAPTURE_TIMING, &params));

        WaitForSingleObject(m_saveCaptureEvent, INFINITE);

        HRESULT hr;
        do
        {
            hr = PIXEndCapture(m_discardCapture);
        } while (hr == E_PENDING);

        DX::ThrowIfFailed(hr);
    }
#endif
}

Sample::Sample() noexcept(false) :
    m_captureStartFrame(0)
    , m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
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

    m_captureIdx = 0;
    m_discardCapture = false;
    m_saveCaptureEvent = CreateEvent(NULL, FALSE, FALSE, nullptr);
    m_captureThread = new std::thread(&Sample::CaptureThreadProc, this);
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    if ((timer.GetFrameCount() - m_captureStartFrame) >= 1000)
    {
        // Trigger saving the current PIX capture to a file
        m_lastCaptureRanges.push_back(std::make_pair(m_captureStartFrame, timer.GetFrameCount()));
        m_discardCapture = false;
        SetEvent(m_saveCaptureEvent);
        m_captureStartFrame = timer.GetFrameCount();
    }

    auto pad = m_gamePad->GetState(0);

    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
        if (m_gamePadButtons.a == DirectX::GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            // Trigger saving the current PIX capture to a file
            m_lastCaptureRanges.push_back(std::make_pair(m_captureStartFrame, timer.GetFrameCount()));
            m_discardCapture = false;
            SetEvent(m_saveCaptureEvent);
            m_captureStartFrame = timer.GetFrameCount();
        }
        else if (m_gamePadButtons.b == DirectX::GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            // Trigger saving the current PIX capture to a file
            m_discardCapture = true;
            SetEvent(m_saveCaptureEvent);
            m_captureStartFrame = timer.GetFrameCount();
        }
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

    PIXEndEvent();
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

    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* pHeaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), GetTextureSize(m_background.Get()), XMFLOAT2(0, 0));

    if (m_timer.GetFrameCount() > 3)
    {
        wchar_t buffer[256];

        m_largeFont->DrawString(m_spriteBatch.get(), L"PIX Circular Capture Sample", pos);
        pos.y += (m_largeFont->GetLineSpacing() * 2);
        pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));

        DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), L"Press [A] to save current PIX capture buffer", pos);

        pos.y += (m_regularFont->GetLineSpacing() * 1.1f);

        DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), L"Press [B] to restart PIX capture buffer", pos);

        pos.y += m_regularFont->GetLineSpacing() * 2.1f;
        swprintf(buffer, 256, L"Current capture start frame: %d", m_captureStartFrame);
        m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);

        if (m_lastCaptureRanges.size() > 0)
        {
            while (m_lastCaptureRanges.size() > 10)
                m_lastCaptureRanges.erase(m_lastCaptureRanges.begin());
            pos.y += m_regularFont->GetLineSpacing() * 1.2f;
            m_regularFont->DrawString(m_spriteBatch.get(), L"Last capture saved for frame blocks", pos);
            pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));
            for (const auto& iter : m_lastCaptureRanges)
            {
                pos.y += m_regularFont->GetLineSpacing() * 1.2f;
                swprintf(buffer, 256, L"Frames %d - %d", iter.first, iter.second);
                m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
            }
        }
    }

    m_spriteBatch->End();

    PIXEndEvent(commandList);

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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif
    wchar_t strFilePath[MAX_PATH] = {};

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    DX::FindMediaFile(strFilePath, MAX_PATH, L"ATGSampleBackground.dds");
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, resourceUpload,
            strFilePath,
            m_background.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState);

    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_regularFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::RegularFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::RegularFont));
    }

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_24.spritefont");
        m_largeFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::LargeFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::LargeFont));
    }

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneController.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));
    }

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
