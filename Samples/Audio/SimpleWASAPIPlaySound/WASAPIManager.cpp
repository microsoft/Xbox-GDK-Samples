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
#include "WASAPIManager.h"

namespace
{
    VOID CALLBACK RenderWorkCallback(_Inout_ PTP_CALLBACK_INSTANCE, _Inout_opt_ PVOID Context, _Inout_ PTP_WORK)
    {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        auto Manager = reinterpret_cast<WASAPIManager*>(Context);

        while (Manager->m_threadActive)
        {
            // Wait for next buffer event to be signaled.
            DWORD retval = WaitForSingleObject(Manager->m_SampleReadyEvent, 2000);
            if (retval != WAIT_OBJECT_0)
            {
                // Event handle timed out after a 2-second wait.
                // This wait will timeout across a suspend/resume cycle if the timeout is not INFINITE.
                Manager->m_threadActive = false;
                Manager->StopDevice();
                Manager->m_state = DeviceState::DeviceStateInError;
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
                Manager->m_threadActive = false;
                Manager->StopDevice();
                Manager->m_state = DeviceState::DeviceStateInError;
            }
        }
    }
}

WASAPIManager::WASAPIManager() noexcept(false) :
    m_threadActive(false),
    m_SampleReadyEvent(nullptr),
    m_state(DeviceState::DeviceStateUnInitialized),
    m_BufferFrames(0),
    m_MixFormat(nullptr),
    m_DeviceProps{},
    m_ToneSource(nullptr),
    m_CritSec{},
    m_playingSound(false),
    m_renderWorkThread(nullptr)
{
    // Create events for sample ready or user stop
    m_SampleReadyEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (nullptr == m_SampleReadyEvent)
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    InitializeCriticalSection(&m_CritSec);

    InitializeDevice();
}

WASAPIManager::~WASAPIManager()
{
    if (INVALID_HANDLE_VALUE != m_SampleReadyEvent)
    {
        CloseHandle(m_SampleReadyEvent);
        m_SampleReadyEvent = INVALID_HANDLE_VALUE;
    }

    DeleteCriticalSection(&m_CritSec);
}

//--------------------------------------------------------------------------------------
//  Name: InitializeDevice
//  Desc: Sets up a new instance of the WASAPI renderer
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::InitializeDevice()
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    Microsoft::WRL::ComPtr<IMMDevice> device;

    // Configure user based properties
    m_DeviceProps = {};
    m_DeviceProps.IsBackground = false;
    m_DeviceProps.hnsBufferDuration = static_cast<REFERENCE_TIME>(0);
    m_DeviceProps.Frequency = 440;

    // Create a device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Please note that any invalidation error such as AUDCLNT_E_DEVICE_INVALIDATED or AUDCLNT_E_RESOURCES_INVALIDATED during initialization
    // should be treated as something that can be retried.

    // Get the default renderer
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Activate the endpoint
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_AudioClient);
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

    // Initialize the AudioClient in Shared Mode with the user specified buffer
    hr = m_AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
        m_DeviceProps.hnsBufferDuration,
        m_DeviceProps.hnsBufferDuration,
        m_MixFormat,
        nullptr);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Get the maximum size of the AudioClient Buffer
    hr = m_AudioClient->GetBufferSize(&m_BufferFrames);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Get the render client
    hr = m_AudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_AudioRenderClient);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Sets the event handle that the system signals when an audio buffer is ready to be processed by the client
    hr = m_AudioClient->SetEventHandle(m_SampleReadyEvent);
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
        // Setup either ToneGeneration or File Playback
        REFERENCE_TIME defaultDevicePeriod = 0;
        REFERENCE_TIME minimumDevicePeriod = 0;

        // Get the audio device period
        hr = m_AudioClient->GetDevicePeriod(&defaultDevicePeriod, &minimumDevicePeriod);
        if (FAILED(hr))
        {
            m_state = DeviceState::DeviceStateInError;
            return hr;
        }

        double devicePeriodInSeconds = defaultDevicePeriod / (double)REFTIMES_PER_SEC;

        UINT32 FramesPerPeriod = static_cast<UINT32>(m_MixFormat->nSamplesPerSec * devicePeriodInSeconds + 0.5);

        // Generate the sine wave sample buffer
        m_ToneSource = new ToneSampleGenerator();
        hr = m_ToneSource->GenerateSampleBuffer(m_DeviceProps.Frequency, FramesPerPeriod, m_MixFormat);
        if (FAILED(hr))
        {
            return hr;
        }

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
    HRESULT hr = AUDCLNT_E_DEVICE_INVALIDATED;

    while (AUDCLNT_E_UNSUPPORTED_FORMAT == hr ||
            AUDCLNT_E_RESOURCES_INVALIDATED == hr ||
            AUDCLNT_E_DEVICE_INVALIDATED == hr ||
            AUDCLNT_E_ENDPOINT_CREATE_FAILED == hr)
    {
        // Any of these errors can occur when a title becomes unconstrained and the renderer is not ready
        // Once the IMMDevice is obtained, it could possibly be invalidated at any time
        // Retry initialization until it is successful or hits a different error

        // Only re-initialize the device if necessary
        if (m_state != DeviceState::DeviceStateInitialized &&
            m_state != DeviceState::DeviceStatePaused)
        {
            hr = InitializeDevice();
            if (SUCCEEDED(hr))
            {
                hr = StartDevice();
            }
        }
        else
        {
            hr = StartDevice();
        }

        // Give the device time to finish processing an invalidation
        if (FAILED(hr))
        {
            Sleep(50);
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
                hr = m_AudioRenderClient->GetBuffer(framesAvailable, &pData);

                if (SUCCEEDED(hr))
                {
                    hr = m_AudioRenderClient->ReleaseBuffer(framesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
                }
            }
            else
            {
                if (m_state == DeviceState::DeviceStatePlaying)
                {
                    // Fill the buffer with a playback sample
                    hr = GetToneSample(framesAvailable);
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
//  Desc:   Fills buffer with a tone sample
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::GetToneSample(UINT32 FramesAvailable)
{
    HRESULT hr = S_OK;
    BYTE *Data;

    // Post-Roll Silence
    if (m_ToneSource->IsEOF())
    {
        hr = m_AudioRenderClient->GetBuffer(FramesAvailable, &Data);
        if (SUCCEEDED(hr))
        {
            // Ignore the return
            hr = m_AudioRenderClient->ReleaseBuffer(FramesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
        }

        StopDevice();
    }
    else if (m_ToneSource->GetBufferLength() <= (FramesAvailable * m_MixFormat->nBlockAlign))
    {
        UINT32 ActualFramesToRead = m_ToneSource->GetBufferLength() / m_MixFormat->nBlockAlign;
        UINT32 ActualBytesToRead = ActualFramesToRead * m_MixFormat->nBlockAlign;

        hr = m_AudioRenderClient->GetBuffer(ActualFramesToRead, &Data);
        if (SUCCEEDED(hr))
        {
            hr = m_ToneSource->FillSampleBuffer(ActualBytesToRead, Data);
            if (SUCCEEDED(hr))
            {
                hr = m_AudioRenderClient->ReleaseBuffer(ActualFramesToRead, 0);
            }
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name:   StopDevice
//  Desc:   Stop playback, if WASAPI renderer exists
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::StopDevice()
{
    if (m_state != DeviceState::DeviceStateUnInitialized &&
        m_state != DeviceState::DeviceStateInError)
    {
        if (m_threadActive)
        {
            m_threadActive = false;
            m_playingSound = false;
        }

        // Flush anything left in buffer with silence
        OnAudioSampleRequested(true);

        m_state = DeviceState::DeviceStateStopped;

        // Flush remaining buffers
        m_ToneSource->Flush();

        return m_AudioClient->Stop();
    }

    return E_FAIL;
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

    return E_FAIL;
}

//--------------------------------------------------------------------------------------
//  Name: Restart
//  Desc: Stops, cleans, then starts playback
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::Restart()
{
    StopDevice();
    m_state = DeviceState::DeviceStateUnInitialized;
    m_AudioClient = nullptr;
    m_AudioRenderClient = nullptr;

    HRESULT hr = Play();

    // StopDevice marks the thread as inactive, so mark
    // it as active here on success so sound continues playing
    if (SUCCEEDED(hr))
    {
        m_threadActive = true;
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
            HRESULT hr = Play();

            // Create a new render thread if the previous one went away due to an error
            // or this is the first time calling play
            if (SUCCEEDED(hr) && !m_threadActive)
            {
                if (m_renderWorkThread)
                {
                    // Close the previous work thread if it exited due to error
                    WaitForThreadpoolWorkCallbacks(m_renderWorkThread, FALSE);
                    CloseThreadpoolWork(m_renderWorkThread);
                    m_renderWorkThread = nullptr;
                }

                m_threadActive = true;
                m_playingSound = true;
                m_renderWorkThread = CreateThreadpoolWork(RenderWorkCallback, this, nullptr);
                SubmitThreadpoolWork(m_renderWorkThread);
            }

            return hr;
        }
    }

    return E_FAIL;
}
