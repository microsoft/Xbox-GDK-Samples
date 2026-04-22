//--------------------------------------------------------------------------------------
// HapticsDevice.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#elif GAMEINPUT_API_VERSION == 3
using namespace GameInput::v3;
#endif

#include <memory>
#include "Audio/WASAPIManager.h"
#include "Audio/XAudio2Manager.h"

namespace ATG
{
    enum class HapticPlaybackEngine
    {
        WASAPI,
        XAudio2
    };

    class HapticsDevice
    {
        public:
            HapticsDevice() = default;
            ~HapticsDevice() = default;

            HapticsDevice(HapticsDevice&&) = delete;
            HapticsDevice& operator= (HapticsDevice&&) = default;

            HapticsDevice(HapticsDevice const&) = delete;
            HapticsDevice& operator= (HapticsDevice const&) = delete;

            void PlayWAVFile(const wchar_t* filename, HapticPlaybackEngine engine) const;
            void PlayWAVData(const uint8_t* wavData, size_t wavDataSize, HapticPlaybackEngine engine) const;
            void Stop() const;
            bool IsPlaying() const { return (m_wasapi && m_wasapi->IsPlaying()) || (m_xaudio && m_xaudio->IsPlaying()); }

        private:
            HRESULT Initialize(IGameInputDevice* giDevice, wchar_t* endpoint, uint32_t locationCount, GUID* locations);
            std::unique_ptr<WASAPIManager> m_wasapi {};
            std::unique_ptr<XAudio2Manager> m_xaudio {};

        friend class HapticsManager;
    };
}
