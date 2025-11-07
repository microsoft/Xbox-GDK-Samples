//--------------------------------------------------------------------------------------
// WASAPIManager.cpp
//
// Wraps the underlying WASAPIRenderer in a simple WinRT class that can receive
// the DeviceStateChanged events.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include <devicetopology.h>

#include "WASAPIManager.h"
#include "WAVFileReader.h"

namespace
{
    // mapping for 2 channel (FL/FR) and 4 channel (FL/FR/BL/BR) devices
    const uint32_t ChannelMasks[] = { SPEAKER_FRONT_LEFT, SPEAKER_FRONT_RIGHT, SPEAKER_BACK_LEFT, SPEAKER_BACK_RIGHT };

    DWORD RenderWorkCallback(_In_ LPVOID Context)
    {
        DX::ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
        auto Manager = reinterpret_cast<WASAPIManager*>(Context);

        while (Manager && Manager->m_playingSound)
        {
            // Wait for next buffer event to be signaled.
            DWORD retval = WaitForSingleObject(Manager->m_sampleReadyEvent, 2000);
            if (retval != WAIT_OBJECT_0)
            {
                // we hit a timeout, bail out of this thread becuase we're probably shutting down
                return 0;
            }

            HRESULT hr = Manager->OnAudioSampleRequested(false);

            //	If anything says that the resources have been invalidated then reinitialize
            //	the device.
            if (AUDCLNT_E_RESOURCES_INVALIDATED == hr || AUDCLNT_E_DEVICE_INVALIDATED == hr)
            {
                // Only recover if the stream was currently playing, otherwise we will recover
                // when the client is started again
                if (Manager->m_state == DeviceState::DeviceStatePlaying)
                {
                    hr = Manager->Restart();
                }
            }

            if (FAILED(hr))
            {
                Manager->m_playingSound = false;
                Manager->Stop();
                Manager->m_state = DeviceState::DeviceStateInError;
            }
        }

        return 0;
    }
}

WASAPIManager::WASAPIManager() noexcept(false) :
    m_playingSound(false),
    m_sampleReadyEvent(nullptr),
    m_state(DeviceState::DeviceStateUnInitialized),
    m_device(nullptr),
    m_locationCount(0),
    m_locations(nullptr),
    m_BufferFrames(0),
    m_MixFormat(nullptr),
    m_waveGenerator(nullptr),
    m_CritSec{},
    m_renderWorkThread(nullptr)
{
    // Create events for sample ready or user stop
    m_sampleReadyEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (nullptr == m_sampleReadyEvent)
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    InitializeCriticalSection(&m_CritSec);
}

WASAPIManager::~WASAPIManager()
{
    if (INVALID_HANDLE_VALUE != m_sampleReadyEvent)
    {
        CloseHandle(m_sampleReadyEvent);
        m_sampleReadyEvent = INVALID_HANDLE_VALUE;
    }

    DeleteCriticalSection(&m_CritSec);

    if(m_locations)
    {
        delete[] m_locations;
        m_locations = nullptr;
    }
}

//--------------------------------------------------------------------------------------
//  Name: InitializeDevice
//  Desc: Sets up a new instance of the WASAPI renderer for the specified device
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::InitializeDevice(wchar_t* endpoint, uint32_t locationCount, GUID* locations)
{
    HRESULT hr = S_OK;

    if(!endpoint)
    {
        return E_INVALIDARG;
    }

    m_locationCount = locationCount;

    if(m_locations)
    {
        delete[] m_locations;
    }

    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    enumerator->GetDevice(endpoint, &m_device);

    m_locations = new GUID[locationCount];
    memcpy(m_locations, locations, locationCount * sizeof(GUID));

    // Activate the endpoint
    hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_AudioClient);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Every method using IMMDevice after the client is activated is also retryable on invalidation errors such as
    // AUDCLNT_E_DEVICE_INVALIDATED, AUDCLNT_E_RESOURCES_INVALIDATED, AUDCLNT_E_ENDPOINT_CREATE_FAILED, and AUDCLNT_E_UNSUPPORTED_FORMAT.
 
    AudioClientProperties audioProps = {};
    audioProps.cbSize = sizeof(AudioClientProperties);
    audioProps.bIsOffload = false;
    audioProps.eCategory = AudioCategory_ForegroundOnlyMedia;

    hr = m_AudioClient->SetClientProperties(&audioProps);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Get the endpoint format
    hr = m_AudioClient->GetMixFormat(&m_MixFormat);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    WORD channels = 0;
    DWORD channelMask = 0;

    for(uint32_t i = 0; i < locationCount; i++)
    {
        if(locations[i] != GAMEINPUT_HAPTIC_LOCATION_NONE)
        {
            channels++;
            channelMask |= ChannelMasks[i];
        }
    }

    // modify the definition of the output channels based on haptics locations
    WAVEFORMATEXTENSIBLE* wfx = (WAVEFORMATEXTENSIBLE *)m_MixFormat;
    wfx->dwChannelMask = channelMask;
    wfx->Format.nChannels = channels;
    wfx->Format.nBlockAlign = static_cast<WORD>((wfx->Format.nChannels * wfx->Format.wBitsPerSample)/8);
    wfx->Format.nAvgBytesPerSec = wfx->Format.nSamplesPerSec * wfx->Format.nBlockAlign;

    // Initialize the AudioClient in Shared Mode with the user specified buffer
    hr = m_AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
        static_cast<REFERENCE_TIME>(0),
        static_cast<REFERENCE_TIME>(0),
        reinterpret_cast<WAVEFORMATEX*>(wfx),
        nullptr);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    REFERENCE_TIME defaultDevicePeriod = 0;
    REFERENCE_TIME minimumDevicePeriod = 0;

    // Get the audio device period
    hr = m_AudioClient->GetDevicePeriod(&defaultDevicePeriod, &minimumDevicePeriod);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    m_framesPerPeriod = static_cast<UINT32>(m_MixFormat->nSamplesPerSec * ((double)defaultDevicePeriod / (double)REFTIMES_PER_SEC) + 0.5);

    // Get the maximum size of the AudioClient Buffer
    hr = m_AudioClient->GetBufferSize(&m_BufferFrames);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Get the render client
    hr = m_AudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_audioRenderClient);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Sets the event handle that the system signals when an audio buffer is ready to be processed by the client
    hr = m_AudioClient->SetEventHandle(m_sampleReadyEvent);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    m_state = DeviceState::DeviceStateInitialized;

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name: StartDevice
//  Desc: Pre-rolls (if necessary) and starts playback on the render device
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::StartDevice()
{
    HRESULT hr = S_OK;

    if (m_state == DeviceState::DeviceStateStopped ||
        m_state == DeviceState::DeviceStateInitialized)
    {
        // Pre-Roll the buffer with silence
        hr = OnAudioSampleRequested(true);
        if (FAILED(hr))
        {
            m_state = DeviceState::DeviceStateInError;
            return hr;
        }

        m_state = DeviceState::DeviceStatePlaying;

        // Actually start the playback
        hr = m_AudioClient->Start();
    }
    else if (m_state == DeviceState::DeviceStatePaused)
    {
        // Pre-Roll the buffer with silence
        hr = OnAudioSampleRequested(true);
        if (FAILED(hr))
        {
            m_state = DeviceState::DeviceStateInError;
            return hr;
        }

        m_playingSound = true;
        m_state = DeviceState::DeviceStatePlaying;

        // Actually start the playback
        hr = m_AudioClient->Start();
    }

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name: Play
//  Desc: Initialize and start playback
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::Play()
{
    // we are already playing or we don't have a sample generator, bail out
    if(m_state == DeviceState::DeviceStatePlaying)
    {
        return S_OK;
    }

    if(m_currentGenerator == nullptr)
    {
        return E_FAIL;
    }

    HRESULT hr = AUDCLNT_E_DEVICE_INVALIDATED;

    while (AUDCLNT_E_UNSUPPORTED_FORMAT == hr ||
            AUDCLNT_E_RESOURCES_INVALIDATED == hr ||
            AUDCLNT_E_DEVICE_INVALIDATED == hr ||
            AUDCLNT_E_ENDPOINT_CREATE_FAILED == hr)
    {
        // Any of these errors can occur when a title becomes unconstrained and the renderer is not ready
        // Once the IMMDevice is obtained, it could possibly be invalidated at any time
        // Retry initialization until it is successful or hits a different error

        hr = StartDevice();

        // Give the device time to finish processing an invalidation
        if (FAILED(hr))
        {
            Sleep(50);
        }

        // Create a new render thread if the previous one went away due to an error
        // or this is the first time calling play
        if (SUCCEEDED(hr) && !m_playingSound)
        {
            if (m_renderWorkThread)
            {
                m_renderWorkThread = nullptr;
            }

            m_playingSound = true;
            m_renderWorkThread = CreateThread(nullptr, 0, RenderWorkCallback, this, 0, nullptr);
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name:   OnAudioSampleRequested
//  Desc:   Called when audio device fires m_SampleReadyEvent
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::OnAudioSampleRequested(bool bIsSilence)
{
    HRESULT hr = S_OK;
    UINT32  paddingFrames = 0;
    UINT32  framesAvailable = 0;

    if(!m_AudioClient)
    {
        return E_FAIL;
    }

    // Prevent multiple concurrent submissions of samples
    EnterCriticalSection(&m_CritSec);

    // Get padding in existing buffer
    hr = m_AudioClient->GetCurrentPadding(&paddingFrames);

    if (SUCCEEDED(hr))
    {
        // In non-HW shared mode, GetCurrentPadding represents the number of queued frames
        // so we can subtract that from the overall number of frames we have
        framesAvailable = m_BufferFrames - paddingFrames;

        // Only continue if we have buffer to write data
        if (framesAvailable > 0)
        {
            if (bIsSilence)
            {
                BYTE *pData;

                // Fill the buffer with silence
                hr = m_audioRenderClient->GetBuffer(framesAvailable, &pData);

                if (SUCCEEDED(hr))
                {
                    hr = m_audioRenderClient->ReleaseBuffer(framesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
                }
            }
            else
            {
                if (m_state == DeviceState::DeviceStatePlaying)
                {
                    // Fill the buffer with a playback sample
                    hr = GetSample(framesAvailable);
                }
            }
        }
    }

    LeaveCriticalSection(&m_CritSec);

    // Device state will be set by the caller
    return hr;
}

//--------------------------------------------------------------------------------------
//  Name:   GetToneSample
//  Desc:   Fills buffer with a sample
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::GetSample(UINT32 FramesAvailable)
{
    HRESULT hr = S_OK;
    BYTE *Data;
    UINT32 bufferLength = m_currentGenerator->GetBufferLength();

    // Post-Roll Silence
    if (m_currentGenerator->IsEOF())
    {
        hr = m_audioRenderClient->GetBuffer(FramesAvailable, &Data);
        if (SUCCEEDED(hr))
        {
            // Ignore the return
            hr = m_audioRenderClient->ReleaseBuffer(FramesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
        }

        Stop();
    }
    else if (bufferLength <= (FramesAvailable * m_MixFormat->nBlockAlign))
    {
        UINT32 ActualFramesToRead = bufferLength / m_MixFormat->nBlockAlign;
        UINT32 ActualBytesToRead = ActualFramesToRead * m_MixFormat->nBlockAlign;

        hr = m_audioRenderClient->GetBuffer(ActualFramesToRead, &Data);
        if (SUCCEEDED(hr))
        {
            hr = m_currentGenerator->FillSampleBuffer(ActualBytesToRead, Data);
            if (SUCCEEDED(hr))
            {
                hr = m_audioRenderClient->ReleaseBuffer(ActualFramesToRead, 0);
            }
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name:   StopDevice
//  Desc:   Stop playback, if WASAPI renderer exists
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::Stop()
{
    if (m_state != DeviceState::DeviceStateUnInitialized &&
        m_state != DeviceState::DeviceStateInError)
    {
        if(m_playingSound)
        {
            m_playingSound = false;
        }

        // Flush anything left in buffer with silence
        OnAudioSampleRequested(true);

        m_state = DeviceState::DeviceStateStopped;

        // Flush remaining buffers
        if(m_currentGenerator)
        {
            m_currentGenerator->Flush();
        }

        return m_AudioClient->Stop();
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
//  Name: Pause
//  Desc: If device is playing, pause playback. Otherwise do nothing.
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::Pause()
{
    if (m_state == DeviceState::DeviceStatePlaying)
    {
        m_state = DeviceState::DeviceStatePaused;
        m_playingSound = false;
        return m_AudioClient->Stop();
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
//  Name: Restart
//  Desc: Stops, cleans, then starts playback
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::Restart()
{
    Stop();
    m_state = DeviceState::DeviceStateUnInitialized;
    m_AudioClient = nullptr;
    m_audioRenderClient = nullptr;

    HRESULT hr = Play();

    // StopDevice marks the thread as inactive, so mark
    // it as active here on success so sound continues playing
    if (SUCCEEDED(hr))
    {
        m_playingSound = true;
    }

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name: PlayPauseToggle
//  Desc: Toggle pause state
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::PlayPauseToggle()
{
    if (m_state != DeviceState::DeviceStateUnInitialized)
    {
        //  We only permit a pause state change if we're fully playing
        //  or fully paused.
        if (m_state == DeviceState::DeviceStatePlaying)
        {
            // Starts a work item to pause playback
            return Pause();
        }
        else
        {
            // Initialize the device and play
            return Play();
        }
    }

    return E_FAIL;
}

//--------------------------------------------------------------------------------------
//  Name:   ConfigureWaveSource
//  Desc:   Configures WAVE playback
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::ConfigureWaveSource(const wchar_t* filename)
{
    WAVEFORMATEXTENSIBLE wfx;

    ZeroMemory(&wfx, sizeof(WAVEFORMATEXTENSIBLE));

    // Read the wave file
    DX::WAVData waveData;
    HRESULT hr = DX::LoadWAVAudioFromFileEx(filename, m_waveFileData, waveData);
    if (FAILED(hr))
        return hr;

    // Read the format header
    memcpy(&wfx, waveData.wfx, sizeof(WAVEFORMATEXTENSIBLE));

    m_waveGenerator = std::make_unique<WaveSampleGenerator>();
    hr = m_waveGenerator->GenerateSampleBuffer(&m_waveFileData[0], waveData.audioBytes, &wfx, m_framesPerPeriod, m_MixFormat);
    if (FAILED(hr))
    {
        return hr;
    }

    m_currentGenerator = m_waveGenerator;
    return hr;
}
