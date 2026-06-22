//--------------------------------------------------------------------------------------
// SimpleXDSPFFT.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleXDSPFFT.h"
#include "xmem.h"

#include "ATGColors.h"

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
    m_producerThread(nullptr),
    m_consumerThread(nullptr),
    m_critErrorOccurred(false),
    m_terminateThread(false),
    m_dspClientHandle(nullptr),
    m_dspStreamHandle(nullptr),
    m_dspStatus(nullptr),
    m_dspBuffer(nullptr),
    m_lastBufferedCommandSequence(0),
    m_lastRetrievedCommandSequence(0),
    m_lastFinishedCommandSequence(0),
    m_currentEQIndex(0),
    m_showLines(false),
    m_keyDown(false),
    m_pMasteringVoice(nullptr),
    m_pSourceVoice(nullptr),
    m_waveData{},
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
    DX::ThrowIfFailed(DX::LoadWAVAudioFromFileEx(L"Assets/SineSweep20to20000.wav", m_waveFile, m_waveData));

    uint32_t inputFileDataLength = MAX_BLOCK_FRAME_SIZE * MAX_BUFFER_COUNT * sizeof(float);
    uint32_t totalBufferSize = 2 * inputFileDataLength;
    float* inputDSPBuffers[MAX_BUFFER_COUNT];
    float* outputDSPBuffers[MAX_BUFFER_COUNT];
    
    if (totalBufferSize < 64 * 1024)
    {
        totalBufferSize = 64 * 1024;
    }

    m_dspBuffer = static_cast<float*>(XMemAlloc(totalBufferSize, XMemAllocAttributes));
    if (!m_dspBuffer)
    {
        throw std::bad_alloc();
    }
    DX::ThrowIfFailed(XDspConnect(m_dspBuffer, totalBufferSize, 0, &m_dspClientHandle));

    for (int i = 0; i < MAX_BUFFER_COUNT; i++)
    {
        inputDSPBuffers[i] = m_dspBuffer + (MAX_BLOCK_FRAME_SIZE * i);
        outputDSPBuffers[i] = inputDSPBuffers[i] + inputFileDataLength/sizeof(float);
    }

    m_DSPBuffers.SetBuffers(inputDSPBuffers, outputDSPBuffers);

    XDspActivationParameters params;
    params.type = XDspProcessType::ForwardFourierTransform;
    params.options = XDspActivationOptions::None;
    params.blockFrameCount = MAX_BLOCK_FRAME_COUNT;
    params.channelCount = 1; // mono
    params.impulseResponseLengthInFloats = 0;
    params.impulseResponse = nullptr;

    DX::ThrowIfFailed(XDspActivate(m_dspClientHandle, &params, &m_dspStatus, &m_dspStreamHandle));

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
            if (pad.IsAPressed())
            {
                m_keyDown = true;
                m_showLines = !m_showLines;
            }
        }
        else if (!pad.IsAPressed() && !pad.IsYPressed())
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

    m_font->DrawString(m_spriteBatch.get(), L"Real-time FFT Sample", pos);

    pos.y = 940;

    for (int i = 0; i < EQ_BUCKET_COUNT; i++)
    {
        pos.x = (float)305 + (i*144);
        m_font->DrawString(m_spriteBatch.get(), EQBucketsStrings[i], pos);
    }

    m_spriteBatch->End();

    RenderEQ(commandList);
 
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

    {
        EffectPipelineStateDescription pd(&VertexPositionColor::InputLayout, CommonStates::Opaque, CommonStates::DepthNone, CommonStates::CullNone, rtState, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
        m_batchEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
    }

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

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

void XM_CALLCONV Sample::RenderEQ(ID3D12GraphicsCommandList* commandList)
{
    m_batchEffect->Apply(commandList);
    m_batch->Begin(commandList);

    if (m_showLines)
    {
        VertexPositionColor lineLeft, lineRight;
        float leftHeight, rightHeight;

        lineLeft.position.z = lineRight.position.z = 0.f;
            
        lineLeft.color = lineRight.color = XMFLOAT4(ATG::Colors::White);

        for (int i = 0; i < EQ_BUCKET_COUNT - 1; i++)
        {
            //Draw EQ[i]
            lineLeft.position.x = .15f * i - .7f;
            lineRight.position.x = .15f * (i + 1) - .7f;

            leftHeight = rightHeight = 0;

            for (int j = 0; j < EQ_BUCKET_DEPTH; j++)
            {
                leftHeight += m_EQ[j][i];
                rightHeight += m_EQ[j][i+1];
            }

            lineLeft.position.y = (leftHeight / (EQ_BUCKET_DEPTH * 100.f)) - .6f;
            lineRight.position.y = (rightHeight / (EQ_BUCKET_DEPTH * 100.f)) - .6f;

            m_batch->DrawLine(lineLeft, lineRight);
        }
    }
    else
    {
        VertexPositionColor bottomLeft, bottomRight, topLeft, topRight, lineLeft, lineRight;
        float height = 0;
        float maxHeight = 0;

        bottomLeft.position.z = bottomRight.position.z = topLeft.position.z
            = topRight.position.z = lineLeft.position.z = lineRight.position.z = 0.f;
        bottomLeft.position.y = bottomRight.position.y = -.7f;

        bottomLeft.color = bottomRight.color = topLeft.color = topRight.color = XMFLOAT4(ATG::Colors::Green);
        lineLeft.color = lineRight.color = XMFLOAT4(ATG::Colors::Orange);

        for (int i = 0; i < EQ_BUCKET_COUNT; i++)
        {
            //Draw EQ[i]
            bottomLeft.position.x = topLeft.position.x = lineLeft.position.x = .15f * i - .7f;
            bottomRight.position.x = topRight.position.x = lineRight.position.x = topLeft.position.x + .08f;

            height = 0;
            maxHeight = 0;

            for (int j = 0; j < EQ_BUCKET_DEPTH; j++)
            {
                height += m_EQ[j][i];
                maxHeight = std::max(maxHeight, (float)m_EQ[j][i]);
            }

            topLeft.position.y = topRight.position.y = (height / (EQ_BUCKET_DEPTH * 100.f)) - .6f;
            lineLeft.position.y = lineRight.position.y = (maxHeight / 100.f) - .6f;

            m_batch->DrawTriangle(bottomLeft, bottomRight, topLeft);
            m_batch->DrawTriangle(topLeft, topRight, bottomRight);
            m_batch->DrawLine(lineLeft, lineRight);
        }
    }

    m_batch->End();
}

//--------------------------------------------------------------------------------------
// Name: SendConvCommandThread()
// Desc: Sends convolution commands
//--------------------------------------------------------------------------------------
DWORD WINAPI Sample::SendConvCommandThread(LPVOID lpParam)
{
    SetThreadDescription(GetCurrentThread(), L"SendConvCommandThread");

    auto sample = static_cast<Sample*>(lpParam);

    HRESULT hr = S_OK;

    while (!sample->m_terminateThread)
    {
        while (sample->m_lastBufferedCommandSequence - sample->m_lastFinishedCommandSequence >= MAX_BUFFER_COUNT)
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

                XDspCommand command;
                command.inputBlockBuffer = inputBuffer;
                command.outputBlockBuffer = outputBuffer;
                command.blockBufferMultiplier = nullptr;

                hr = XDspSubmitCommand(sample->m_dspStreamHandle, &command, &sequence);
                if (SUCCEEDED(hr))
                {
                    assert(sequence == sample->m_currentId);

                sample->m_currentId++;
                sample->m_lastBufferedCommandSequence = sequence;
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
        while (sample->m_lastBufferedCommandSequence == sample->m_lastFinishedCommandSequence)
        {
            // There are no buffers ready at this time - we should wait for the ReadFile thread to stream in data.
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

            sample->m_DSPBuffers.ReleaseOldestBuffer();
            sample->m_lastFinishedCommandSequence++;
        }

        DX::ThrowIfFailed(sample->m_dspStatus->result);

        if (sample->m_dspStatus->lastProcessedCommandSequence > sample->m_lastRetrievedCommandSequence)
        {
            uint32_t count = sample->m_lastBufferedCommandSequence - sample->m_lastRetrievedCommandSequence;
            for (uint32_t buffs = 0; buffs < count; buffs++)
            {
                float* inBuffer = {};
                float* outBuffer = {};
                if (sample->m_DSPBuffers.GetBuffers(sample->m_lastRetrievedCommandSequence + 1, &inBuffer, &outBuffer))
                {
                    sample->m_lastRetrievedCommandSequence++;

                    XAUDIO2_BUFFER buffer = {};
                    buffer.AudioBytes = MAX_BLOCK_FRAME_SIZE;
                    buffer.pAudioData = (BYTE*)inBuffer;
                    buffer.pContext = inBuffer;

                    DX::ThrowIfFailed(sample->m_pSourceVoice->SubmitSourceBuffer(&buffer));

                    int currentBucket = 0;
                    float real, imag, mag, freq;
                    float freqScale = (float)sample->m_waveData.wfx->nSamplesPerSec / MAX_BLOCK_FRAME_COUNT;

                    sample->m_currentEQIndex = (sample->m_currentEQIndex + 1) % EQ_BUCKET_DEPTH;
                    ZeroMemory(&sample->m_EQ[sample->m_currentEQIndex], EQ_BUCKET_COUNT * sizeof(float));

                    //Bucketize FFT result
                    for (uint32_t i = 0; i < MAX_BLOCK_FRAME_SIZE/2; i++)
                    {
                        freq = i * freqScale;
                        
                        if (freq > EQ_MAX_FREQ)
                        {
                            //Out of range
                            break;
                        }
                        else if (freq >= EQBuckets[0])
                        {
                            real = outBuffer[i * 2];
                            imag = outBuffer[i * 2 + 1];
                            
                            mag = sqrt((real * real) + (imag * imag));
                            
                            //Put the mag in each freq
                            while (currentBucket < EQ_BUCKET_COUNT && freq > EQBuckets[currentBucket + 1])
                            {
                                currentBucket++;
                            }

                            sample->m_EQ[sample->m_currentEQIndex][currentBucket] = std::max((float)sample->m_EQ[sample->m_currentEQIndex][currentBucket], (float)mag);
                        }
                    }
                }
            }
        }
    }

    return S_OK;
}
