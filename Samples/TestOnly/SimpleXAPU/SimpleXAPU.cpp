//--------------------------------------------------------------------------------------
// SimpleXAPU.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleXAPU.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_keyDown(false),
    m_pMasteringVoice(nullptr),
    m_pSourceVoice(nullptr),
    m_xapuHandle(nullptr),
    m_opusFile(nullptr),
#ifdef WRITETOFILE
    m_outFile(nullptr),
#endif
    m_waveSize(0),
    m_currentPosition(0),
    m_NumberOfBuffersProduced(0),
    m_NumberOfBuffersConsumed(0),
    m_producerThread(nullptr),
    m_consumerThread(nullptr),
    m_isPlaying(true),
    m_terminateThread(false),
    m_loop(true),
    m_pitch(0.f)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
}

Sample::~Sample()
{
    m_terminateThread = true;

    //Terminate read/render threads
    if (m_consumerThread)
    {
        WaitForSingleObject(m_consumerThread, INFINITE);
        m_consumerThread = nullptr;
    }

    if (m_producerThread)
    {
        WaitForSingleObject(m_producerThread, INFINITE);
        m_producerThread = nullptr;
    }

    //Close audio source file
    if (m_opusFile)
    {
        fclose(m_opusFile);
    }

#ifdef WRITETOFILE
    //Close decoded audio output file
    if (m_outFile)
    {
        fclose(m_outFile);
    }
#endif

    //Deactivate and disconnect XAPU
    XApuCommandId id;
    id.type = XApuCommandType::Deactivate;
    id.streamIndex = 0;
    id.sequence = 0;

    XApuEnqueueCommand(m_xapuHandle, &id, nullptr);

    XApuDisconnect(m_xapuHandle);

    //Stop and clean up XAudio2
    if (m_pXAudio2)
    {
        m_pXAudio2->StopEngine();

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

    // Initialize XAudio2 objects
    DX::ThrowIfFailed(XAudio2Create(m_pXAudio2.GetAddressOf(), 0));

#ifdef _DEBUG
    // Enable debugging features
    XAUDIO2_DEBUG_CONFIGURATION debug = {};
    debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
    debug.BreakMask = XAUDIO2_LOG_ERRORS;
    m_pXAudio2->SetDebugConfiguration(&debug, 0);
#endif

    DX::ThrowIfFailed(m_pXAudio2->CreateMasteringVoice(&m_pMasteringVoice));

    //Open OPUS file
    fopen_s(&m_opusFile, "MusicMono.opus", "rb");

    if (nullptr == m_opusFile)
    {
        throw DX::com_exception(HRESULT_FROM_WIN32(GetLastError()));
    }

    //Read the length of the opus file
    fseek(m_opusFile, 0, SEEK_END);
    m_waveSize = ftell(m_opusFile);
    fseek(m_opusFile, 0, SEEK_SET);

#ifdef WRITETOFILE
    //Create output file
    fopen_s(&m_outFile, "decodeoutput.wav", "wb");

    if (nullptr == m_outFile)
    {
        throw DX::com_exception(HRESULT_FROM_WIN32(GetLastError()));
    }

    fwrite(WaveHeaderMonoFloat48K, 1, sizeof(WaveHeaderMonoFloat48K), m_outFile);
#endif

    const XApuConnectInputParameters* connectParameters = m_memoryManager.GetConnectParametersRef();
    const XApuConnectOutputParameters* connectionData = m_memoryManager.GetConnectionDataRef();
    DX::ThrowIfFailed(XApuConnect((XApuConnectInputParameters*)connectParameters, (XApuConnectOutputParameters*)connectionData, &m_xapuHandle));

    uint32_t processingMaxByteCount = 0;

    XApuDecodeConvertActivateCommand command = {};
    command.id.type = XApuCommandType::Activate;
    command.id.streamIndex = 0;
    command.id.sequence = 0;
    command.channelCount = 1;
    command.startPitch = 0.0f;
    command.processingBuffer = m_memoryManager.GetProcessingBuffer(processingMaxByteCount);
    command.processingBufferLength = processingMaxByteCount;

    DX::ThrowIfFailed(XApuEnqueueCommand(m_xapuHandle, &command.id, nullptr));

    //Create source voice
    WAVEFORMATEX wfx = {};
    wfx.nChannels = 1;
    wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wfx.nSamplesPerSec = 48000;
    wfx.wBitsPerSample = 32;
    wfx.nBlockAlign = (WORD)(wfx.wBitsPerSample * wfx.nChannels / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
    DX::ThrowIfFailed(m_pXAudio2->CreateSourceVoice(&m_pSourceVoice, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_VoiceContext));

    StartPlayback();
}

void Sample::StartPlayback()
{
    m_terminateThread = false;

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

void Sample::StopPlayback()
{
    m_terminateThread = true;
    m_pSourceVoice->Stop();
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
        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (pad.IsDPadUpPressed())
        {
            m_pitch += .1f;
            if (m_pitch > MAX_PITCH) m_pitch = MAX_PITCH;
        }
        else if (pad.IsDPadDownPressed())
        {
            m_pitch -= .1f;
            if (m_pitch < 0.f) m_pitch = 0.f;
        }

        if (!m_keyDown)
        {
            if (pad.IsXPressed())
            {
                m_keyDown = true;
                m_loop = !m_loop;
            }
            else if (pad.IsAPressed())
            {
                m_keyDown = true;
                if (m_isPlaying)
                {
                    StopPlayback();
                }
                else
                {
                    StartPlayback();
                }

                m_isPlaying = !m_isPlaying;
            }
        }
        else if (!pad.IsXPressed() && !pad.IsAPressed())
        {
            m_keyDown = false;
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
    //if (!m_isPlaying && m_pSourceVoice && m_NumberOfBuffersProduced > 0)
    //{
    //    XAUDIO2_VOICE_STATE state;
    //    m_pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    //    bool isRunning = (state.BuffersQueued > 0);
    //    if (isRunning == false)
    //    {
    //        m_pSourceVoice->DestroyVoice();
    //        m_pSourceVoice = nullptr;
    //        m_isPlaying = false;
    //    }
    //}

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    auto fullscreen = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea((UINT)(fullscreen.right - fullscreen.left), (UINT)(fullscreen.bottom - fullscreen.top));

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);
    wchar_t tempString[256] = {};

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    m_font->DrawString(m_spriteBatch.get(), m_isPlaying ? L"Playing" : L"Stopped", pos);
    pos.y += m_font->GetLineSpacing() * 1.5f;

    m_font->DrawString(m_spriteBatch.get(), m_loop ? L"Loop: On" : L"Loop: Off", pos);
    pos.y += m_font->GetLineSpacing() * 1.5f;

    swprintf(tempString, 255, L"Pitch: %3.3f", m_pitch);
    m_font->DrawString(m_spriteBatch.get(), tempString, pos);

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
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    // Don't need to clear color as the sample draws a fullscreen image background

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
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
    auto vp = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(vp);
}
#pragma endregion

//--------------------------------------------------------------------------------------------------
// Name: ReadFileThread()
// Desc: Reads Opus packets from disk and queues their decode. Blocks when the buffer queue is full
//--------------------------------------------------------------------------------------------------
DWORD WINAPI Sample::ReadFileThread(LPVOID lpParam)
{
    SetThreadDescription(GetCurrentThread(), L"ReadFileThread");

    auto sample = static_cast<Sample*>(lpParam);

    while (!sample->m_terminateThread && (sample->m_currentPosition < (uint32_t)(sample->m_waveSize)))
    {
        while (sample->m_NumberOfBuffersProduced - sample->m_NumberOfBuffersConsumed >= MAX_BUFFER_COUNT)
        {
            // We reached our capacity to stream in data - we should wait for XAudio2 to finish
            // processing at least one buffer.
            // At this point we could go to sleep, or do something else.
            // For the purposes of this sample, we'll just yield.
            SwitchToThread();
        }

        unsigned char segmentCount;
        unsigned char segmentTable[256];

        //Skip header
        fseek(sample->m_opusFile, 349, SEEK_CUR);

        //Skip to segments
        fseek(sample->m_opusFile, 26, SEEK_CUR);

        //Segment count
        if(fread_s(&segmentCount, 1, 1, 1, sample->m_opusFile) != 1)
        {
            throw std::runtime_error("fread_s count");
        }

        //Segment table
        if(fread_s(&segmentTable, 256, 1, segmentCount, sample->m_opusFile) != segmentCount)
        {
            throw std::runtime_error("fread_s table");
        }

        sample->m_currentPosition += 27 + segmentCount;

        uint32_t currentSize = 0;

        for (int i = 0; i < segmentCount; i++)
        {
            uint32_t inputMaxByteCount = 0;
            uint32_t outputMaxByteCount;
            uint8_t* inputBuffer = sample->m_memoryManager.GetRawInputBuffer(sample->m_NumberOfBuffersProduced % MAX_BUFFER_COUNT, inputMaxByteCount);

            //Read the segment from disk
            if(fread_s((void*)((char*)inputBuffer + currentSize),
                inputMaxByteCount,
                1,
                segmentTable[i],
                sample->m_opusFile) != segmentTable[i])
            {
                throw std::runtime_error("fread_s seg");
            }

            currentSize += segmentTable[i];

            if (segmentTable[i] < 255)
            {
                //Final segment, so queue buffer for decode
                XApuDecodeConvertCommand command = {};
                command.id.type = XApuCommandType::Process;
                command.id.streamIndex = 0;
                command.id.sequence = sample->m_NumberOfBuffersProduced + 1;
                command.inputData = inputBuffer;
                command.inputDataLength = currentSize;
                command.outputData = sample->m_memoryManager.GetRawOutputBuffer(sample->m_NumberOfBuffersConsumed % MAX_BUFFER_COUNT, outputMaxByteCount);
                command.maxOutputDataLength = outputMaxByteCount;
                command.targetPitch = sample->m_pitch;
                command.pitchRampRate = XAPU_DEFAULT_PITCH_RAMP_RATE;

                sample->m_currentPosition += command.inputDataLength;

                while (!sample->m_terminateThread)
                {
                    while (sample->m_NumberOfBuffersProduced > sample->m_NumberOfBuffersConsumed)
                    {
                        // We reached our capacity to stream in data - we should wait for XAudio2 to finish
                        // processing at least one buffer.
                        // At this point we could go to sleep, or do something else.
                        // For the purposes of this sample, we'll just yield.
                        SwitchToThread();
                    }

                    HRESULT hr = XApuEnqueueCommand(sample->m_xapuHandle, &command.id, nullptr);
                    if (hr == XAPU_E_QUEUE_FULL)
                    {
                        ::Sleep(10);
                    }
                    else
                    {
                        DX::ThrowIfFailed(hr);

                        // A buffer is ready.
                        sample->m_NumberOfBuffersProduced++;
                        break;
                    }
                }

                currentSize = 0;
            }
        }
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
    XApuResult result = {};

    while (!sample->m_terminateThread)
    {
        while (sample->m_NumberOfBuffersProduced == sample->m_NumberOfBuffersConsumed)
        {
            if (sample->m_NumberOfBuffersProduced == 0)
            {
                HRESULT hr = XApuDequeueResult(sample->m_xapuHandle, &result);
                if (SUCCEEDED(hr))
                {
                    //Ensure we are done with activation
                    if (result.id.type != XApuCommandType::Activate)
                    {
                        throw std::runtime_error("SubmitAudioBufferThread");
                    }
                }
            }

            // There are no buffers ready at this time - we should wait for the ReadFile thread to stream in data.
            // At this point we could go to sleep, or do something else.
            // For the purposes of this sample, we'll just yield.
            SwitchToThread();
        }

#ifndef WRITETOFILE
        // Wait for XAudio2 to be ready - we need at least one free spot inside XAudio2's queue.
        for (;;)
        {
            XAUDIO2_VOICE_STATE state;

            sample->m_pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

            if (state.BuffersQueued < MAX_BUFFER_COUNT - 1)
                break;

            WaitForSingleObject(sample->m_VoiceContext.m_hBufferEndEvent, INFINITE);
        }
#endif

        // Allocate memory to stream in data.
        // In a game you would probably acquire this from a memory pool.
        // For the purposes of this sample, we'll allocate it here and have the XAudio2 callback free it later.
#ifndef WRITETOFILE
        uint8_t* pbBuffer = nullptr;
#endif
        result = {};

        // Stream in the PCM data.
        HRESULT hr = XApuDequeueResult(sample->m_xapuHandle, &result);
        if (SUCCEEDED(hr))
        {
            if (result.id.type == XApuCommandType::Process)
            {
#ifndef WRITETOFILE
                pbBuffer = new uint8_t[result.outputDataLength];
                memcpy(pbBuffer, result.outputData, result.outputDataLength);
#else
                //Write to file
                fwrite(result.outputData, sizeof(char), result.outputDataLength, sample->m_outFile);
#endif

                if (result.outputDataLength > 0)
                {
#ifndef WRITETOFILE
                    XAUDIO2_BUFFER buffer = {};
                    buffer.AudioBytes = result.outputDataLength;
                    buffer.pAudioData = pbBuffer;
                    if (sample->m_currentPosition >= (uint32_t)(sample->m_waveSize))
                    {
                        buffer.Flags = XAUDIO2_END_OF_STREAM;
                    }

                    // Point pContext at the allocated buffer so that we can free it in the OnBufferEnd() callback
                    buffer.pContext = pbBuffer;

                    // Now we have at least one spot free in our buffer queue, and at least one spot free
                    // in XAudio2's queue, so submit the next buffer.
                    DX::ThrowIfFailed(sample->m_pSourceVoice->SubmitSourceBuffer(&buffer));

                    // Check if this is the last buffer.
                    if (buffer.Flags == XAUDIO2_END_OF_STREAM)
                    {
                        // We are done.
                        sample->m_isPlaying = false;
                        break;
                    }
#endif
                    // A buffer is free.
                    sample->m_NumberOfBuffersConsumed++;
                }
            }
            else if (hr != XAPU_E_PENDING_RESULTS)
            {
                DX::ThrowIfFailed(hr);
            }
        }

        if (sample->m_currentPosition >= (uint32_t)(sample->m_waveSize))
        {
#ifdef WRITETOFILE
            //Close audio output file
            if (sample->m_outFile)
            {
                fclose(sample->m_outFile);
            }

            sample->m_isPlaying = false;
#endif

            sample->m_terminateThread = true;
        }
    }

    return S_OK;
}
