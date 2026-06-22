//--------------------------------------------------------------------------------------
// SimpleXDSPFFT.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "WAVFileReader.h"

#include "mf_x\xdspaudio.h"

constexpr int MAX_BUFFER_COUNT = 3;
constexpr int EQ_BUCKET_DEPTH = 5;
constexpr int EQ_BUCKET_COUNT = 10;
constexpr int EQ_MAX_FREQ = 20000;
const uint32_t EQBuckets[EQ_BUCKET_COUNT] = { 30, 60, 120, 240, 500, 1000, 2000, 4000, 8000, 16000 };
static const wchar_t* EQBucketsStrings[EQ_BUCKET_COUNT] = { L"30", L"60", L"120", L"240", L"500", L"1K", L"2K", L"4K", L"8K", L"16K" };

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

    STDMETHOD_(void, OnBufferEnd)(void*) override
    {
        SetEvent(m_hBufferEndEvent);
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

struct BufferManager
{
    float* inBuffer[MAX_BUFFER_COUNT];
    float* outBuffer[MAX_BUFFER_COUNT];
    uint32_t bufferId[MAX_BUFFER_COUNT];
    int firstAvailable;
    std::mutex lock;

    BufferManager() :
        inBuffer{},
        outBuffer{},
        firstAvailable(-1)
    {
        for (int i = 0; i < MAX_BUFFER_COUNT; i++)
        {
            bufferId[i] = 0;
        }
    }

    void SetBuffers(float** inBuffers, float** outBuffers)
    {
        lock.lock();
        memcpy(inBuffer, inBuffers, MAX_BUFFER_COUNT * sizeof(float*));
        memcpy(outBuffer, outBuffers, MAX_BUFFER_COUNT * sizeof(float*));
        firstAvailable = 0;
        lock.unlock();
    }

    bool AcquireBuffers(uint32_t id, float** input, float** output)
    {
        if (firstAvailable == -1)
        {
            return false;
        }
        
        lock.lock();
        *input = inBuffer[firstAvailable];
        *output = outBuffer[firstAvailable];
        bufferId[firstAvailable] = id;
        ResetAvailable();
        lock.unlock();

        return true;
    }

    bool GetBuffers(uint32_t id, float** input, float** output)
    {
        bool found = false;
        lock.lock();
        for (int i = 0; i < MAX_BUFFER_COUNT; i++)
        {
            if (bufferId[i] == id)
            {
                *input = inBuffer[i];
                *output = outBuffer[i];
                found = true;
                break;
            }
        }
        lock.unlock();

        return found;
    }

    void ReleaseBuffer(uint32_t id)
    {
        lock.lock();
        for (int i = 0; i < MAX_BUFFER_COUNT; i++)
        {
            if (bufferId[i] == id)
            {
                bufferId[i] = 0;
                break;
            }
        }

        ResetAvailable();
        lock.unlock();
    }

    void ReleaseOldestBuffer()
    {
        uint32_t lowestId = bufferId[0];
        int found = 0;

        lock.lock();
        for (int i = 1; i < MAX_BUFFER_COUNT; i++)
        {
            if (bufferId[i] < lowestId)
            {
                lowestId = bufferId[i];
                found = i;
            }
        }

        bufferId[found] = 0;


        ResetAvailable();
        lock.unlock();
    }

    void ResetAvailable()
    {
        firstAvailable = -1;
        for (int i = 0; i < MAX_BUFFER_COUNT; i++)
        {
            if (bufferId[i] == 0)
            {
                firstAvailable = i;
                break;
            }
        }
    }
};

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

    static constexpr uint32_t MAX_BLOCK_FRAME_COUNT = 1024;
    static constexpr uint32_t MAX_BLOCK_FRAME_SIZE = MAX_BLOCK_FRAME_COUNT * sizeof(float);

    void XM_CALLCONV RenderEQ(ID3D12GraphicsCommandList* commandList);

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    static DWORD WINAPI SendConvCommandThread(LPVOID lpParam);
    static DWORD WINAPI SubmitAudioBufferThread(LPVOID lpParam);

    void InitializeXAudio();
    STDMETHOD_(void, OnProcessingPassStart) () override {}
    STDMETHOD_(void, OnProcessingPassEnd)() override {}
    STDMETHOD_(void, OnCriticalError) (THIS_ HRESULT)
    {
        //When the renderer is invalidated, restart
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

        m_critErrorOccurred = true;
    }

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::BasicEffect>       m_batchEffect;

    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;


    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    HANDLE                                      m_producerThread;
    HANDLE                                      m_consumerThread;
    std::atomic<bool>                           m_critErrorOccurred;
    std::atomic<bool>                           m_terminateThread;
    XDspClientHandle                            m_dspClientHandle;
    XDspStreamHandle                            m_dspStreamHandle;
    XDspStatus*                                 m_dspStatus;
    float*                                      m_dspBuffer;

    uint32_t                                    m_lastBufferedCommandSequence;
    uint32_t                                    m_lastRetrievedCommandSequence;
    uint32_t                                    m_lastFinishedCommandSequence;
    std::atomic<float>                          m_EQ[EQ_BUCKET_DEPTH][EQ_BUCKET_COUNT];
    uint32_t                                    m_currentEQIndex;
    bool                                        m_showLines;
    bool                                        m_keyDown;

    Microsoft::WRL::ComPtr<IXAudio2>            m_pXAudio2;
    IXAudio2MasteringVoice*                     m_pMasteringVoice;
    IXAudio2SourceVoice*                        m_pSourceVoice;
    PlaySoundStreamVoiceContext					m_voiceContext;

    std::unique_ptr<uint8_t[]>                  m_waveFile;
    DX::WAVData                                 m_waveData;

    BufferManager                               m_DSPBuffers;
    uint32_t                                    m_currentPosition;
    uint32_t                                    m_currentId;

    enum Descriptors
    {
        TextFont,
        Background,
        Count,
    };
};
