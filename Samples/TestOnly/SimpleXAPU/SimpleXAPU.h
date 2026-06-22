//--------------------------------------------------------------------------------------
// SimpleXAPU.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

//#define WRITETOFILE

#include "DeviceResources.h"
#include "StepTimer.h"
#include "mf_x/xapu.h"

#ifdef WRITETOFILE
constexpr BYTE WaveHeaderMonoFloat48K[] = {
    0x52, 0x49, 0x46, 0x46, 0x3C, 0xC8, 0xAF, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x80, 0xBB, 0x00, 0x00, 0x00, 0xEE, 0x02, 0x00,
    0x04, 0x00, 0x20, 0x00, 0x66, 0x61, 0x63, 0x74, 0x04, 0x00, 0x00, 0x00, 0xFD, 0xF1, 0x2B, 0x00,
    0x50, 0x45, 0x41, 0x4B, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0xA3, 0x32, 0x5C,
    0xAC, 0x16, 0x7F, 0x3E, 0x63, 0x5C, 0x01, 0x00, 0x64, 0x61, 0x74, 0x61, 0xFF, 0xFF, 0xFF, 0xFF };
#endif

const uint32_t MAX_BUFFER_COUNT = 3;
const float MAX_PITCH = 8.f;

class DecodeOneSimpleMemoryManager
{
public:
    static constexpr uint32_t MaxStreamCount = MAX_BUFFER_COUNT;
    static constexpr uint32_t MaxInputBufferLength = 512;
    static constexpr uint32_t MaxOutputBufferLength = 3840; // 20 msec * 48K * sizeof(float)
    static constexpr uint32_t DecodedFrameCount = 960; // 20 msec opus packet - this value should work for both 10 and 20 msec input streams
    static constexpr uint32_t OutputFrameCount = 512;
    static constexpr uint32_t MaxProcessingBufferLength =
        (2 * DecodedFrameCount * sizeof(float)) + MaxOutputBufferLength; // this value cover pitch ratio from 1/8 to 8/1

    uint32_t currentStream = 0;

private:
    XApuConnectInputParameters _memory = {
        XApuProcessType::DecodeConvertOpus,
        MaxStreamCount,
        1,
        MaxStreamCount * (MaxInputBufferLength + MaxOutputBufferLength + MaxProcessingBufferLength),
        OutputFrameCount,
        XApuConnectOptions::None };

    XApuConnectOutputParameters _outParam = {};

public:
    const XApuConnectInputParameters* GetConnectParametersRef() const
    {
        return &_memory;
    }

    const XApuConnectOutputParameters* GetConnectionDataRef() const
    {
        return &_outParam;
    }

    uint8_t* GetRawInputBuffer(uint32_t bufferId, uint32_t& maxByteCount)
    {
        maxByteCount = MaxInputBufferLength;
        return reinterpret_cast<uint8_t*>(_outParam.baseData) + (bufferId * MaxInputBufferLength);
    }

    uint8_t* GetRawOutputBuffer(uint32_t bufferId, uint32_t& maxByteCount)
    {
        maxByteCount = MaxOutputBufferLength;
        return reinterpret_cast<uint8_t*>(_outParam.baseData) + (bufferId * MaxOutputBufferLength) + (MaxInputBufferLength * MaxStreamCount);
    }

    uint8_t* GetProcessingBuffer(uint32_t& maxByteCount) {
        maxByteCount = MaxProcessingBufferLength;
        return reinterpret_cast<uint8_t*>(_outParam.baseData) + (MaxStreamCount * MaxInputBufferLength) + (MaxStreamCount * MaxOutputBufferLength);
    }
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
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

private:
    void StartPlayback();
    void StopPlayback();

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    static DWORD WINAPI ReadFileThread(LPVOID lpParam);
    static DWORD WINAPI SubmitAudioBufferThread(LPVOID lpParam);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    bool                                        m_keyDown;

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

    XApuHandle                                  m_xapuHandle;

    FILE*                                       m_opusFile;
#ifdef WRITETOFILE
    FILE*                                       m_outFile;
#endif

    DecodeOneSimpleMemoryManager                m_memoryManager;

    long                                        m_waveSize;
    uint32_t                                    m_currentPosition;

    std::atomic<uint32_t>                       m_NumberOfBuffersProduced;
    std::atomic<uint32_t>                       m_NumberOfBuffersConsumed;

    PlaySoundStreamVoiceContext					m_VoiceContext;
    HANDLE                                      m_producerThread;
    HANDLE                                      m_consumerThread;
    bool				                        m_isPlaying;
    std::atomic<bool>                           m_terminateThread;
    bool                                        m_loop;
    float                                       m_pitch;
};
