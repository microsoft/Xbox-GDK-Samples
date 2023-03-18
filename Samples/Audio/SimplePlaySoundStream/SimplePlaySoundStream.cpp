//--------------------------------------------------------------------------------------
// SimplePlaySoundStream.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimplePlaySoundStream.h"

#include "ATGColors.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pMasteringVoice(nullptr),
    m_pSourceVoice(nullptr),
    m_DoneSubmitting(false),
    m_DonePlaying(false),
    m_waveSize(0),
    m_currentPosition(0),
    m_Buffers{},
    m_NumberOfBuffersProduced(0),
    m_NumberOfBuffersConsumed(0),
    m_producerThread(nullptr),
    m_consumerThread(nullptr),
    m_terminateThread(false),
    m_CritErrorOccurred(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    m_terminateThread = true;

    if (m_producerThread)
    {
        WaitForSingleObject(m_producerThread, INFINITE);
        m_producerThread = nullptr;
    }

    if (m_consumerThread)
    {
        WaitForSingleObject(m_consumerThread, INFINITE);
        m_consumerThread = nullptr;
    }

    if (m_pXAudio2)
    {
        m_pXAudio2->StopEngine();

        m_pXAudio2->UnregisterForCallbacks(this);

        if (m_pSourceVoice)
        {
            m_pSourceVoice->DestroyVoice();
            m_pSourceVoice = nullptr;
        }

        if (m_pMasteringVoice)
        {
            m_pMasteringVoice->DestroyVoice();
            m_pMasteringVoice = nullptr;
        }

        m_pXAudio2.Reset();
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

    InitializeXAudio();
}

void Sample::InitializeXAudio()
{
    // Initialize XAudio2 objects
    DX::ThrowIfFailed(XAudio2Create(m_pXAudio2.GetAddressOf(), 0));

#ifdef _DEBUG
    // Enable debugging features
    XAUDIO2_DEBUG_CONFIGURATION debug = {};
    debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
    debug.BreakMask = XAUDIO2_LOG_ERRORS;
    m_pXAudio2->SetDebugConfiguration(&debug, 0);
#endif
    m_pXAudio2->RegisterForCallbacks(this);

    DX::ThrowIfFailed(m_pXAudio2->CreateMasteringVoice(&m_pMasteringVoice));

    // Open the file for reading and parse its header
    DX::ThrowIfFailed(LoadPCMFile(L"71_setup_sweep_xbox.wav"));

    assert(m_pSourceVoice != nullptr);

    // Start the voice.
    DX::ThrowIfFailed(m_pSourceVoice->Start(0));

    // Create the producer thread (reads PCM chunks from disk)
    m_producerThread = CreateThread(nullptr, 0, Sample::ReadFileThread, this, 0, nullptr);
    if (!m_producerThread)
    {
        throw DX::com_exception(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Create the consumer thread (submits PCM chunks to XAudio2)
    m_consumerThread = CreateThread(nullptr, 0, Sample::SubmitAudioBufferThread, this, 0, nullptr);
    if (!m_consumerThread)
    {
        throw DX::com_exception(HRESULT_FROM_WIN32(GetLastError()));
    }
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

    if (m_CritErrorOccurred)
    {
        m_CritErrorOccurred = false;
        InitializeXAudio();
    }

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        if (pad.IsViewPressed())
        {
            ExitSample();
        }
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

    // Check to see if buffer has finished playing
    if (!m_DonePlaying && m_pSourceVoice && m_NumberOfBuffersProduced > 0 && m_NumberOfBuffersConsumed > 0)
    {
        XAUDIO2_VOICE_STATE state = {};
        m_pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        bool isRunning = (state.BuffersQueued > 0);
        if (isRunning == false)
        {
            m_DonePlaying = true;
        }
    }
    
    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    auto const fullscreen = m_deviceResources->GetOutputSize();

    auto const safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    m_font->DrawString(m_spriteBatch.get(), m_DoneSubmitting ? L"Stream finished" : L"Playing stream", pos);

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

    // Don't need to clear color as the sample draws a fullscreen image background

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

    // Suspend audio engine
    m_pXAudio2->StopEngine();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();

    // Resume audio engine
    m_pXAudio2->StartEngine();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_spriteBatch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    m_font = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_18.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"ATGSampleBackground.DDS", m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const vp = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(vp);
}
#pragma endregion

//--------------------------------------------------------------------------------------
// Name: ReadFileThread()
// Desc: Reads PCM chunks from disk. Blocks when the buffer queue is full
//--------------------------------------------------------------------------------------
DWORD WINAPI Sample::ReadFileThread(LPVOID lpParam)
{
    SetThreadDescription(GetCurrentThread(), L"ReadFileThread");

    auto sample = static_cast<Sample*>(lpParam);

    while (!sample->m_terminateThread && (sample->m_currentPosition < sample->m_waveSize))
    {
        while (sample->m_NumberOfBuffersProduced - sample->m_NumberOfBuffersConsumed >= MAX_BUFFER_COUNT)
        {
            //
            // We reached our capacity to stream in data - we should wait for XAudio2 to finish
            // processing at least one buffer.
            // At this point we could go to sleep, or do something else.
            // For the purposes of this sample, we'll just yield.
            //
            SwitchToThread();

            if (sample->m_terminateThread)
                break;
        }

        if (sample->m_terminateThread)
            break;

        uint32_t cbValid = std::min(STREAMING_BUFFER_SIZE, sample->m_waveSize - sample->m_currentPosition);

        //
        // Allocate memory to stream in data.
        // In a game you would probably acquire this from a memory pool.
        // For the purposes of this sample, we'll allocate it here and have the XAudio2 callback free it later.
        //
        auto pbBuffer = new uint8_t[cbValid];

        //
        // Stream in the PCM data.
        // You could potentially use an async read for this. We are already in another thread so we choose to block.
        //
        DX::ThrowIfFailed(
            sample->m_WaveFile.ReadSample(sample->m_currentPosition, pbBuffer, cbValid, nullptr)
        );

        sample->m_currentPosition += cbValid;

        XAUDIO2_BUFFER buffer = {};
        buffer.AudioBytes = cbValid;
        buffer.pAudioData = pbBuffer;
        if (sample->m_currentPosition >= sample->m_waveSize)
            buffer.Flags = XAUDIO2_END_OF_STREAM;

        //
        // Point pContext at the allocated buffer so that we can free it in the OnBufferEnd() callback
        //
        buffer.pContext = pbBuffer;

        //
        // Make the buffer available for consumption.
        //
        sample->m_Buffers[sample->m_NumberOfBuffersProduced % MAX_BUFFER_COUNT] = buffer;

        //
        // A buffer is ready.
        //
        sample->m_NumberOfBuffersProduced++;
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: SubmitAudioBufferThread()
// Desc: Submits audio buffers to XAudio2. Blocks when XAudio2's queue is full or our buffer queue is empty
//--------------------------------------------------------------------------------------

DWORD WINAPI Sample::SubmitAudioBufferThread(LPVOID lpParam)
{
    SetThreadDescription(GetCurrentThread(), L"SubmitAudioBufferThread");

    auto sample = static_cast<Sample*>(lpParam);
    assert(sample != nullptr);
    assert(sample->m_pSourceVoice != nullptr);

    while (!sample->m_terminateThread)
    {
        while (sample->m_NumberOfBuffersProduced - sample->m_NumberOfBuffersConsumed == 0)
        {
            //
            // There are no buffers ready at this time - we should wait for the ReadFile thread to stream in data.
            // At this point we could go to sleep, or do something else.
            // For the purposes of this sample, we'll just yield.
            //
            SwitchToThread();
        }

        //
        // Wait for XAudio2 to be ready - we need at least one free spot inside XAudio2's queue.
        //
        while (!sample->m_terminateThread)
        {
            assert(sample->m_pSourceVoice != nullptr);

            XAUDIO2_VOICE_STATE state = {};
            sample->m_pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

            if (state.BuffersQueued < MAX_BUFFER_COUNT - 1)
                break;

            WaitForSingleObject(sample->m_VoiceContext.m_hBufferEndEvent, INFINITE);
        }

        if (sample->m_terminateThread)
            break;

        //
        // Now we have at least one spot free in our buffer queue, and at least one spot free
        // in XAudio2's queue, so submit the next buffer.
        //
        XAUDIO2_BUFFER buffer = sample->m_Buffers[sample->m_NumberOfBuffersConsumed % MAX_BUFFER_COUNT];
        DX::ThrowIfFailed(sample->m_pSourceVoice->SubmitSourceBuffer(&buffer));

        //
        // A buffer is free.
        //
        sample->m_NumberOfBuffersConsumed++;

        //
        // Check if this is the last buffer.
        //
        if (buffer.Flags == XAUDIO2_END_OF_STREAM)
        {
            //
            // We are done.
            //
            sample->m_DoneSubmitting = true;
            break;
        }
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: LoadPCMFile
// Desc: Opens a PCM file for reading and parses its header
//--------------------------------------------------------------------------------------
HRESULT Sample::LoadPCMFile(const wchar_t* szFilename)
{
    //
    // Read the wave file
    //
    HRESULT hr = m_WaveFile.Open(szFilename);
        if (FAILED(hr))
            return hr;

    // Read the format header
    WAVEFORMATEXTENSIBLE wfx = {};
    hr = m_WaveFile.GetFormat(reinterpret_cast<WAVEFORMATEX*>(&wfx), sizeof(wfx));
        if (FAILED(hr))
            return hr;

    // Calculate how many bytes and samples are in the wave
    m_waveSize = m_WaveFile.GetDuration();

    //
    // Create the source voice to playback the PCM content
    //
    hr = m_pXAudio2->CreateSourceVoice(&m_pSourceVoice, &(wfx.Format), 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_VoiceContext);

    return hr;
}
