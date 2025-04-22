//--------------------------------------------------------------------------------------
// MP4Reader.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MP4Reader.h"

#include "ATGColors.h"
#include "ControllerFont.h"

#include "codecapi.h"

// The m_useSoftwareDecode variable allows for software decoding. For example, the HW decoder only supports
// 4K decoding when using HEVC, so if you want to decode 4K H.264, then you can enable software decode

// Define the input url path here, use local file by default. 
// Can also change the URL to remote MP4 or smoothstreaming or http live streaming URLs as below:
//
// Http live streaming url example:
// #define INPUT_FILE_PATH L"https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/bipbop_4x3_variant.m3u8"
//
// Smooth streaming url example:
// #define INPUT_FILE_PATH L"http://playready.directtaps.net/smoothstreaming/SSWSS720H264/SuperSpeedway_720.ism/Manifest"
//

// This video is AVC-1080p.
// Compatible with Software and Hardware decoding.
// Supported by XBoxOne and Scarlett.
#define INPUT_FILE_PATH  L"FluidParticles_1080p_AVC.mp4"

// This video is HEVC-UHD.
// Only compatible with Hardware decoding.
// Supported only by Scarlett.
// #define INPUT_FILE_PATH  L"FluidParticles_UHD_HEVC.mp4"

extern void ExitSample() noexcept;

using namespace DirectX;
using Microsoft::WRL::ComPtr;

constexpr LONGLONG g_100nsInSecs = 10000000ll;  // Media time is defined in 100 nanoseconds time intervals

int64_t GetCurrentTimeInHNS()
{
    LARGE_INTEGER m_liCurrent;
    static LARGE_INTEGER s_liFrequency = {};
    if (s_liFrequency.QuadPart == 0)
    {
        QueryPerformanceFrequency(&s_liFrequency);
    }

    QueryPerformanceCounter(&m_liCurrent);
    return MFllMulDiv(m_liCurrent.QuadPart, g_100nsInSecs, s_liFrequency.QuadPart, 0);
}

namespace
{
    // For D3D12, application must call MFD3D12GpuSignalForSampleFree after finished processing the video sample
    HRESULT MFD3D12GpuSignalSampleFree(
        _In_ ID3D12CommandQueue* pCmdQueue,
        _In_ IMFSample* pVideoSample)
    {       
        ComPtr<IMFMediaBuffer> pBuffer;
        HRESULT hr = pVideoSample->GetBufferByIndex(0, &pBuffer);
        if (SUCCEEDED(hr))
        {
            ComPtr<IMFDXGIBuffer>  pDXGIBuffer;
            hr = pBuffer->QueryInterface<IMFDXGIBuffer>(&pDXGIBuffer);
            if (SUCCEEDED(hr))
            {
                ComPtr<IMFD3D12SynchronizationObjectCommands> pMFSyncObj;
                hr = pDXGIBuffer->GetUnknown(MF_D3D12_SYNCHRONIZATION_OBJECT, IID_PPV_ARGS(&pMFSyncObj));
                if (SUCCEEDED(hr))
                {
                    //GPU signal the sample can be freed for decoding
                    hr = pMFSyncObj->EnqueueResourceRelease(pCmdQueue);
                }
            }
        }
        return hr;
    }

    // For D3D12, application must call MFD3D12GpuWaitForSampleReady to make sure the GPU waits for decode to complete before process the sample using GPU code
    HRESULT MFD3D12GpuWaitForSampleReady(
        _In_ ID3D12CommandQueue* pCmdQueue,
        _In_ IMFSample* pVideoSample)
    {
        ComPtr<IMFMediaBuffer> pBuffer;
        HRESULT hr = pVideoSample->GetBufferByIndex(0, &pBuffer);
        if (SUCCEEDED(hr))
        {
            ComPtr<IMFDXGIBuffer> pDXGIBuffer;
            hr = pBuffer->QueryInterface<IMFDXGIBuffer>(&pDXGIBuffer);
            if (SUCCEEDED(hr))
            {
                ComPtr<IMFD3D12SynchronizationObjectCommands> pMFSyncObj;
                hr = pDXGIBuffer->GetUnknown(MF_D3D12_SYNCHRONIZATION_OBJECT, IID_PPV_ARGS(&pMFSyncObj));
                if (SUCCEEDED(hr))
                {
                    // GPU wait until the decoding completed
                    hr = pMFSyncObj->EnqueueResourceReadyWait(pCmdQueue);
                }
            }
        }
        return hr;
    }
}

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_videoDone(false)
    , m_audioDone(false)
    , m_numberOfFramesDecoded(0)
    , m_videoWidth(0)
    , m_videoHeight(0)
#ifdef USE_WASAPI
    , m_pAudioClientWFX(nullptr)
#endif
    , m_bufferFrameCount(0)
    , m_pAudioReaderOutputWFX(nullptr)
    , m_pVideoRender(nullptr)
#ifdef USE_XAUDIO2
    , m_pXAudio2(nullptr)
    , m_pMasteringVoice(nullptr)
    , m_pSourceVoice(nullptr)
    , m_dwAudioFramesDecoded(0)
    , m_dwAudioFramesRendered(0)
    , m_terminateAudioThread(false)
    , m_suspendAudioThread(false)
    , m_audioThread(nullptr)
#endif
    , m_hasAudioStream(true)
    , m_audioStarted(false)
    , m_finished(false)
    , m_sampleLoops(true)
    , m_useSoftwareDecode(false)
    , m_seekLoop(true)
    , m_llStartTimeStamp(INVALID_SAMPLE_TIME)
    , m_llStartHNS(0)
    , m_llStopHNS(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);

#if DISABLE_AUDIO
    m_hasAudioStream = false;
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

    // Initialize the Media Foundation platform.
    DX::ThrowIfFailed(MFStartup(MF_VERSION));

    InitializeMedia();
}

void Sample::InitializeMedia()
{
    auto device = m_deviceResources->GetD3DDevice();
    ComPtr<IMFDXGIDeviceManager> pDXVAManager;
    ComPtr<IMFAttributes> pMFAttributes;
    ComPtr<IUnknown> punkDeviceMgr;

    uint32_t uResetToken = 0;

    // Call the MFCreateDXGIDeviceManager function to create the Direct3D device manager
    DX::ThrowIfFailed(MFCreateDXGIDeviceManager(&uResetToken, &pDXVAManager));

    // Call the MFResetDXGIDeviceManagerX function with a pointer to the Direct3D device
    DX::ThrowIfFailed(MFResetDXGIDeviceManagerX(pDXVAManager.Get(), device, uResetToken));

    // Create an attribute store
    DX::ThrowIfFailed(pDXVAManager->QueryInterface(IID_PPV_ARGS(&punkDeviceMgr)));
    DX::ThrowIfFailed(MFCreateAttributes(&pMFAttributes, 3));
    DX::ThrowIfFailed(pMFAttributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, punkDeviceMgr.Get()));
    DX::ThrowIfFailed(pMFAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));

    DX::ThrowIfFailed(pMFAttributes->SetUINT32(MF_SOURCE_READER_DISABLE_DXVA, m_useSoftwareDecode));
    // Don't set the MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING MFAttribute to TRUE. It is too slow.

    // Create the source reader.
    DX::ThrowIfFailed(MFCreateSourceReaderFromURL(INPUT_FILE_PATH, pMFAttributes.Get(), &m_pReader));
    ConfigureSourceReaderOutput(m_pReader.Get(), (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM);

    if (m_useSoftwareDecode)
    {
        // Example of setting threading parameters for software decode. This code restricts decoding to happen only on cores 0..3
        ComPtr<IMFTransform> transform;
        ComPtr<IMFAttributes> attributes;
        DX::ThrowIfFailed(m_pReader->GetServiceForStream((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, GUID_NULL, IID_PPV_ARGS(transform.GetAddressOf())));
        transform->GetAttributes(attributes.GetAddressOf());
        attributes->SetUINT32(CODECAPI_AVDecVideoThreadAffinityMask, 0x0F);         // Use the first 4 cores
        attributes->SetUINT32(CODECAPI_AVDecNumWorkerThreads, 0x4);                 // 4 threads
        attributes->SetUINT32(CODECAPI_AVPriorityControl, THREAD_PRIORITY_HIGHEST); // Set thread priority. Same as Win32 SetThreadPriority for priorty levels
    }

    ComPtr<IMFAttributes> pMFRenderAttributes;
    CXboxNV12ToRGBConverter* videoRenderPtr = nullptr;
    DX::ThrowIfFailed(CreateDxvaSampleRendererX(device, pMFRenderAttributes.Get(), &videoRenderPtr));
    m_pVideoRender = std::unique_ptr<CXboxNV12ToRGBConverter>(videoRenderPtr);
    m_pVideoRender->m_spDevice = device;

    // Now initialize Audio    
    InitializeAudio();

    ComPtr<IMFMediaType> pMediaType;

    if (m_hasAudioStream)
    {
        HRESULT hr = m_pReader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &pMediaType);
        if (FAILED(hr))
        {
            m_hasAudioStream = false;
        }
        else
        {
            uint32_t size = 0;
            ConfigureSourceReaderOutput(m_pReader.Get(), uint32_t(MF_SOURCE_READER_FIRST_AUDIO_STREAM));
            DX::ThrowIfFailed(m_pReader->GetCurrentMediaType(uint32_t(MF_SOURCE_READER_FIRST_AUDIO_STREAM), pMediaType.ReleaseAndGetAddressOf()));
            DX::ThrowIfFailed(MFCreateWaveFormatExFromMFMediaType(pMediaType.Get(), &m_pAudioReaderOutputWFX, &size, MFWaveFormatExConvertFlag_Normal));

#ifdef USE_WASAPI

            if (m_pAudioReaderOutputWFX->nSamplesPerSec != m_pAudioClientWFX->nSamplesPerSec && m_pAudioReaderOutputWFX->wBitsPerSample != m_pAudioClientWFX->wBitsPerSample)
            {
                // currently, WSAPI only supports 48hz float, title os does not have resampler for now, so we can only render 48khz content. 
                // title can use Xaudio2 for rendering, and xaudio2 has ability to convert different sample rate. 
                DX::ThrowIfFailed(MF_E_UNSUPPORTED_RATE);
            }

#elif defined(USE_XAUDIO2)

            DX::ThrowIfFailed(m_pXAudio2->CreateSourceVoice(&m_pSourceVoice, m_pAudioReaderOutputWFX, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_VoiceContext));

#endif  // USE_XAUDIO2

        }
    }
}

void Sample::InitializeAudio(bool reinit /*= false*/)
{
    if (reinit)
    {
        CleanupAudio();
    }

    // start audio render using WSAPI. 
    const CLSID clsidDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    const IID iidDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

#ifdef USE_WASAPI

    REFERENCE_TIME hnsRequestedDuration = g_100nsInSecs / 2;
    const IID iidIAudioRenderClient = __uuidof(IAudioRenderClient);
    const IID iidIAudioClient = __uuidof(IAudioClient);

#endif  // USE_WASAPI

    ComPtr<IMMDeviceEnumerator> pAudioEnumerator;
    ComPtr<IMMDevice>           pAudioDevice;

    DX::ThrowIfFailed(CoCreateInstance(
        clsidDeviceEnumerator, nullptr,
        CLSCTX_ALL, iidDeviceEnumerator,
        reinterpret_cast<void**>(pAudioEnumerator.ReleaseAndGetAddressOf())));

    DX::ThrowIfFailed(pAudioEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pAudioDevice));

#ifdef USE_WASAPI

    DX::ThrowIfFailed(pAudioDevice->Activate(iidIAudioClient, CLSCTX_ALL, nullptr, reinterpret_cast<void**>(m_pAudioClient.ReleaseAndGetAddressOf())));

    DX::ThrowIfFailed(m_pAudioClient->GetMixFormat(&m_pAudioClientWFX));

    DX::ThrowIfFailed(m_pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        hnsRequestedDuration,
        0,
        m_pAudioClientWFX,
        nullptr));

    // Get the actual size of the allocated buffer.
    DX::ThrowIfFailed(m_pAudioClient->GetBufferSize(&m_bufferFrameCount));
    DX::ThrowIfFailed(m_pAudioClient->GetService(iidIAudioRenderClient, reinterpret_cast<void**>(m_pAudioRenderClient.ReleaseAndGetAddressOf())));

#endif  // USE_WASAPI

#ifdef USE_XAUDIO2

    DX::ThrowIfFailed(XAudio2Create(&m_pXAudio2, 0));
    DX::ThrowIfFailed(m_pXAudio2->CreateMasteringVoice(&m_pMasteringVoice));

    if (m_pAudioReaderOutputWFX)
    {
        DX::ThrowIfFailed(m_pXAudio2->CreateSourceVoice(&m_pSourceVoice, m_pAudioReaderOutputWFX, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_VoiceContext));
    }

    m_audioResumeSignal.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!m_audioResumeSignal.IsValid())
    {
        throw std::exception("CreateEvent");
    }

    // Create the consumer thread (submits PCM chunks to XAudio2)
    m_audioThread = new std::thread(&Sample::SubmitAudioBufferThreadProc, this);

#endif  // USE_XAUDIO2
}

void Sample::ReleaseCurrentVideoSample()
{
    if (m_pOutputVideoSample)
    {
        if (!m_useSoftwareDecode)
        {
            DX::ThrowIfFailed(MFD3D12GpuSignalSampleFree(m_pVideoRender->GetVideoProcessCommandQueue(), m_pOutputVideoSample.Get()));
        }

        m_pOutputVideoSample.Reset();
    }
}

//--------------------------------------------------------------------------------------
// Name: ConfigureSourceReaderOutput()
// Desc: Configure the MFSourceReader output type
//--------------------------------------------------------------------------------------
void Sample::ConfigureSourceReaderOutput(IMFSourceReader* pReader, uint32_t dwStreamIndex)
{
    ComPtr<IMFMediaType> pNativeType;
    ComPtr<IMFMediaType> pType;
    GUID majorType, subtype;

    // Find the native format of the stream.
    DX::ThrowIfFailed(pReader->GetNativeMediaType(dwStreamIndex, 0, &pNativeType));

    // Find the major type.
    DX::ThrowIfFailed(pNativeType->GetGUID(MF_MT_MAJOR_TYPE, &majorType));

    // Define the output type.
    DX::ThrowIfFailed(MFCreateMediaType(&pType));

    DX::ThrowIfFailed(pType->SetGUID(MF_MT_MAJOR_TYPE, majorType));

    // Select a subtype.
    if (majorType == MFMediaType_Video)
    {
        // NV12 is the only supported output type of Xbox One HW decoders
        // Don't set the subtype to RGB32. It is too slow.
        subtype = MFVideoFormat_NV12;
    }
    else if (majorType == MFMediaType_Audio)
    {
        subtype = MFAudioFormat_Float;
    }
    else
    {
        // Unrecognized type. Skip.
        return;
    }

    DX::ThrowIfFailed(pType->SetGUID(MF_MT_SUBTYPE, subtype));

    // Set the uncompressed format.
    DX::ThrowIfFailed(pReader->SetCurrentMediaType(dwStreamIndex, nullptr, pType.Get()));
}

#ifdef USE_XAUDIO2

//--------------------------------------------------------------------------------------
// Name: SubmitAudioBufferThread()
// Desc: Submits audio buffers to XAudio2. Blocks when XAudio2's queue is full or our buffer queue is empty
//--------------------------------------------------------------------------------------
void Sample::SubmitAudioBufferThreadProc()
{
    auto threadhandle = GetCurrentThread();
    SetThreadDescription(threadhandle, L"SubmitAudioBufferThreadProc");

    // Set helper thread to CPU 1
    SetThreadAffinityMask(threadhandle, 0x2);

    m_terminateAudioThread = false;
    while (!m_terminateAudioThread)
    {
        if (m_suspendAudioThread)
        {
            std::ignore = WaitForSingleObject(m_audioResumeSignal.Get(), INFINITE);
        }

        // Yield to another thread if we have no frames to render
        if (m_dwAudioFramesRendered == m_dwAudioFramesDecoded || !m_audioStarted)
        {
            SwitchToThread();
        }
        else
        {
            // Wait for XAudio2 to be ready - we need at least one free spot inside XAudio2's queue.
            for (;;)
            {
                XAUDIO2_VOICE_STATE state;
                m_pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
                if (state.BuffersQueued < MP4R_XA2_MAX_BUFFER_COUNT - 1)
                    break;

                HANDLE events[2] = { m_audioResumeSignal.Get(), m_VoiceContext.m_hBufferEndEvent };
                WaitForMultipleObjects(_countof(events), events, false, INFINITE);
            }

            // Now we have at least one spot free in our buffer queue, and at least one spot free
            // in XAudio2's queue, so submit the next buffer.
            XAUDIO2_BUFFER buffer = m_Buffers[m_dwAudioFramesRendered % MP4R_XA2_MAX_BUFFER_COUNT];
            DX::ThrowIfFailed(m_pSourceVoice->SubmitSourceBuffer(&buffer));
            ++m_dwAudioFramesRendered;
        }

        if (m_audioDone)
        {
            OutputDebugString(L"Audio is done. Shutting down SubmitAudioBufferThread.\n");
            m_terminateAudioThread = true;
        }
    }
}

#endif  // USE_XAUDIO2


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

    auto pad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        using ButtonState = GamePad::ButtonStateTracker;
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();

#ifdef USE_XAUDIO2
            m_terminateAudioThread = true;
#endif
        }

        if (m_gamePadButtons.a == ButtonState::RELEASED)
        {
            m_sampleLoops = !m_sampleLoops;
        }

        if (m_gamePadButtons.b == ButtonState::RELEASED)
        {
            m_seekLoop = !m_seekLoop;
        }

        if (m_gamePadButtons.x == ButtonState::RELEASED)
        {
            m_useSoftwareDecode = !m_useSoftwareDecode;
            ResetAndCreatePlayer();
        }

        if (m_gamePadButtons.y == ButtonState::RELEASED)
        {
            m_hasAudioStream = !m_hasAudioStream;
            ResetAndCreatePlayer();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // If the media finished and the sample is set to loop, then it will
    // loop using the chosen method (seek vs recreating media player).
    if (m_finished && m_sampleLoops)
    {
        if (m_seekLoop)
        {
            Seek(0);
        }
        else
        {
            ResetAndCreatePlayer();
        }

        m_finished = false;
    }

    ProcessAudio();
    ProcessVideo();
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

    XMVECTORF32 YellowColor = { { { 1.0f, 1.0f, 0.0f, 1.0f } } };

    // Set the descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    if (m_pOutputVideoSample)
    {
        // Must occur first since video processor writes to entire visible screen area
        RenderVideoFrame(m_pOutputVideoSample.Get());
    }

    m_spriteBatch->Begin(commandList);

    auto const size = m_deviceResources->GetOutputSize();
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    DX::DrawControllerString(m_spriteBatch.get(), m_overlayFont.get(), m_controllerFont.get(), L"Press [view] to exit the sample...\n",
        XMFLOAT2(float(safe.left), float(safe.bottom) - 5.0f * m_overlayFont->GetLineSpacing()), YellowColor);

    DX::DrawControllerString(m_spriteBatch.get(), m_overlayFont.get(), m_controllerFont.get(), L"Press [X] to toggle software/hardware decoding.\n",
        XMFLOAT2(float(safe.left), float(safe.bottom) - 4.0f * m_overlayFont->GetLineSpacing()), YellowColor);

    DX::DrawControllerString(m_spriteBatch.get(), m_overlayFont.get(), m_controllerFont.get(), L"Press [Y] to toggle audio.\n",
        XMFLOAT2(float(safe.left), float(safe.bottom) - 3.0f * m_overlayFont->GetLineSpacing()), YellowColor);

    DX::DrawControllerString(m_spriteBatch.get(), m_overlayFont.get(), m_controllerFont.get(), L"Press [A] to toggle looping.\n",
        XMFLOAT2(float(safe.left), float(safe.bottom) - 2.0f * m_overlayFont->GetLineSpacing()), YellowColor);

    if (m_sampleLoops)
    {
        DX::DrawControllerString(m_spriteBatch.get(), m_overlayFont.get(), m_controllerFont.get(), L"Press [B] to toggle loop method.\n",
            XMFLOAT2(float(safe.left), float(safe.bottom) - m_overlayFont->GetLineSpacing()), YellowColor);
    }

    const float yInc = m_overlayFont->GetLineSpacing() * 1.5f;
    SimpleMath::Vector2 overlayPos(float(safe.left), float(safe.top));

    wchar_t buffer[128] = {};
    if (m_pOutputVideoSample)
    {
        if (m_videoDone && m_audioDone)
        {
            m_overlayFont->DrawString(m_spriteBatch.get(), L"Decoding has finished.", overlayPos, YellowColor);
        }
        else
        {
            m_overlayFont->DrawString(m_spriteBatch.get(), L"Decoding is in progress.", overlayPos, YellowColor);
        }
        overlayPos.y += yInc;

        swprintf_s(buffer, _countof(buffer), L"Video frame size is %dx%d", m_videoWidth, m_videoHeight);
        m_overlayFont->DrawString(m_spriteBatch.get(), buffer, overlayPos, YellowColor);
        overlayPos.y += yInc;

        swprintf_s(buffer, _countof(buffer), (m_useSoftwareDecode) ? L"Using Software Decoding" : L"Using Media Foundation");
        m_overlayFont->DrawString(m_spriteBatch.get(), buffer, overlayPos, YellowColor);
        overlayPos.y += yInc;

        swprintf_s(buffer, _countof(buffer), (m_sampleLoops) ? L"Looping Enabled" : L"Looping Disabled");
        m_overlayFont->DrawString(m_spriteBatch.get(), buffer, overlayPos, YellowColor);
        overlayPos.y += yInc;

        if (m_sampleLoops)
        {
            swprintf_s(buffer, _countof(buffer), (m_seekLoop) ? L"Loop Uses Seek" : L"Loops Restarts Media Player");
            m_overlayFont->DrawString(m_spriteBatch.get(), buffer, overlayPos, YellowColor);
            overlayPos.y += yInc;
        }

#ifdef USE_WASAPI
        swprintf_s(buffer, _countof(buffer), L"Using WASAPI for audio.");
#elif  defined(USE_XAUDIO2)
        swprintf_s(buffer, _countof(buffer), L"Using XAUDIO2 for audio.");
#endif
        m_overlayFont->DrawString(m_spriteBatch.get(), buffer, overlayPos, YellowColor);
        overlayPos.y += yInc;

        swprintf_s(buffer, _countof(buffer), (m_hasAudioStream) ? L"Audio Enabled" : L"Audio Disabled");
        m_overlayFont->DrawString(m_spriteBatch.get(), buffer, overlayPos, YellowColor);
        overlayPos.y += yInc;
    }
    else
    {
        m_overlayFont->DrawString(m_spriteBatch.get(), L"Decoding has not yet started.", overlayPos, YellowColor);
        overlayPos.y += yInc;
    }

    swprintf_s(buffer, _countof(buffer), L"Number of decoded frames received = %d", m_numberOfFramesDecoded);
    m_overlayFont->DrawString(m_spriteBatch.get(), buffer, overlayPos, YellowColor);

    m_spriteBatch->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();

    m_finished = m_videoDone && m_audioDone;
}

//--------------------------------------------------------------------------------------
// Name: GetCurrentRenderTime()
// Desc: calculate current audio clock 
//--------------------------------------------------------------------------------------
int64_t Sample::GetCurrentRenderTime()
{
    uint64_t llMasterClock = 0;
    uint64_t hnscorrelationTime = 0;

    if (m_hasAudioStream)
    {

#ifdef USE_WASAPI

        uint64_t llClockTime = 0;
        uint64_t llQPC = 0;
        uint64_t llFrequence = 1;

        {
            ComPtr<IAudioClock> pAudioClock;

            DX::ThrowIfFailed(m_pAudioClient->GetService(__uuidof(IAudioClock), reinterpret_cast<void**>(pAudioClock.GetAddressOf())));
            DX::ThrowIfFailed(pAudioClock->GetPosition(&llClockTime, &llQPC));
            DX::ThrowIfFailed(pAudioClock->GetFrequency(&llFrequence));

            hnscorrelationTime = GetCurrentTimeInHNS() - llQPC;
            if (hnscorrelationTime > g_100nsInSecs / 2)
            {
                // simple logic to detect if audio clock was stopped or not (due to suspend)
                hnscorrelationTime = 0;
            }
            llMasterClock = llClockTime * g_100nsInSecs / llFrequence + hnscorrelationTime;
        }

#elif defined(USE_XAUDIO2)

        if (m_pAudioReaderOutputWFX)
        {
            //Take a snapshot of the voice context before processing to ensure it does not change between operations
            if (m_VoiceContext.m_llLastBufferStartTime != 0)
            {
                llMasterClock = m_VoiceContext.m_qwRenderedBytes * g_100nsInSecs
                    / (m_pAudioReaderOutputWFX->nSamplesPerSec * m_pAudioReaderOutputWFX->wBitsPerSample * m_pAudioReaderOutputWFX->nChannels / 8);
                llMasterClock += static_cast<uint64_t>(GetCurrentTimeInHNS()) - static_cast<uint64_t>(m_VoiceContext.m_llLastBufferStartTime);
                hnscorrelationTime = static_cast<uint64_t>(GetCurrentTimeInHNS()) - static_cast<uint64_t>(m_VoiceContext.m_llLastBufferStartTime);
                if (hnscorrelationTime > g_100nsInSecs / 2)
                {
                    // simple logic to detect if audio clock was stopped or not ( due to suspend )
                    hnscorrelationTime = 0;
                }
                llMasterClock += hnscorrelationTime;
            }
        }
#endif
        if (m_audioDone)
        {
            llMasterClock += GetCurrentTimeInHNS() - m_llStopHNS;
        }
    }
    else
    {
        llMasterClock = uint64_t(GetCurrentTimeInHNS() - m_llStartHNS);
    }

    return m_llStartTimeStamp + int64_t(llMasterClock);
}

//--------------------------------------------------------------------------------------
// Name: RenderVideoFrame()
// Desc: Helper method used by Render
//--------------------------------------------------------------------------------------
void Sample::RenderVideoFrame(IMFSample* pSample)
{
    auto commandList = m_deviceResources->GetCommandList();
    auto commandQueue = m_deviceResources->GetCommandQueue();
    auto backBuffer = m_deviceResources->GetRenderTarget();

    DX::ThrowIfFailed(m_pVideoRender->RenderDecodedSampleToResource(commandList, commandQueue, pSample, m_videoWidth, m_videoHeight, backBuffer));
}

//--------------------------------------------------------------------------------------
// Name: ProcessVideo()
// Desc: read from video stream
//--------------------------------------------------------------------------------------
void Sample::ProcessVideo()
{
    DWORD streamIndex = MAXDWORD;
    DWORD dwStreamFlags = 0;
    int64_t llTimestamp = 0;
    bool readNewSample = true;
    HRESULT hr = S_OK;

    if (m_pOutputVideoSample && SUCCEEDED(m_pOutputVideoSample->GetSampleTime(&llTimestamp)))
    {
        // if audio is not rendered in this sample, just simply sync the video sample time to system clock. 
        // sample time is in hundred nanoseconds and system clock is in millseconds. 
        if (!m_hasAudioStream && m_llStartTimeStamp == INVALID_SAMPLE_TIME)
        {
            m_llStartTimeStamp = llTimestamp;
            m_llStartHNS = GetCurrentTimeInHNS();
        }

        int64_t currentRenderTime = GetCurrentRenderTime();
        if (llTimestamp > currentRenderTime)
        {
            // the previous sample has not expired, don't read new sample for now. 
            readNewSample = false;
        }
    }

    if (!m_videoDone && readNewSample)
    {
        // Retreive sample from source reader
        ComPtr<IMFSample> pOutputSample;

        hr = m_pReader->ReadSample(
            uint32_t(MF_SOURCE_READER_FIRST_VIDEO_STREAM), // Stream index.
            0,                                             // Flags.
            &streamIndex,                                  // Receives the actual stream index. 
            &dwStreamFlags,                                // Receives status flags.
            &llTimestamp,                                  // Receives the time stamp.
            &pOutputSample                                 // Receives the sample or nullptr.  If this parameter receives a non-NULL pointer, the caller must release the interface.
        );

        if (SUCCEEDED(hr))
        {
            if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                m_videoDone = true;
            }

            if (dwStreamFlags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)
            {
                // The format changed. Reconfigure the decoder.
                ConfigureSourceReaderOutput(m_pReader.Get(), streamIndex);
            }

            if (pOutputSample)
            {
                if (m_videoWidth == 0 || m_videoHeight == 0
                    || (dwStreamFlags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED) || (dwStreamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED))
                {
                    // Update video width and height
                    ComPtr<IMFMediaType> pMediaType;

                    if (SUCCEEDED(m_pReader->GetCurrentMediaType(uint32_t(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &pMediaType)))
                    {
                        MFVideoArea videoArea = {};
                        if (SUCCEEDED(pMediaType->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (uint8_t*)&videoArea, sizeof(MFVideoArea), nullptr)))
                        {
                            m_videoWidth = UINT(videoArea.Area.cx);
                            m_videoHeight = UINT(videoArea.Area.cy);
                        }
                        else
                        {
                            DX::ThrowIfFailed(MFGetAttributeSize(pMediaType.Get(), MF_MT_FRAME_SIZE, &m_videoWidth, &m_videoHeight));
                        }
                    }
                }

                ReleaseCurrentVideoSample();

                if (!m_useSoftwareDecode)
                {
                    // The output buffer may still used by decoding ( although decode returns the buffer from CPU), put a wait single on the GPU to wait to the decoding to complete
                    DX::ThrowIfFailed(MFD3D12GpuWaitForSampleReady(m_pVideoRender->GetVideoProcessCommandQueue(), pOutputSample.Get()));
                }

                m_pOutputVideoSample = pOutputSample;
                ++m_numberOfFramesDecoded;
            }
        }
        else
        {
            // From docs: MF_SOURCE_READERF_ERROR- An error occurred. If you receive this flag, do not make any further calls to IMFSourceReader methods.
            if (dwStreamFlags & MF_SOURCE_READERF_ERROR)
            {
                // If we don't throw here, ReadSample will be called in the next update, and that will lead to undefined behaviour.
                throw std::runtime_error("IMFSourceReader::ReadSample returned E_FAIL and status flag was set to MF_SOURCE_READERF_ERROR.");
            }
        }
    }
}

//--------------------------------------------------------------------------------------
// Name: RenderAudioFrame()
// Desc: Helper method used by Render
//--------------------------------------------------------------------------------------
bool Sample::RenderAudioFrame(IMFSample* pSample)
{

#ifdef USE_XAUDIO2
    // don't queue up more samples if there are already enough ready to be played
    if (m_dwAudioFramesDecoded - m_dwAudioFramesRendered >= MP4R_XA2_MAX_BUFFER_COUNT)
    {
        if (!m_audioStarted)
        {
            HRESULT result = m_pSourceVoice->Start();
            DX::ThrowIfFailed(result);
            m_audioStarted = true;
        }

        return false;
    }

    ComPtr<IMFMediaBuffer> pUncompressedBuf;
    DWORD dwBufferLen;
    DWORD dwMaxBufferLen;

    if (m_llStartTimeStamp == INVALID_SAMPLE_TIME)
    {
        DX::ThrowIfFailed(pSample->GetSampleTime(&m_llStartTimeStamp));
    }

    int64_t timeStamp;
    DX::ThrowIfFailed(pSample->GetSampleTime(&timeStamp));

    DX::ThrowIfFailed(pSample->ConvertToContiguousBuffer(pUncompressedBuf.GetAddressOf()));
    DX::ThrowIfFailed(pUncompressedBuf->GetCurrentLength(&dwBufferLen));

    auto pOutputData = new uint8_t[dwBufferLen];
    uint8_t* pDecompressedData;

    DX::ThrowIfFailed(pUncompressedBuf->Lock(&pDecompressedData, &dwMaxBufferLen, &dwBufferLen));
    memcpy(pOutputData, pDecompressedData, dwBufferLen);

    const size_t size = m_dwAudioFramesDecoded % MP4R_XA2_MAX_BUFFER_COUNT;
    memset(&m_Buffers[size], 0, sizeof(XAUDIO2_BUFFER));
    m_Buffers[size].pAudioData = pOutputData;
    m_Buffers[size].AudioBytes = dwBufferLen;
    m_Buffers[size].pContext = new AudioBufferContext(pOutputData, dwBufferLen);

    // if this is the last audio sample, make sure to enable to END_OF_STREAM flag
    if (m_audioDone)
    {
        m_Buffers[size].Flags |= XAUDIO2_END_OF_STREAM;
    }

    ++m_dwAudioFramesDecoded;

    return true;
#endif  // USE_XAUDIO2

#ifdef USE_WASAPI
    UINT32 numFramesPadding = 0;
    UINT32 numFramesAvailable = 0;
    uint8_t* pOutputData;
    uint8_t* pDecompressedData;

    ComPtr<IMFMediaBuffer> pUncompressedBuf;
    DWORD dwBufferLen;
    DWORD dwMaxBufferLen;
    uint32_t dwUncompressedFrameCount;
    uint32_t frameSizeFromDecoder = uint32_t(m_pAudioReaderOutputWFX->wBitsPerSample) * uint32_t(m_pAudioReaderOutputWFX->nChannels) / 8;
    uint32_t frameSizeFromOutput = uint32_t(m_pAudioClientWFX->wBitsPerSample) * uint32_t(m_pAudioClientWFX->nChannels) / 8;

    if (m_llStartTimeStamp == INVALID_SAMPLE_TIME)
    {
        DX::ThrowIfFailed(pSample->GetSampleTime(&m_llStartTimeStamp));
    }

    DX::ThrowIfFailed(pSample->ConvertToContiguousBuffer(pUncompressedBuf.GetAddressOf()));
    DX::ThrowIfFailed(pUncompressedBuf->GetCurrentLength(&dwBufferLen));
    DX::ThrowIfFailed(m_pAudioClient->GetCurrentPadding(&numFramesPadding));

    dwUncompressedFrameCount = dwBufferLen / frameSizeFromDecoder;
    numFramesAvailable = m_bufferFrameCount - numFramesPadding;

    if (numFramesAvailable < dwUncompressedFrameCount)
    {
        if (!m_audioStarted)
        {
            m_pAudioClient->Start();
            m_audioStarted = true;
        }

        return false;
    }

    // Grab all the available space in the shared buffer.
    DX::ThrowIfFailed(m_pAudioRenderClient->GetBuffer(dwUncompressedFrameCount, &pOutputData));
    // don't have resampler, just render the first few channels from the 7.1 client. 
    DX::ThrowIfFailed(pUncompressedBuf->Lock(&pDecompressedData, &dwMaxBufferLen, &dwBufferLen));
    for (uint32_t f = 0; f < dwUncompressedFrameCount; f++)
    {
        if (frameSizeFromDecoder >= frameSizeFromOutput)
        {
            memcpy(pOutputData, pDecompressedData, frameSizeFromOutput);
        }
        else
        {
            memcpy(pOutputData, pDecompressedData, frameSizeFromDecoder);

            // put other channels to 0
            memset(pOutputData + frameSizeFromDecoder, 0, frameSizeFromOutput - frameSizeFromDecoder);
        }
        pOutputData += frameSizeFromOutput;
        pDecompressedData += frameSizeFromDecoder;
    }
    pUncompressedBuf->Unlock();
    DX::ThrowIfFailed(m_pAudioRenderClient->ReleaseBuffer(dwUncompressedFrameCount, 0));

    return true;
#endif  // USE_WASAPI
}

//--------------------------------------------------------------------------------------
// Name: ProcessAudio()
// Desc: read and render from audio stream
//--------------------------------------------------------------------------------------
void Sample::ProcessAudio()
{
    DWORD streamIndex = MAXDWORD;
    DWORD dwStreamFlags = 0;
    int64_t llTimestamp = 0;
    bool readNewSample = true;
    HRESULT hr = S_OK;

    if (!m_hasAudioStream)
    {
        m_audioDone = true;
        return;
    }

    if (m_pOutputAudioSample)
    {
        if (RenderAudioFrame(m_pOutputAudioSample.Get()))
        {
            m_pOutputAudioSample.Reset();
        }
        else
        {
            //  audio buffer is full, wait for next update
            return;
        }
    }

    while (!m_audioDone && readNewSample)
    {
        // Retreive sample from source reader
        ComPtr<IMFSample> pOutputSample;

        hr = m_pReader->ReadSample(
            uint32_t(MF_SOURCE_READER_FIRST_AUDIO_STREAM), // Stream index.
            0,                                             // Flags.
            &streamIndex,                                  // Receives the actual stream index. 
            &dwStreamFlags,                                // Receives status flags.
            &llTimestamp,                                  // Receives the time stamp.
            &pOutputSample                                 // Receives the sample or nullptr.  If this parameter receives a non-NULL pointer, the caller must release the interface.
        );

        if (SUCCEEDED(hr))
        {
            if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                m_audioDone = true;
                m_llStopHNS = GetCurrentTimeInHNS();
            }

            if (dwStreamFlags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)
            {
                // The format changed. Reconfigure the decoder.
                ConfigureSourceReaderOutput(m_pReader.Get(), streamIndex);
            }

            if (pOutputSample)
            {
                readNewSample = RenderAudioFrame(pOutputSample.Get());

                if (readNewSample)
                {
                    pOutputSample.Reset();
                }
                else
                {
                    m_pOutputAudioSample = pOutputSample;
                }
                ++m_numberOfFramesDecoded;
            }
        }
        else
        {
            readNewSample = false;
            if (MF_E_END_OF_STREAM == hr)
            {
                m_audioDone = true;
            }
        }
    }
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

#ifdef USE_XAUDIO2
    // Signal the audio thread to suspend
    ResetEvent(m_audioResumeSignal.Get());
    m_suspendAudioThread = true;

    // Suspend audio engine
    m_pXAudio2->StopEngine();
#endif

}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();

#ifdef USE_XAUDIO2
    // Signal the audio thread to resume
    m_suspendAudioThread = false;
    SetEvent(m_audioResumeSignal.Get());

    // Resume audio engine
    m_pXAudio2->StartEngine();
#endif
}

#pragma endregion

#pragma region Direct3D Resources

// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create descriptor heap
    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::NumDescriptors);

    const RenderTargetState rtState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
    SpriteBatchPipelineStateDescription pd(rtState);
    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    auto index = static_cast<size_t>(ResourceDescriptors::OverlayFont);
    m_overlayFont = std::make_unique<SpriteFont>(device, resourceUpload, L"SegoeUI_18.spritefont", m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));

    index = static_cast<size_t>(ResourceDescriptors::ControllerFont);
    m_controllerFont = std::make_unique<SpriteFont>(device, resourceUpload, L"XboxOneControllerSmall.spritefont", m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());   
    uploadResourcesFinished.wait();     // Wait for resources to upload  

    // Render all UI at 1080p so that it's easy to swtich between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_spriteBatch->SetViewport(viewportUI);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
}

#pragma endregion

// Destructor 
Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    CleanupAudio();

    m_pReader.Reset();

    if (m_pOutputVideoSample)
    {
        if (m_useSoftwareDecode)
        {
            MFD3D12GpuSignalSampleFree(m_pVideoRender->GetVideoProcessCommandQueue(), m_pOutputVideoSample.Get());
        }

        m_pOutputVideoSample.Reset();
    }

    m_pOutputAudioSample.Reset();

    // Shut down Media Foundation.
    MFShutdown();
}

void Sample::CleanupAudio()
{
#ifdef USE_WASAPI

    if (m_pAudioClient)
    {
        m_pAudioClient->Stop();
    }

    m_pAudioClient.Reset();
    m_pAudioRenderClient.Reset();

    if (m_pAudioClientWFX)
    {
        CoTaskMemFree(m_pAudioClientWFX);
        m_pAudioClientWFX = nullptr;
    }

#elif defined(USE_XAUDIO2)

    // Force the Audio thread to exit
    if (m_audioThread)
    {
        m_terminateAudioThread = true;
        m_audioThread->join();
        m_audioThread = nullptr;
    }

    if (m_pSourceVoice)
    {
        DX::ThrowIfFailed(m_pSourceVoice->Stop());
        DX::ThrowIfFailed(m_pSourceVoice->FlushSourceBuffers());
        m_pSourceVoice->DestroyVoice();
    }

    if (m_pMasteringVoice)
    {
        m_pMasteringVoice->DestroyVoice();
    }

    if (m_pXAudio2)
    {
        m_pXAudio2.Reset();
    }

    m_VoiceContext.Reset();
    m_dwAudioFramesDecoded = 0;
    m_dwAudioFramesRendered = 0;

#endif

    m_llStartTimeStamp = INVALID_SAMPLE_TIME;
    m_llStartHNS = 0;
    m_llStopHNS = 0;
    m_audioStarted = false;
}

void Sample::Seek(QWORD hnsPosition)
{
    m_videoDone = false;
    m_audioDone = false;

    ReleaseCurrentVideoSample();

    m_pOutputAudioSample.Reset();

    InitializeAudio(true);

    PROPVARIANT position;
    position.vt = VT_I8;
    position.hVal.QuadPart = static_cast<LONGLONG>(hnsPosition);
    m_pReader->SetCurrentPosition(GUID_NULL, position);
}

void Sample::ResetAndCreatePlayer()
{
    CleanupAudio();
    
    m_pOutputAudioSample.Reset();
    ReleaseCurrentVideoSample();

    // Variables to reset back to default values
    m_videoDone = false;
    m_audioDone = false;
    m_videoWidth = 0;
    m_videoHeight = 0;

    // Having this here means this case explicitly counts from zero on every loop,
    // since it resets the player. In the seek scenario we continue to count decoded frames.
    m_numberOfFramesDecoded = 0;

#ifdef USE_WASAPI

    m_bufferFrameCount = 0;

#endif

    if (m_hasAudioStream)
    {
        if (m_pAudioReaderOutputWFX)
        {
            CoTaskMemFree(m_pAudioReaderOutputWFX);
            m_pAudioReaderOutputWFX = nullptr;
        }
    }

    m_pVideoRender.release();

#ifdef USE_XAUDIO2

    m_suspendAudioThread = false;

#endif

    // After reseting variables, re-initialize video and audio
    InitializeMedia();
}
