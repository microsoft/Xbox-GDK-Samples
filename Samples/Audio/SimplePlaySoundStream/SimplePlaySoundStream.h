//--------------------------------------------------------------------------------------
// SimplePlaySoundStream.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "WAVStreamer.h"

//--------------------------------------------------------------------------------------
// Name: struct PlaySoundStreamVoiceContext
// Desc: Frees up the audio buffer after processing
//--------------------------------------------------------------------------------------
struct PlaySoundStreamVoiceContext : public IXAudio2VoiceCallback
{
    STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) override {}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}
    STDMETHOD_(void, OnStreamEnd)() override {}
    STDMETHOD_(void, OnBufferStart)(void*) override {}

    STDMETHOD_(void, OnBufferEnd)(void* pBufferContext) override
    {
        SetEvent(m_hBufferEndEvent);
        //
        // Free up the memory chunk holding the PCM data that was read from disk earlier.
        // In a game you would probably return this memory to a pool.
        //
        auto pBuffer = static_cast<uint8_t*>(pBufferContext);
        delete[] pBuffer;
    }

    STDMETHOD_(void, OnLoopEnd)(void*) override {}
    STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override {}

    HANDLE m_hBufferEndEvent;

    PlaySoundStreamVoiceContext() noexcept(false) : m_hBufferEndEvent(nullptr)
    {
        m_hBufferEndEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (!m_hBufferEndEvent)
        {
            throw std::exception("CreateEvent");
        }
    }
    virtual ~PlaySoundStreamVoiceContext()
    {
        if (m_hBufferEndEvent)
        {
            CloseHandle(m_hBufferEndEvent);
            m_hBufferEndEvent = nullptr;
        }
    }
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public IXAudio2EngineCallback
{
public:

    Sample() noexcept(false);
    virtual ~Sample();

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

private:

    static const uint32_t STREAMING_BUFFER_SIZE = 65536;
    static const size_t MAX_BUFFER_COUNT = 3;

    void InitializeXAudio();

    STDMETHOD_(void, OnProcessingPassStart) () override {}
    STDMETHOD_(void, OnProcessingPassEnd)() override {}
    STDMETHOD_(void, OnCriticalError) (THIS_ HRESULT)
    {
        //When the renderer is invalidated, restart
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

        m_CritErrorOccurred = true;
    }


    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    static DWORD WINAPI ReadFileThread(LPVOID lpParam);
    static DWORD WINAPI SubmitAudioBufferThread(LPVOID lpParam);

    HRESULT LoadPCMFile(const wchar_t* szFilename);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum Descriptors
    {
        TextFont,
        Background,
        Count,
    };

    // Audio objects.
    Microsoft::WRL::ComPtr<IXAudio2>            m_pXAudio2;
    IXAudio2MasteringVoice*                     m_pMasteringVoice;
    IXAudio2SourceVoice*                        m_pSourceVoice;

    bool				                        m_DoneSubmitting;
    PlaySoundStreamVoiceContext					m_VoiceContext;
    WaveFile								    m_WaveFile;
    uint32_t                                    m_waveSize;
    uint32_t                                    m_currentPosition;
    XAUDIO2_BUFFER						        m_Buffers[MAX_BUFFER_COUNT];
    size_t                                      m_NumberOfBuffersProduced;
    size_t                                      m_NumberOfBuffersConsumed;
    HANDLE                                      m_producerThread;
    HANDLE                                      m_consumerThread;
    std::atomic<bool>                           m_terminateThread;
    std::atomic<bool>                           m_CritErrorOccurred;
};
