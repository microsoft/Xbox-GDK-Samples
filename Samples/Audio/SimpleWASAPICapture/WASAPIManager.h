//--------------------------------------------------------------------------------------
// WASAPIManager.h
//
// Wraps the underlying WASAPIRenderer in a simple WinRT class that can receive
// the DeviceStateChanged events.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "DeviceManager.h"
#include "WaveFileWriter.h"
#include "WaveSampleGenerator.h"
#include "WAVFileReader.h"

static const LPCWSTR g_FileName = L"Recording.wav";

enum class DeviceState
{
    DeviceStateUnInitialized,
    DeviceStateInError,
    DeviceStateDiscontinuity,
    DeviceStateFlushing,
    DeviceStateActivated,
    DeviceStateInitialized,
    DeviceStateStarting,
    DeviceStatePlaying,
    DeviceStateCapturing,
    DeviceStatePaused,
    DeviceStateStopped
};

constexpr REFERENCE_TIME REFTIMES_PER_SEC = 10000000LL;

class WASAPIManager
{
public:
    WASAPIManager() noexcept(false);
    ~WASAPIManager();

    WASAPIManager(WASAPIManager&&) = default;
    WASAPIManager& operator= (WASAPIManager&&) = default;

    WASAPIManager(WASAPIManager const&) = delete;
    WASAPIManager& operator= (WASAPIManager const&) = delete;

    HRESULT PlayToggle();
    HRESULT RecordToggle();
    HRESULT SetCaptureDevice(UINT32 index);
    HRESULT OnAudioRenderSampleRequested(bool bIsSilence);
    HRESULT OnAudioCaptureSampleRequested();
    HRESULT StartRenderDevice();
    HRESULT StopRenderDevice();
    HRESULT StopCaptureDevice();
    HRESULT InitializeRenderDevice();
    HRESULT StartPlayback();
    HRESULT StopPlayback();
    HRESULT RestartPlayback();

    bool m_renderThreadActive;
    bool m_captureThreadActive;

    HANDLE						m_renderSampleReadyEvent;
    HANDLE						m_captureSampleReadyEvent;
    DeviceState					m_state;

    bool						m_playingSound;
    bool						m_recordingSound;

    //--------------------------------------------------------------------------------------
    //  Name: SetDeviceChangeCallback
    //  Desc: Sets the callback when capture devices change
    //--------------------------------------------------------------------------------------
    void SetDeviceChangeCallback(void(*inFunc)(int))
    {
        m_deviceManager.SetDeviceListReport(inFunc);
    }

    void GetCaptureDevices(std::vector<LPWSTR> &device)
    {
        m_deviceManager.GetCaptureDevices(device);
    }


private:
    HRESULT InitializeCaptureDevice(UINT32 id);

    HRESULT StartCapture();
    HRESULT StopCapture();

    HRESULT ConfigureSource(const wchar_t* filename);
    HRESULT GetWaveSample(UINT32 FramesAvailable);

    UINT32										m_RenderBufferFrames;
    UINT32										m_CaptureBufferFrames;
    WAVEFORMATEX*								m_CaptureWfx;
    WAVEFORMATEX*								m_RenderWfx;
    std::vector<BYTE>							m_WaveData;

    Microsoft::WRL::ComPtr<IAudioClient2>       m_AudioClientForRender;
    Microsoft::WRL::ComPtr<IAudioClient2>       m_AudioClientForCapture;
    Microsoft::WRL::ComPtr<IAudioRenderClient>	m_AudioRenderClient;
    Microsoft::WRL::ComPtr<IAudioCaptureClient>	m_AudioCaptureClient;

    UINT										m_captureIndex;

    DeviceManager								m_deviceManager;

    CRITICAL_SECTION							m_CritSec;

    //worker thread for renderer
    PTP_WORK									m_renderWorkThread;

    //worker thread for capture
    PTP_WORK									m_captureWorkThread;
    
    WaveSampleGenerator*						m_WaveSource;
    std::unique_ptr<uint8_t[]>					m_waveFile;
    CWaveFileWriter*							m_pWaveFile;
};
