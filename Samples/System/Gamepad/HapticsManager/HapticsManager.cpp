//--------------------------------------------------------------------------------------
// HapticsManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HapticsManager.h"
#include "HapticsDevice.h"

using namespace ATG;

HRESULT HapticsManager::Initialize(IGameInput* gameInput)
{
    m_gameInput = gameInput;

    if(m_deviceCallbackToken == 0)
    {
        return m_gameInput->RegisterDeviceCallback(nullptr,
            GameInputKind::GameInputKindController,
            GameInputDeviceStatus::GameInputDeviceAnyStatus,
            GameInputEnumerationKind::GameInputBlockingEnumeration,
            this,
            &HapticsManager::DeviceCallback, &m_deviceCallbackToken);
    }
    return E_NOT_VALID_STATE;
}

HapticsManager::~HapticsManager()
{
    if(m_deviceCallbackToken != 0)
    {
        m_gameInput->UnregisterCallback(m_deviceCallbackToken);
    }
}

void HapticsManager::DeviceCallback(GameInputCallbackToken, void* context, IGameInputDevice* gameInputDevice, uint64_t,
                                    GameInputDeviceStatus currentStatus, GameInputDeviceStatus previousStatus) noexcept
{
    HapticsManager* manager = reinterpret_cast<HapticsManager*>(context);

    bool wasConnected = (previousStatus & GameInputDeviceConnected) != 0;
    bool isConnected = (currentStatus & GameInputDeviceConnected) != 0;

    // Haptic information can become ready after the device connects.
    if(isConnected && currentStatus & GameInputDeviceStatus::GameInputDeviceHapticInfoReady)
    {
        GameInputHapticInfo hapticInfo;
        if(FAILED(gameInputDevice->GetHapticInfo(&hapticInfo)))
        {
            return;
        }

        manager->AddHapticsDevice(gameInputDevice, hapticInfo.audioEndpointId, hapticInfo.locationCount, hapticInfo.locations);
    }

    if(wasConnected && !isConnected && manager->m_devices.find(gameInputDevice) != manager->m_devices.end())
    {
        manager->m_devices.erase(gameInputDevice);
    }
}

HRESULT HapticsManager::AddHapticsDevice(IGameInputDevice* gameInputDevice, wchar_t* endpoint, uint32_t locationCount, GUID* locations)
{
    if (m_devices.find(gameInputDevice) == m_devices.end())
    {
        HapticsDevice hapticsDevice;

        HRESULT hr = hapticsDevice.Initialize(gameInputDevice, endpoint, locationCount, locations);
        if (FAILED(hr))
        {
            return hr;
        }

        m_devices[gameInputDevice] = std::move(hapticsDevice);
    }

    return S_OK;
}

const HapticsDevice* HapticsManager::GetHapticsDevice(IGameInputDevice* device)
{
    auto it = m_devices.find(device);
    if(it != m_devices.end())
    {
        return &it->second;
    }
    return nullptr;
}

void HapticsManager::StopAllDevices()
{
    for (auto& deviceEntry : m_devices)
    {
        deviceEntry.second.Stop();
    }
}
