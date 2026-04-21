//--------------------------------------------------------------------------------------
// HapticHelper.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <map>
#include <wrl.h>

#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#elif GAMEINPUT_API_VERSION == 3
using namespace GameInput::v3;
#endif

#include "Audio/WASAPIManager.h"
#include "Audio/XAudio2Manager.h"
#include "HapticsDevice.h"

namespace ATG
{
    class HapticsManager
    {
        public:
            HapticsManager() = default;
            ~HapticsManager();

            HapticsManager(HapticsManager&&) = default;
            HapticsManager& operator= (HapticsManager&&) = default;

            HapticsManager(HapticsManager const&) = delete;
            HapticsManager& operator= (HapticsManager const&) = delete;

            HRESULT Initialize(IGameInput* gameInput);
            const HapticsDevice* GetHapticsDevice(IGameInputDevice* device);
            size_t GetDeviceCount() const { return m_devices.size(); }

        private:
            static void DeviceCallback(GameInputCallbackToken, void* context, IGameInputDevice* device, uint64_t, GameInputDeviceStatus currentStatus, GameInputDeviceStatus previousStatus) noexcept;
            HRESULT AddHapticsDevice(IGameInputDevice* device, wchar_t* endpoint, uint32_t locationCount, GUID* locations);

            std::map<Microsoft::WRL::ComPtr<IGameInputDevice>, HapticsDevice> m_devices {};
            Microsoft::WRL::ComPtr<IGameInput> m_gameInput = nullptr;
            GameInputCallbackToken m_deviceCallbackToken = 0;
    };
}
