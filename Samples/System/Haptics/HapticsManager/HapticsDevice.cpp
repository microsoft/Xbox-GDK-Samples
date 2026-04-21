//--------------------------------------------------------------------------------------
// HapticsDevice.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HapticsDevice.h"

namespace ATG
{
    HRESULT HapticsDevice::Initialize(IGameInputDevice* giDevice, wchar_t* endpoint, uint32_t locationCount, GUID* locations)
    {
        const GameInputDeviceInfo* di = nullptr;
        HRESULT hr = giDevice->GetDeviceInfo(&di);
        if (FAILED(hr))
        {
            return hr;
        }

        // create a WASAPI engine for the device
        m_wasapi = std::make_unique<WASAPIManager>();
        hr = m_wasapi->InitializeDevice(endpoint, locationCount, locations);
        if (FAILED(hr))
        {
            return hr;
        }

        // create an XAudio2 engine for the device
        m_xaudio = std::make_unique<XAudio2Manager>();
        hr = m_xaudio->InitializeDevice(endpoint, locationCount, locations);
        if (FAILED(hr))
        {
            return hr;
        }

        return S_OK;
    }

    void HapticsDevice::PlayWAVFile(const wchar_t* filename, HapticPlaybackEngine engine) const
    {
        switch(engine)
        {
            case HapticPlaybackEngine::WASAPI:
                if(m_wasapi)
                {
                    m_wasapi->ConfigureWaveSource(filename);
                    m_wasapi->Play();
                }
                break;

            case HapticPlaybackEngine::XAudio2:
                if(m_xaudio)
                {
                    m_xaudio->ConfigureWaveSource(filename);
                    m_xaudio->Play();
                }
                break;
        }
    }

    void HapticsDevice::PlayWAVData(const uint8_t* wavData, size_t wavDataSize, HapticPlaybackEngine engine) const
    {
        switch(engine)
        {
            case HapticPlaybackEngine::WASAPI:
                if(m_wasapi)
                {
                    m_wasapi->ConfigureWaveSource(wavData, wavDataSize);
                    m_wasapi->Play();
                }
                break;
            case HapticPlaybackEngine::XAudio2:
                if(m_xaudio)
                {
                    m_xaudio->ConfigureWaveSource(wavData, wavDataSize);
                    m_xaudio->Play();
                }
                break;
        }
    }

    void HapticsDevice::Stop() const
    {
        if(m_wasapi)
        {
            m_wasapi->Stop();
        }
        if(m_xaudio)
        {
            m_xaudio->Stop();
        }
    }
}
