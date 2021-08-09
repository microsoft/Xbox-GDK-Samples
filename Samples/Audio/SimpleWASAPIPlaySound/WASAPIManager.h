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

#ifndef WASAPI_MANAGER_H_INCLUDED
#define WASAPI_MANAGER_H_INCLUDED

#include "ToneSampleGenerator.h"

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

// User Configurable Arguments for Scenario
struct DEVICEPROPS
{
	bool					IsHWOffload;
	bool					IsBackground;
	bool					IsRawSupported;
	bool					IsRawChosen;
	REFERENCE_TIME          hnsBufferDuration;
	unsigned long           Frequency;
};

static const REFERENCE_TIME	REFTIMES_PER_SEC = 10000000LL;

class WASAPIManager
{
public:
    WASAPIManager() noexcept(false);

    WASAPIManager(WASAPIManager&&) = default;
    WASAPIManager& operator= (WASAPIManager&&) = default;

    WASAPIManager(WASAPIManager const&) = delete;
    WASAPIManager& operator= (WASAPIManager const&) = delete;

	HRESULT PlayPauseToggle();
    HRESULT InitializeDevice();
    bool IsPlaying() const { return m_state == DeviceState::DeviceStatePlaying; };
	HRESULT OnAudioSampleRequested(bool bIsSilence);
    HRESULT Restart();
    HRESULT Play();
    HRESULT Pause();
    HRESULT StopDevice();

	bool m_threadActive;
	HANDLE						m_SampleReadyEvent;
	DeviceState					m_state;

private:
    ~WASAPIManager();

    HRESULT StartDevice();
    HRESULT GetToneSample(UINT32 FramesAvailable);

	UINT32										m_BufferFrames;
	WAVEFORMATEX*								m_MixFormat;
	Microsoft::WRL::ComPtr<IAudioClient2>       m_AudioClient;
	Microsoft::WRL::ComPtr<IAudioRenderClient>	m_AudioRenderClient;

	DEVICEPROPS						m_DeviceProps;

	ToneSampleGenerator*			m_ToneSource;
	CRITICAL_SECTION				m_CritSec;

	//worker thread for renderer
	bool m_playingSound;
	PTP_WORK m_renderWorkThread;
};

#endif
