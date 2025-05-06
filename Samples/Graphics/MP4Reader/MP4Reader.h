//--------------------------------------------------------------------------------------
// MP4Reader.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

//
// Use one of these two definitions to see how the two different technologies perform.
//
//#define USE_XAUDIO2
#define USE_WASAPI

#include "DeviceResources.h"
#include "StepTimer.h"

#if defined(USE_WASAPI)
#include "Audioclient.h"
#elif defined(USE_XAUDIO2)
#include <xaudio2.h>
#endif // USE_XAUDIO2

#ifndef INVALID_SAMPLE_TIME
#define INVALID_SAMPLE_TIME 0x7fffffffffffffffui64
#endif

#if defined(USE_XAUDIO2)

// XAudio2-specific helpers required for class definition
constexpr uint32_t MP4R_XA2_MAX_BUFFER_COUNT = 3;

int64_t GetCurrentTimeInHNS();

struct AudioBufferContext
{
    AudioBufferContext(uint8_t* pData, uint32_t dwAudioBytes)
        :m_pData(pData)
        , m_dwAudioBytes(dwAudioBytes)
    {
    }

    std::unique_ptr<BYTE[]> m_pData;
    uint32_t m_dwAudioBytes;
};

//--------------------------------------------------------------------------------------
// Name: struct PlaySoundStreamVoiceContext
// Desc: Frees up the audio buffer after processing
//--------------------------------------------------------------------------------------
struct PlaySoundStreamVoiceContext : public IXAudio2VoiceCallback
{
    STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) override {}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}
    STDMETHOD_(void, OnStreamEnd)() override {}

    STDMETHOD_(void, OnBufferStart)(void*) override
    {
        m_llLastBufferStartTime = GetCurrentTimeInHNS();
    }

    STDMETHOD_(void, OnBufferEnd)(void* pBufferContext) override
    {
        SetEvent(m_hBufferEndEvent);

        // Free up the memory chunk holding the PCM data that was read from disk earlier.
        // In a game you would probably return this memory to a pool.
        if (pBufferContext)
        {
            auto pAudioBufferContext = reinterpret_cast<AudioBufferContext*>(pBufferContext);
            m_qwRenderedBytes += pAudioBufferContext->m_dwAudioBytes;
            delete pAudioBufferContext;
        }

        m_llLastBufferStartTime = GetCurrentTimeInHNS();
    }

    STDMETHOD_(void, OnLoopEnd)(void*) override {}
    STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override {}

    PlaySoundStreamVoiceContext() noexcept(false)
        : m_hBufferEndEvent(0)
        , m_qwRenderedBytes(0)
        , m_llLastBufferStartTime(INVALID_SAMPLE_TIME)
    {
        m_hBufferEndEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (!m_hBufferEndEvent)
            throw std::exception("CreateEventEx");
    }

    PlaySoundStreamVoiceContext(PlaySoundStreamVoiceContext&&) = default;
    PlaySoundStreamVoiceContext& operator= (PlaySoundStreamVoiceContext&&) = default;

    PlaySoundStreamVoiceContext(PlaySoundStreamVoiceContext&) = delete;
    PlaySoundStreamVoiceContext& operator= (PlaySoundStreamVoiceContext&) = delete;

    virtual ~PlaySoundStreamVoiceContext()
    {
        if (m_hBufferEndEvent)
        {
            CloseHandle(m_hBufferEndEvent);
            m_hBufferEndEvent = nullptr;
        }
    }

    void Reset()
    {
        m_qwRenderedBytes = 0;
        m_llLastBufferStartTime = INVALID_SAMPLE_TIME;
    }

    HANDLE m_hBufferEndEvent;
    uint64_t m_qwRenderedBytes;
    int64_t m_llLastBufferStartTime;
};

#endif // USE_XAUDIO2

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void InitializeMedia();
    void InitializeAudio(bool reinit = false);
    void CleanupAudio();
    void ReleaseCurrentVideoSample();
    void ConfigureSourceReaderOutput(IMFSourceReader* pReader, uint32_t dwStreamIndex);
    void RenderVideoFrame(IMFSample* pSample);
    bool RenderAudioFrame(IMFSample* pSample);
    void ProcessVideo();
    void ProcessAudio();
    int64_t GetCurrentRenderTime();
    void Seek(QWORD hnsPosition);
    void ResetAndCreatePlayer();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::SpriteFont>        m_overlayFont;
    std::unique_ptr<DirectX::SpriteFont>        m_controllerFont;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // object for MF interaction
    std::atomic<bool>                           m_videoDone;
    std::atomic<bool>                           m_audioDone;

    Microsoft::WRL::ComPtr<IMFSample>           m_pOutputVideoSample;

    uint32_t                                    m_numberOfFramesDecoded;

    uint32_t                                    m_videoWidth;
    uint32_t                                    m_videoHeight;

    Microsoft::WRL::ComPtr<IMFSample>           m_pOutputAudioSample;

#ifdef USE_WASAPI

    Microsoft::WRL::ComPtr<IAudioClient>        m_pAudioClient;
    Microsoft::WRL::ComPtr<IAudioRenderClient>  m_pAudioRenderClient;
    WAVEFORMATEX*                               m_pAudioClientWFX;

#endif  // USE_WASAPI

    uint32_t                                    m_bufferFrameCount;
    WAVEFORMATEX*                               m_pAudioReaderOutputWFX;

    Microsoft::WRL::ComPtr<IMFSourceReader>     m_pReader;
    std::unique_ptr<CXboxNV12ToRGBConverter>    m_pVideoRender;

#ifdef USE_XAUDIO2
    
    Microsoft::WRL::ComPtr<IXAudio2>            m_pXAudio2;
    IXAudio2MasteringVoice*                     m_pMasteringVoice;
    IXAudio2SourceVoice*                        m_pSourceVoice;
    PlaySoundStreamVoiceContext                 m_VoiceContext;
    uint32_t                                    m_dwCurrentPosition;
    uint32_t                                    m_dwAudioFramesDecoded;
    uint32_t                                    m_dwAudioFramesRendered;
    XAUDIO2_BUFFER                              m_Buffers[MP4R_XA2_MAX_BUFFER_COUNT];

    std::atomic<bool>                           m_terminateAudioThread;
    std::atomic<bool>                           m_suspendAudioThread;
    std::thread*                                m_audioThread;
    Microsoft::WRL::Wrappers::Event             m_audioResumeSignal;
    void SubmitAudioBufferThreadProc();

#endif  // USE_XAUDIO2

    bool                                        m_hasAudioStream;
    bool                                        m_audioStarted;
    bool                                        m_finished;
    bool                                        m_sampleLoops;
    bool                                        m_useSoftwareDecode;
    bool                                        m_seekLoop;
    int64_t                                     m_llStartTimeStamp;
    int64_t                                     m_llStartHNS;
    int64_t                                     m_llStopHNS;

    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorHeap;

    enum ResourceDescriptors
    {
        VideoTexture_Y,
        VideoTexture_UV,
        OverlayFont,
        ControllerFont,
        NumDescriptors
    };
};
