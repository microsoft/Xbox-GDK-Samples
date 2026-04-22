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

void HapticsManager::DeviceCallback(GameInputCallbackToken, void* context, IGameInputDevice* giDevice, uint64_t,
                                    GameInputDeviceStatus currentStatus, GameInputDeviceStatus previousStatus) noexcept
{
    HapticsManager* mgr = reinterpret_cast<HapticsManager*>(context);

    const GameInputDeviceInfo* di = nullptr;
    if(FAILED(giDevice->GetDeviceInfo(&di)))
    {
        return;
    }

    bool wasConnected = (previousStatus & GameInputDeviceConnected) != 0;
    bool isConnected = (currentStatus & GameInputDeviceConnected) != 0;

    // newly connected haptics device, initialize haptics
    if(isConnected && currentStatus & GameInputDeviceStatus::GameInputDeviceHapticInfoReady)
    {
        GameInputHapticInfo hapticInfo;
        if(FAILED(giDevice->GetHapticInfo(&hapticInfo)))
        {
            return;
        }

        mgr->AddHapticsDevice(giDevice, hapticInfo.audioEndpointId, hapticInfo.locationCount, hapticInfo.locations);
    }

    // newly disconnected device, remove from our list
    if(wasConnected && !isConnected && mgr->m_devices.find(giDevice) != mgr->m_devices.end())
    {
        mgr->m_devices.erase(giDevice);
    }
}

HRESULT HapticsManager::AddHapticsDevice(IGameInputDevice* giDevice, wchar_t* endpoint, uint32_t locationCount, GUID* locations)
{
    // don't add the same device twice
    if (m_devices.find(giDevice) == m_devices.end())
    {
        HapticsDevice hd;

        HRESULT hr = hd.Initialize(giDevice, endpoint, locationCount, locations);
        if (FAILED(hr))
        {
            return hr;
        }

        m_devices[giDevice] = std::move(hd);
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
