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
    //Thread to manage render sample requests
    VOID CALLBACK RenderWorkCallback(_Inout_ PTP_CALLBACK_INSTANCE, _Inout_opt_ PVOID Context, _Inout_ PTP_WORK)
    {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        auto Manager = reinterpret_cast<WASAPIManager*>(Context);

        while (Manager->m_renderThreadActive)
        {
            // Wait for next buffer event to be signaled.
            DWORD retval = WaitForSingleObject(Manager->m_renderSampleReadyEvent, 2000);
            if (retval != WAIT_OBJECT_0)
            {
                // Event handle timed out after a 2-second wait.
                Manager->m_renderThreadActive = false;
                Manager->StopRenderDevice();
                Manager->m_state = DeviceState::DeviceStateInError;
            }

            HRESULT hr = Manager->OnAudioRenderSampleRequested(false);

            // If anything says that the resources have been invalidated then reinitialize
            // the device.
            if (AUDCLNT_E_RESOURCES_INVALIDATED == hr || AUDCLNT_E_DEVICE_INVALIDATED == hr)
            {
                // Only recover if the stream was currently playing, otherwise we will recover
                // when the client is started again
                if (Manager->m_state == DeviceState::DeviceStatePlaying)
                {
                    hr = Manager->RestartPlayback();
                }
            }

            if (FAILED(hr))
            {
                Manager->m_renderThreadActive = false;
                Manager->StopRenderDevice();
                Manager->m_state = DeviceState::DeviceStateInError;
            }
        }
    }
}

namespace
{
    //Thread to manage capture sample requests
    VOID CALLBACK CaptureWorkCallback(_Inout_ PTP_CALLBACK_INSTANCE, _Inout_opt_ PVOID Context, _Inout_ PTP_WORK)
    {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        auto Manager = reinterpret_cast<WASAPIManager*>(Context);

        while (Manager->m_captureThreadActive)
        {
            // Wait for next buffer event to be signaled.
            DWORD retval = WaitForSingleObject(Manager->m_captureSampleReadyEvent, 2000);
            if (retval != WAIT_OBJECT_0)
            {
                // Event handle timed out after a 2-second wait.
                Manager->StopCaptureDevice();
            }

            if (FAILED(Manager->OnAudioCaptureSampleRequested()))
            {
                Manager->m_state = DeviceState::DeviceStateInError;
                Manager->m_captureThreadActive = false;
            }
        }
    }
}

WASAPIManager::WASAPIManager() noexcept(false) :
    m_renderThreadActive(false),
    m_captureThreadActive(false),
    m_renderSampleReadyEvent(nullptr),
    m_captureSampleReadyEvent(nullptr),
    m_state(DeviceState::DeviceStateUnInitialized),
    m_playingSound(false),
    m_recordingSound(false),
    m_RenderBufferFrames(0),
    m_CaptureBufferFrames(0),
    m_CaptureWfx(nullptr),
    m_RenderWfx(nullptr),
    m_captureIndex(0),
    m_CritSec{},
    m_renderWorkThread(nullptr),
    m_captureWorkThread(nullptr),
    m_WaveSource(nullptr),
    m_pWaveFile(nullptr)
{
    // Create events for sample ready or user stop
    m_renderSampleReadyEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (nullptr == m_renderSampleReadyEvent)
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    m_captureSampleReadyEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (nullptr == m_captureSampleReadyEvent)
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    InitializeCriticalSection(&m_CritSec);

    if (SUCCEEDED(InitializeRenderDevice()))
    {
        InitializeCaptureDevice(0);
    }
}


WASAPIManager::~WASAPIManager()
{
    if (INVALID_HANDLE_VALUE != m_renderSampleReadyEvent)
    {
        CloseHandle(m_renderSampleReadyEvent);
        m_renderSampleReadyEvent = INVALID_HANDLE_VALUE;
    }

    if (INVALID_HANDLE_VALUE != m_captureSampleReadyEvent)
    {
        CloseHandle(m_captureSampleReadyEvent);
        m_captureSampleReadyEvent = INVALID_HANDLE_VALUE;
    }

    DeleteCriticalSection( &m_CritSec );
}

//--------------------------------------------------------------------------------------
//  Name: InitializeRenderDevice
//  Desc: Sets up a new instance of the WASAPI renderer and creates WASAPI session
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::InitializeRenderDevice()
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    Microsoft::WRL::ComPtr<IMMDevice> device;


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
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_AudioClientForRender);
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

    hr = m_AudioClientForRender->SetClientProperties(&audioProps);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Get the endpoint format
    hr = m_AudioClientForRender->GetMixFormat(&m_RenderWfx);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // For this sample we will force stereo output (default is 7.1)
    m_RenderWfx->nChannels = 2;
    m_RenderWfx->nBlockAlign = 8;
    m_RenderWfx->nAvgBytesPerSec = 384000;

    hr = m_AudioClientForRender->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
        (REFTIMES_PER_SEC / 1000) * 20,
        0,
        m_RenderWfx,
        &AUDIOSESSIONGUID);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Get the maximum size of the AudioClient Buffer
    hr = m_AudioClientForRender->GetBufferSize(&m_RenderBufferFrames);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Get the render client
    hr = m_AudioClientForRender->GetService(__uuidof(IAudioRenderClient), (void**)&m_AudioRenderClient);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Sets the event handle that the system signals when an audio buffer is ready to be processed by the client
    hr = m_AudioClientForRender->SetEventHandle(m_renderSampleReadyEvent);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    m_state = DeviceState::DeviceStateInitialized;

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name: InitializeCaptureDevice
//  Desc: Sets up a new instance of the WASAPI capture
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::InitializeCaptureDevice(UINT32 id)
{
    HRESULT hr = S_OK;

    m_captureIndex = id;

    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDeviceCollection* deviceCollectionInterface;
    IMMDevice* device = nullptr;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
        (void**)&enumerator);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    hr = enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATEMASK_ALL, &deviceCollectionInterface);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    //Set the capture device based on the index
    hr = deviceCollectionInterface->Item(m_captureIndex, &device);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    hr = device->Activate(
        __uuidof(IAudioClient2), CLSCTX_ALL,
        nullptr, (void**)&m_AudioClientForCapture);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Hardware offload isn't supported
    AudioClientProperties audioProps = {};
    audioProps.cbSize = sizeof(AudioClientProperties);
    audioProps.bIsOffload = false;
    audioProps.eCategory = AudioCategory_GameChat;

    hr = m_AudioClientForCapture->SetClientProperties(&audioProps);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    //Get the default format of the device
    hr = m_AudioClientForCapture->GetMixFormat(&m_CaptureWfx);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Initialize the AudioClient in Shared Mode with the user specified buffer
    hr = m_AudioClientForCapture->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
        REFTIMES_PER_SEC,
        0,
        m_CaptureWfx,
        &AUDIOSESSIONGUID);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Get the maximum size of the AudioClient Buffer
    hr = m_AudioClientForCapture->GetBufferSize(&m_CaptureBufferFrames);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Get the capture client
    hr = m_AudioClientForCapture->GetService(__uuidof(IAudioCaptureClient), (void**)m_AudioCaptureClient.GetAddressOf());
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }

    // Sets the event handle that the system signals when an audio buffer is ready to be processed by the client
    hr = m_AudioClientForCapture->SetEventHandle(m_captureSampleReadyEvent);
    if (FAILED(hr))
    {
        m_state = DeviceState::DeviceStateInError;
        return hr;
    }
        
    //Register the device manager
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> deviceEnum;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnum);
    deviceEnum->RegisterEndpointNotificationCallback(&m_deviceManager);

    m_deviceManager.UpdateCaptureDeviceList();

    m_state = DeviceState::DeviceStateInitialized;

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name:   StopRenderDevice
//  Desc:   Stop playback, if WASAPI renderer exists
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::StopRenderDevice()
{
    if (m_state != DeviceState::DeviceStateUnInitialized &&
        m_state != DeviceState::DeviceStateInError)
    {
        if (m_renderThreadActive)
        {
            m_renderThreadActive = false;
            m_playingSound = false;
        }

        // Flush anything left in buffer with silence
        OnAudioRenderSampleRequested(true);

        m_state = DeviceState::DeviceStateStopped;

        return m_AudioClientForRender->Stop();
    }

    return E_FAIL;
}

//--------------------------------------------------------------------------------------
//  Name:   StopCaptureDevice
//  Desc:   Stop playback, if WASAPI renderer exists
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::StopCaptureDevice()
{
    if (m_state != DeviceState::DeviceStateUnInitialized &&
        m_state != DeviceState::DeviceStateInError)
    {
        if (m_captureThreadActive)
        {
            m_captureThreadActive = false;
            m_recordingSound = false;
            m_playingSound = false;
            WaitForThreadpoolWorkCallbacks(m_captureWorkThread, FALSE);
            CloseThreadpoolWork(m_captureWorkThread);
        }

        m_state = DeviceState::DeviceStateStopped;

        return m_AudioClientForCapture->Stop();
    }

    return E_FAIL;
}

//--------------------------------------------------------------------------------------
//  Name:   RecordToggle
//  Desc:   Toggles the state of recording
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::RecordToggle()
{
    HRESULT hr = S_OK;
    
    if (m_AudioCaptureClient)
    {
        if (m_state == DeviceState::DeviceStateCapturing)
        {
            return StopCapture();
        }
        else if (m_state == DeviceState::DeviceStatePlaying)
        {
            hr = StopPlayback();

            if (FAILED(hr))
            {
                return hr;
            }

            hr = StartCapture();
        }
        else if (m_state == DeviceState::DeviceStateStopped ||
                m_state == DeviceState::DeviceStateInitialized)
        {
            hr = StartCapture();
        }
    }
    
    return hr;
}

//--------------------------------------------------------------------------------------
//  Name:   SetCaptureDevice
//  Desc:   Switches the capture device
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::SetCaptureDevice(UINT32 index)
{
    if(index != m_captureIndex)
    {
        if (m_state == DeviceState::DeviceStatePlaying)
        {
            StopPlayback();
        }
        else if (m_state == DeviceState::DeviceStateCapturing)
        {
            StopCapture();
        }

        m_deviceManager.SetCaptureId(static_cast<int>(index));
        return InitializeCaptureDevice(index);
    }

    return E_FAIL;
}

//--------------------------------------------------------------------------------------
//  Name: PlayPauseToggle
//  Desc: Toggle pause state
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::PlayToggle()
{
    if (m_state != DeviceState::DeviceStateUnInitialized)
    {
        HRESULT hr;

        if (m_state == DeviceState::DeviceStateCapturing)
        {
            //Stop recording
            hr = RecordToggle();
            if (FAILED(hr))
            {
                return hr;
            }
        }
        
        if (m_state == DeviceState::DeviceStatePlaying)
        {
            //Starts a work item to pause playback
            return StopPlayback();
        }
        else
        {
            //Load recording
            ConfigureSource(g_FileName);

            // Initialize the device and play
            hr = StartPlayback();

            // Create a new render thread if the previous one went away due to an error
            // or this is the first time calling play
            if (SUCCEEDED(hr) && !m_renderThreadActive)
            {
                if (m_renderWorkThread)
                {
                    // Close the previous work thread if it exited due to error
                    WaitForThreadpoolWorkCallbacks(m_renderWorkThread, FALSE);
                    CloseThreadpoolWork(m_renderWorkThread);
                    m_renderWorkThread = nullptr;
                }

                m_renderThreadActive = true;
                m_playingSound = true;
                m_renderWorkThread = CreateThreadpoolWork(RenderWorkCallback, this, nullptr);
                SubmitThreadpoolWork(m_renderWorkThread);
            }

            return hr;
        }
    }

    return E_FAIL;
}

//--------------------------------------------------------------------------------------
//  Name: StartDevice
//  Desc: Pre-rolls (if necessary) and starts playback on the render device
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::StartRenderDevice()
{
    HRESULT hr = S_OK;

    if (m_state == DeviceState::DeviceStateStopped ||
        m_state == DeviceState::DeviceStateInitialized)
    {
        // Setup either ToneGeneration or File Playback
        REFERENCE_TIME defaultDevicePeriod = 0;
        REFERENCE_TIME minimumDevicePeriod = 0;

        // Get the audio device period
        hr = m_AudioClientForRender->GetDevicePeriod(&defaultDevicePeriod, &minimumDevicePeriod);
        if (FAILED(hr))
        {
            m_state = DeviceState::DeviceStateInError;
            return hr;
        }

        // Pre-Roll the buffer with silence
        hr = OnAudioRenderSampleRequested(true);
        if (FAILED(hr))
        {
            m_state = DeviceState::DeviceStateInError;
            return hr;
        }

        m_state = DeviceState::DeviceStatePlaying;

        // Actually start the playback
        hr = m_AudioClientForRender->Start();
    }
    else if (m_state == DeviceState::DeviceStatePaused)
    {
        // Pre-Roll the buffer with silence
        hr = OnAudioRenderSampleRequested(true);
        if (FAILED(hr))
        {
            m_state = DeviceState::DeviceStateInError;
            return hr;
        }

        m_playingSound = true;
        m_state = DeviceState::DeviceStatePlaying;

        // Actually start the playback
        hr = m_AudioClientForRender->Start();
    }

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name: StartPlayback
//  Desc: Initialize and start playback
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::StartPlayback()
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
            hr = InitializeRenderDevice();
            if (SUCCEEDED(hr))
            {
                hr = StartRenderDevice();
            }
        }
        else
        {
            hr = StartRenderDevice();
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
//  Name: StopPlayback
//  Desc: If device is playing, stop playback. Otherwise do nothing.
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::StopPlayback()
{
    if (m_state == DeviceState::DeviceStatePlaying)
    {
        m_state = DeviceState::DeviceStateStopped;

        m_renderThreadActive = false;
        m_playingSound = false;
        m_WaveSource->Flush();
        return m_AudioClientForRender->Stop();
    }

    return E_FAIL;
}

//--------------------------------------------------------------------------------------
//  Name: RestartPlayback
//  Desc: Stops, cleans, then starts playback
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::RestartPlayback()
{
    StopRenderDevice();
    m_state = DeviceState::DeviceStateUnInitialized;
    m_AudioClientForRender = nullptr;
    m_AudioRenderClient = nullptr;

    HRESULT hr = StartPlayback();

    // StopDevice marks the thread as inactive, so mark
    // it as active here on success so sound continues playing
    if (SUCCEEDED(hr))
    {
        m_renderThreadActive = true;
        m_playingSound = true;
    }

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name:   StartCapture
//  Desc:   Method to start capture called from the high priority thread
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::StartCapture()
{
    HRESULT hr = S_OK;

    //Create the WAV file
    m_pWaveFile = new CWaveFileWriter();
    m_pWaveFile->Open(g_FileName, m_CaptureWfx);

    // Actually start recording
    hr = m_AudioClientForCapture->Start();

    if (SUCCEEDED(hr))
    {
        m_captureThreadActive = true;
        m_recordingSound = true;
        m_captureWorkThread = CreateThreadpoolWork(CaptureWorkCallback, this, nullptr);
        SubmitThreadpoolWork(m_captureWorkThread);
        m_state = DeviceState::DeviceStateCapturing;
    }
    else
    {
        m_state = DeviceState::DeviceStateInError;
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
//  Name:   StopCapture
//  Desc:   Method to stop capture called from the high priority thread
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::StopCapture()
{
    if (m_state != DeviceState::DeviceStateCapturing)
    {
        // Flush anything left in buffer
        OnAudioCaptureSampleRequested();
    }

    m_AudioClientForCapture->Stop();
    m_recordingSound = false;

    m_pWaveFile->Close();

    m_state = DeviceState::DeviceStateStopped;
    return S_OK;
}

//--------------------------------------------------------------------------------------
//  Name:   OnAudioRenderSampleRequested
//  Desc:   Called when audio device fires m_SampleReadyEvent
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::OnAudioRenderSampleRequested(bool bIsSilence)
{
    HRESULT hr = S_OK;
    UINT32  paddingFrames = 0;
    UINT32  framesAvailable = 0;

    // Prevent multiple concurrent submissions of samples
    EnterCriticalSection(&m_CritSec);

    // Get padding in existing buffer
    hr = m_AudioClientForRender->GetCurrentPadding(&paddingFrames);

    if (SUCCEEDED(hr))
    {
        // In non-HW shared mode, GetCurrentPadding represents the number of queued frames
        // so we can subtract that from the overall number of frames we have
        framesAvailable = m_RenderBufferFrames - paddingFrames;

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
                    hr = GetWaveSample(framesAvailable);
                }
            }
        }
    }

    LeaveCriticalSection(&m_CritSec);

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name:   OnAudioCaptureSampleRequested
//  Desc:   Called when audio device fires m_SampleReadyEvent
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::OnAudioCaptureSampleRequested()
{
    HRESULT hr = S_OK;
    BYTE *captureData;
    DWORD flags;
    UINT32 numFramesAvailable;

    // Prevent multiple concurrent submissions of samples
    EnterCriticalSection(&m_CritSec);

    hr = m_AudioCaptureClient->GetBuffer(&captureData, &numFramesAvailable, &flags, nullptr, nullptr);
    if (SUCCEEDED(hr))
    {
        //Write to WAV file
        m_pWaveFile->WriteSample(captureData, numFramesAvailable * m_CaptureWfx->nBlockAlign * m_CaptureWfx->nChannels, nullptr);
        hr = m_AudioCaptureClient->ReleaseBuffer(numFramesAvailable);
    }

    LeaveCriticalSection(&m_CritSec);

    return hr;
}


//--------------------------------------------------------------------------------------
//  Name:   GetWaveSample
//  Desc:   Fills buffer with a wave sample
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::GetWaveSample(UINT32 FramesAvailable)
{
    HRESULT hr = S_OK;
    BYTE *Data;
    UINT32 bufferLength = m_WaveSource->GetBufferLength();

    // Post-Roll Silence
    if (m_WaveSource->IsEOF())
    {
        hr = m_AudioRenderClient->GetBuffer(FramesAvailable, &Data);
        if (SUCCEEDED(hr))
        {
            // Ignore the return
            hr = m_AudioRenderClient->ReleaseBuffer(FramesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
        }

        StopPlayback();
    }
    else if (bufferLength <= (FramesAvailable * m_RenderWfx->nBlockAlign))
    {
        UINT32 ActualFramesToRead = bufferLength / m_RenderWfx->nBlockAlign;
        UINT32 ActualBytesToRead = ActualFramesToRead * m_RenderWfx->nBlockAlign;

        hr = m_AudioRenderClient->GetBuffer(ActualFramesToRead, &Data);
        if (SUCCEEDED(hr))
        {
            hr = m_WaveSource->FillSampleBuffer(ActualBytesToRead, Data);
            if (SUCCEEDED(hr))
            {
                hr = m_AudioRenderClient->ReleaseBuffer(ActualFramesToRead, 0);
            }
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name:   ConfigureSource
//  Desc:   Configures WAVE playback
//--------------------------------------------------------------------------------------
HRESULT WASAPIManager::ConfigureSource(const wchar_t* filename)
{
    HRESULT hr = S_OK;

    REFERENCE_TIME defaultDevicePeriod = 0;
    REFERENCE_TIME minimumDevicePeriod = 0;

    // Get the audio device period
    hr = m_AudioClientForRender->GetDevicePeriod(&defaultDevicePeriod, &minimumDevicePeriod);
    if (FAILED(hr))
    {
        return hr;
    }

    double devicePeriodInSeconds = defaultDevicePeriod / (double)REFTIMES_PER_SEC;

    UINT32 FramesPerPeriod = static_cast<UINT32>(m_RenderWfx->nSamplesPerSec * devicePeriodInSeconds + 0.5);
    
    WAVEFORMATEXTENSIBLE wfx;

    ZeroMemory(&wfx, sizeof(WAVEFORMATEXTENSIBLE));

    // Read the wave file
    DX::WAVData waveData;
    DX::ThrowIfFailed(DX::LoadWAVAudioFromFileEx(filename, m_waveFile, waveData));

    // Read the format header
    memcpy(&wfx, waveData.wfx, sizeof(WAVEFORMATEXTENSIBLE));

    m_WaveSource = new WaveSampleGenerator();
    hr = m_WaveSource->GenerateSampleBuffer(&m_waveFile[0], waveData.audioBytes, &wfx, FramesPerPeriod, m_RenderWfx);

    return hr;
}
