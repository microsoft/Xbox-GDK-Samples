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

#include "WaveSampleGenerator.h"

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

static constexpr REFERENCE_TIME REFTIMES_PER_SEC = 10000000LL;

class WASAPIManager
{
public:
    WASAPIManager() noexcept(false);
	~WASAPIManager();

    WASAPIManager(WASAPIManager&&) = default;
    WASAPIManager& operator= (WASAPIManager&&) = default;

    WASAPIManager(WASAPIManager const&) = delete;
    WASAPIManager& operator= (WASAPIManager const&) = delete;

	HRESULT PlayPauseToggle();
    HRESULT InitializeDevice(wchar_t* endpoint, uint32_t locationCount, GUID* locations);
    bool IsPlaying() const { return m_state == DeviceState::DeviceStatePlaying; };
	HRESULT OnAudioSampleRequested(bool bIsSilence);
    HRESULT Restart();
    HRESULT Play();
    HRESULT Pause();
    HRESULT Stop();
	HRESULT ConfigureWaveSource(const wchar_t* filename);

    bool						m_playingSound;
	HANDLE						m_sampleReadyEvent;
	DeviceState					m_state;

private:
    HRESULT StartDevice();
    HRESULT GetSample(UINT32 FramesAvailable);

	UINT32										m_framesPerPeriod = 0;

	IMMDevice*									m_device;
	uint32_t                                    m_locationCount;
	GUID*                                       m_locations;

	UINT32										m_BufferFrames;
	WAVEFORMATEX*								m_MixFormat;
	Microsoft::WRL::ComPtr<IAudioClient2>       m_AudioClient;
	Microsoft::WRL::ComPtr<IAudioRenderClient>	m_audioRenderClient;

	std::shared_ptr<WaveSampleGenerator>        m_waveGenerator;
	std::shared_ptr<IGenerator>                 m_currentGenerator;

	std::unique_ptr<uint8_t[]>                  m_waveFileData;

	//worker thread for renderer
	CRITICAL_SECTION							m_CritSec;
	HANDLE  									m_renderWorkThread;
};
