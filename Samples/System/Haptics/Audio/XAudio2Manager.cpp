//--------------------------------------------------------------------------------------
// XAudio2Manager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "XAudio2Manager.h"
#include "WAVFileReader.h"

#include <xaudio2.h>

XAudio2Manager::XAudio2Manager() noexcept(false) :
    m_xAudio2(nullptr),
    m_masteringVoice(nullptr),
    m_sourceVoice(nullptr),
    m_locationCount(0),
    m_locations(nullptr),
    m_playing(false)
{
}

XAudio2Manager::~XAudio2Manager()
{
    Shutdown();
}

HRESULT XAudio2Manager::InitializeDevice(wchar_t* endpoint, uint32_t locationCount, GUID* locations)
{
    if (!endpoint)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    m_locationCount = locationCount;
    m_locations = new GUID[locationCount];
    memcpy(m_locations, locations, locationCount * sizeof(GUID));

    // Initialize XAudio2 objects
    if (m_xAudio2)
    {
        // if there's an existing XAudio2 stack, shut down all parts before creating a new one
        // this can happen on controller disconnect/reconnect
        Shutdown();
    }

    hr = XAudio2Create(m_xAudio2.GetAddressOf(), 0);
    if (FAILED(hr))
    {
        return hr;
    }

#ifdef _DEBUG
    // Enable debugging features
    XAUDIO2_DEBUG_CONFIGURATION debug = {};
    debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
    debug.BreakMask = XAUDIO2_LOG_ERRORS;
    m_xAudio2->SetDebugConfiguration(&debug, 0);
#endif

    hr = m_xAudio2->RegisterForCallbacks(this);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_xAudio2->CreateMasteringVoice(&m_masteringVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, endpoint);
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

HRESULT XAudio2Manager::ConfigureWaveSource(const wchar_t* filename)
{
    HRESULT hr = S_OK;

    if(m_sourceVoice)
    {
        hr = m_sourceVoice->Stop();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    // Read the wave file
    DX::WAVData waveData;
    hr = DX::LoadWAVAudioFromFileEx(filename, m_waveFileData, waveData);
    if (FAILED(hr))
    {
        return hr;
    }

    // Create the source voice
    hr = m_xAudio2->CreateSourceVoice(&m_sourceVoice, waveData.wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, this);
    if (FAILED(hr))
    {
        return hr;
    }

    // Submit wave data
    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = waveData.startAudio;
    buffer.Flags = XAUDIO2_END_OF_STREAM;       // Indicates all the audio data is being submitted at once
    buffer.AudioBytes = waveData.audioBytes;

    if (waveData.loopLength > 0)
    {
        buffer.LoopBegin = waveData.loopStart;
        buffer.LoopLength = waveData.loopLength;
        buffer.LoopCount = 0;
    }

    hr = m_sourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
    {
        return hr;
    }

    WORD wavChannels = waveData.wfx->nChannels;

    // Assumes haptic locations map to XAudio2-happy default channels
    // https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-default-channel-mapping
    if(wavChannels != m_locationCount)
    {
        // create an appropriately sized output matrix (source channels * destination channels)
        float *matrix = new float[wavChannels * m_locationCount];
        memset(matrix, 0, wavChannels * m_locationCount * sizeof(float));

        for (uint32_t d = 0; d < m_locationCount; d++)
        {
            if (m_locations[d] == GAMEINPUT_HAPTIC_LOCATION_GRIP_LEFT || m_locations[d] == GAMEINPUT_HAPTIC_LOCATION_TRIGGER_LEFT)
            {
                matrix[wavChannels * d + 0] = 1.0f;
            }
            else if (m_locations[d] == GAMEINPUT_HAPTIC_LOCATION_GRIP_RIGHT || m_locations[d] == GAMEINPUT_HAPTIC_LOCATION_TRIGGER_RIGHT)
            {
                matrix[wavChannels * d + 1] = 1.0f;
            }
        }

        hr = m_sourceVoice->SetOutputMatrix(nullptr, wavChannels, m_locationCount, matrix);
        if (FAILED(hr))
            return hr;

        delete[] matrix;
    }

    return S_OK;
}

HRESULT XAudio2Manager::Play()
{
    if(m_sourceVoice)
    {
        m_playing = true;
        return m_sourceVoice->Start();
    }
    return S_OK;
}

HRESULT XAudio2Manager::Stop()
{
    if(m_sourceVoice)
    {
        m_playing = false;
        return m_sourceVoice->Stop();
    }
    return S_OK;
}

HRESULT XAudio2Manager::TitleSuspend()
{
    if (m_xAudio2)
    {
        m_xAudio2->StopEngine();
    }
    return S_OK;
}

HRESULT XAudio2Manager::TitleResume()
{
    if (m_xAudio2)
    {
        return m_xAudio2->StartEngine();
    }
    return S_OK;
}

void XAudio2Manager::Shutdown()
{
    if (m_xAudio2)
    {
        m_xAudio2->StopEngine();

        m_xAudio2->UnregisterForCallbacks(this);

        if (m_sourceVoice)
        {
            m_sourceVoice->DestroyVoice();
            m_sourceVoice = nullptr;
        }

        if (m_masteringVoice)
        {
            m_masteringVoice->DestroyVoice();
            m_masteringVoice = nullptr;
        }

        m_xAudio2.Reset();
    }

    if(m_locations)
    {
        delete[] m_locations;
        m_locations = nullptr;
    }
}

void __stdcall XAudio2Manager::OnCriticalError(HRESULT hr) noexcept { char buff[256]; sprintf_s(buff, "XAudio2 Critical Error: %08X\n", hr); OutputDebugStringA(buff); }
void __stdcall XAudio2Manager::OnProcessingPassStart(void) noexcept {}
void __stdcall XAudio2Manager::OnProcessingPassEnd(void) noexcept {}

void __stdcall XAudio2Manager::OnVoiceProcessingPassStart(UINT32) noexcept {}
void __stdcall XAudio2Manager::OnVoiceProcessingPassEnd(void) noexcept {}
void __stdcall XAudio2Manager::OnStreamEnd(void) noexcept {}
void __stdcall XAudio2Manager::OnBufferStart(void*) noexcept { m_playing = true; }
void __stdcall XAudio2Manager::OnBufferEnd(void*) noexcept { m_playing = false; }
void __stdcall XAudio2Manager::OnLoopEnd(void*) noexcept {}
void __stdcall XAudio2Manager::OnVoiceError(void*, HRESULT hr) noexcept { char buff[256]; sprintf_s(buff, "XAudio2 Voice Error: %08X\n", hr); OutputDebugStringA(buff); }
