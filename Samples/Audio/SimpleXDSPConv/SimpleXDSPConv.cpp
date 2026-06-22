//--------------------------------------------------------------------------------------
// SimpleXDSPConv.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleXDSPConv.h"
#include "xmem.h"

#include "ATGColors.h"

namespace
{
    const wchar_t* g_IRFileList[IR_NUM] = {
        L"Assets/ir_short.wav",
        L"Assets/ir_long.wav"
    };
}

//Required XMem attributes for XDSP buffers
static const ULONGLONG XMemAllocAttributes = MAKE_XALLOC_ATTRIBUTES(eXALLOCAllocatorId_Audio,
    0,
    XALLOC_MEMTYPE_PHYSICAL_CACHEABLE,
    XALLOC_PAGESIZE_64KB,
    XALLOC_ALIGNMENT_64K,
   FALSE);

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_currentLevel(0),
    m_keyDown(false),
    m_producerThread(nullptr),
    m_consumerThread(nullptr),
    m_critErrorOccurred(false),
    m_terminateThread(false),
    m_dspClientHandle(nullptr),
    m_dspStreamHandle{},
    m_dspStatus{},
    m_dspBuffer(nullptr),
    m_lastBufferedCommandSequence{},
    m_lastRetrievedCommandSequence{},
    m_totalQueuedBuffers(0),
    m_totalFinishedBuffers(0),
    m_pMasteringVoice(nullptr),
    m_pSourceVoice(nullptr),
    m_waveData{},
    m_irData{},
    m_currentPosition(0),
    m_currentId(1)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
}

Sample::~Sample()
{
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

        XDspDisconnect(m_dspClientHandle);

        if (m_dspBuffer)
        {
            XMemFree(m_dspBuffer, XMemAllocAttributes);
            m_dspBuffer = nullptr;
        }
    }

#ifdef WRITEFILE
    m_pWaveFile->Close();
#endif
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

    //Load source audio
    DX::ThrowIfFailed(DX::LoadWAVAudioFromFileEx(L"Assets/footsteps.wav", m_waveFile, m_waveData));

    //Load impulse responses
    for (int i = 0; i < IR_NUM; i++)
    {
        DX::ThrowIfFailed(DX::LoadWAVAudioFromFileEx(g_IRFileList[i], m_irFile[i], m_irData[i]));
    };

    uint32_t inputBufferLength = MAX_BLOCK_FRAME_COUNT * MAX_BUFFER_COUNT * sizeof(float);
    uint32_t outputBufferLength = MAX_BLOCK_FRAME_COUNT * MAX_BUFFER_COUNT * IR_NUM * sizeof(float);
    uint32_t totalBufferLength = inputBufferLength + outputBufferLength;
    float* xdspOutputDataBuffer;
    float* inputDSPBuffers[MAX_BUFFER_COUNT];
    float* outputDSPBuffers[MAX_BUFFER_COUNT * IR_NUM];

    if (totalBufferLength < 64 * 1024)
    {
        totalBufferLength = 64 * 1024;
    }

    //Allocate I/O buffers and connect
    m_dspBuffer = static_cast<float*>(XMemAlloc(totalBufferLength, XMemAllocAttributes));
    
    if (!m_dspBuffer)
    {
        throw std::bad_alloc();
    }

    DX::ThrowIfFailed(XDspConnect(m_dspBuffer,
        totalBufferLength,
        4,
        &m_dspClientHandle));

    xdspOutputDataBuffer = m_dspBuffer + (MAX_BLOCK_FRAME_COUNT * MAX_BUFFER_COUNT);

    for (int j = 0; j < MAX_BUFFER_COUNT; j++)
    {
        inputDSPBuffers[j] = m_dspBuffer + (j * MAX_BLOCK_FRAME_COUNT);
        outputDSPBuffers[j] = xdspOutputDataBuffer + (j * MAX_BLOCK_FRAME_COUNT);

        for (int k = 1; k < IR_NUM; k++)
        {
            outputDSPBuffers[j + (MAX_BUFFER_COUNT * k)] = outputDSPBuffers[j] + k * (MAX_BUFFER_COUNT * MAX_BLOCK_FRAME_COUNT);
        }
    }

    m_DSPBuffers.SetBuffers(inputDSPBuffers, outputDSPBuffers);

    //Load IRs
    uint32_t filterLength[IR_NUM];

    for (int i = 0; i < IR_NUM; i++)
    {
        filterLength[i] = (m_irData[i].audioBytes + 15) & ~0xF;

        float* irbuffer = static_cast<float*>(XMemAlloc(filterLength[i], XMemAllocAttributes));
        if (!irbuffer)
        {
            throw std::bad_alloc();
        }
        memcpy(irbuffer, m_irData[i].startAudio, filterLength[i]);

        XDspActivationParameters params;
        params.type = XDspProcessType::Convolution;
        params.options = XDspActivationOptions::None;
        params.blockFrameCount = MAX_BLOCK_FRAME_COUNT;
        params.channelCount = 1; // mono
        params.impulseResponseLengthInFloats = filterLength[i] / sizeof(float);
        params.impulseResponse = irbuffer;

        DX::ThrowIfFailed(XDspActivate(m_dspClientHandle, &params, &m_dspStatus[i], &m_dspStreamHandle[i]));
        if (!irbuffer)
        {
            throw std::bad_alloc();
        }
        XMemFree(irbuffer, XMemAllocAttributes);
    }

#ifdef WRITEFILE
    m_pWaveFile = new CWaveFileWriter();
    m_pWaveFile->Open(L"Recording.wav", m_waveData.wfx);
#endif

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

    // Create the source voice
    DX::ThrowIfFailed(m_pXAudio2->CreateSourceVoice(&m_pSourceVoice, m_waveData.wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_voiceContext));

    // Start the voice.
    DX::ThrowIfFailed(m_pSourceVoice->Start());

    // Create the producer thread (reads PCM chunks from disk)
    m_producerThread = CreateThread(nullptr, 0, Sample::SendConvCommandThread, this, 0, nullptr);
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

    if (m_critErrorOccurred)
    {
        m_critErrorOccurred = false;
        InitializeXAudio();
    }

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (!m_keyDown)
        {
            if (pad.IsDPadUpPressed())
            {
                m_keyDown = true;
                if (m_currentLevel != 0)
                {
                    m_currentLevel--;
                }
            }
            else if (pad.IsDPadDownPressed())
            {
                m_keyDown = true;
                if (m_currentLevel != 20)
                {
                    m_currentLevel++;
                }
            }

        }
        else if (!pad.IsDPadUpPressed() && !pad.IsDPadDownPressed())
        {
            m_keyDown = false;
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

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    m_font->DrawString(m_spriteBatch.get(), L"Real-time Convolution Sample", pos);
    pos.y += m_font->GetLineSpacing() * 1.5f;

    if (m_currentLevel == 0)
    {
        m_font->DrawString(m_spriteBatch.get(), L"<Off>", pos);
    }
    else
    {
        m_font->DrawString(m_spriteBatch.get(), L"  Off", pos);
    }
    pos.y += m_font->GetLineSpacing() * 1.f;

    for (int i = 1; i < 10; i++)
    {
        if (m_currentLevel == i)
        {
            m_font->DrawString(m_spriteBatch.get(), L"<.>", pos);
        }
        else
        {
            m_font->DrawString(m_spriteBatch.get(), L"  .", pos);
        }
        pos.y += m_font->GetLineSpacing() * 1.f;
    }

    if (m_currentLevel == 10)
    {
        m_font->DrawString(m_spriteBatch.get(), L"<Short>", pos);
    }
    else
    {
        m_font->DrawString(m_spriteBatch.get(), L"  Short", pos);
    }
    pos.y += m_font->GetLineSpacing() * 1.f;

    for (int i = 11; i < 20; i++)
    {
        if (m_currentLevel == i)
        {
            m_font->DrawString(m_spriteBatch.get(), L"<.>", pos);
        }
        else
        {
            m_font->DrawString(m_spriteBatch.get(), L"  .", pos);
        }
        pos.y += m_font->GetLineSpacing() * 1.f;
    }

    if (m_currentLevel == 20)
    {
        m_font->DrawString(m_spriteBatch.get(), L"<Long>", pos);
    }
    else
    {
        m_font->DrawString(m_spriteBatch.get(), L"  Long", pos);
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
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

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
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
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

//--------------------------------------------------------------------------------------
// Name: SendConvCommandThread()
// Desc: Sends convolution commands
//--------------------------------------------------------------------------------------
DWORD WINAPI Sample::SendConvCommandThread(LPVOID lpParam)
{
    SetThreadDescription(GetCurrentThread(), L"SendConvCommandThread");

    auto sample = static_cast<Sample*>(lpParam);

    while (!sample->m_terminateThread)
    {
        while (sample->m_totalQueuedBuffers - sample->m_totalFinishedBuffers >= MAX_BUFFER_COUNT)
        {
            // We reached our capacity to stream in data - we should wait for XAudio2 to finish
            // processing at least one buffer.
            // At this point we could go to sleep, or do something else.
            // For the purposes of this sample, we'll just yield.
            SwitchToThread();
        }

        float* inputBuffer = {};
        float* outputBuffer = {};
        uint32_t sequence = 0;

        if (sample->m_DSPBuffers.AcquireBuffers(sample->m_currentId, &inputBuffer, &outputBuffer))
        {
            //Fill input buffer
            if (sample->m_currentPosition + MAX_BLOCK_FRAME_COUNT > sample->m_waveData.GetSampleDuration())
            {
                //We reached the end of file, loop
                size_t blockSize = sample->m_waveData.GetSampleDuration() - sample->m_currentPosition;

                memcpy(inputBuffer, (float*)sample->m_waveData.startAudio + sample->m_currentPosition, blockSize);
                memcpy(inputBuffer + blockSize, sample->m_waveData.startAudio, MAX_BLOCK_FRAME_SIZE - blockSize);

                sample->m_currentPosition = MAX_BLOCK_FRAME_COUNT - (uint32_t)blockSize;
            }
            else
            {
                memcpy(inputBuffer, (float*)sample->m_waveData.startAudio + sample->m_currentPosition, MAX_BLOCK_FRAME_SIZE);
                sample->m_currentPosition += MAX_BLOCK_FRAME_COUNT;
            }

            bool success = true;
            XDspCommand command;
            command.beginScale = 1.0f;
            command.endScale = 1.0f;
            command.inputBlockBuffer = inputBuffer;
            command.outputBlockBuffer = outputBuffer;

            for (int i = 0; i < IR_NUM && success; i++)
            {
                success = SUCCEEDED(XDspSubmitCommand(sample->m_dspStreamHandle[i], &command, &sequence));

                if (!success)
                {
                    break;
                }

                sample->m_lastBufferedCommandSequence[i] = sequence;
                command.outputBlockBuffer += MAX_BUFFER_COUNT * MAX_BLOCK_FRAME_COUNT;
            }

            if (success)
            {
                sample->m_currentId++;
                sample->m_totalQueuedBuffers++;
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

    while (!sample->m_terminateThread)
    {
        while (sample->m_totalQueuedBuffers == sample->m_totalFinishedBuffers)
        {
            // There are no buffers ready at this time
            // At this point we could go to sleep, or do something else.
            // For the purposes of this sample, we'll just yield.
            SwitchToThread();
        }

        // Wait for XAudio2 to be ready - we need at least one free spot inside XAudio2's queue.
        for (;;)
        {
            XAUDIO2_VOICE_STATE state;

            sample->m_pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

            if (state.BuffersQueued < MAX_BUFFER_COUNT - 1)
                break;

            WaitForSingleObject(sample->m_voiceContext.m_hBufferEndEvent, INFINITE);

            sample->m_totalFinishedBuffers++;
            sample->m_DSPBuffers.ReleaseOldestBuffer();
        }

        bool bufferReady = true;
        float* totalOutBuffer;
        float* tempOutBuffer;

        for (int i = 0; i < IR_NUM; i++)
        {
            DX::ThrowIfFailed(sample->m_dspStatus[i]->result);

            if (sample->m_dspStatus[i]->lastProcessedCommandSequence <= sample->m_lastRetrievedCommandSequence[i])
            {
                bufferReady = false;
                break;
            }
        }

        if (bufferReady)
        {
            if (sample->m_DSPBuffers.GetBuffers(sample->m_lastRetrievedCommandSequence[0] + 1, &totalOutBuffer, &tempOutBuffer))
            {
                for (int i = 0; i < IR_NUM; i++)
                {
                    sample->m_lastRetrievedCommandSequence[i]++;
                }

                if (sample->m_currentLevel > 10)
                {
                    int localLevel = sample->m_currentLevel - 10;

                    //IR 0 + IR 1
                    for (uint32_t j = 0; j < MAX_BLOCK_FRAME_COUNT; j++)
                    {
                        totalOutBuffer[j] =
                            (tempOutBuffer[j] * (1 - localLevel / 10)) +
                            ((tempOutBuffer + MAX_BUFFER_COUNT*MAX_BLOCK_FRAME_COUNT)[j] * localLevel / 10);
                    }
                }
                else
                {
                    //IR 0 only
                    for (uint32_t j = 0; j < MAX_BLOCK_FRAME_COUNT; j++)
                    {
                        totalOutBuffer[j] =
                            (totalOutBuffer[j] * (1 - sample->m_currentLevel / 10)) +
                            (tempOutBuffer[j] * sample->m_currentLevel / 10);
                    }
                }

                XAUDIO2_BUFFER buffer = {};
                buffer.AudioBytes = MAX_BLOCK_FRAME_SIZE;
                buffer.pAudioData = (BYTE*)totalOutBuffer;
                buffer.pContext = totalOutBuffer;

                DX::ThrowIfFailed(sample->m_pSourceVoice->SubmitSourceBuffer(&buffer));

#ifdef WRITEFILE
                sample->m_pWaveFile->WriteSample(totalOutBuffer, MAX_BLOCK_FRAME_SIZE, nullptr);
#endif
            }
        }
    }

    return S_OK;
}
