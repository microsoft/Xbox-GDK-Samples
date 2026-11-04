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
            this,
            HapticsManager::OnDeviceChanged, &m_deviceCallbackToken);
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

void CALLBACK HapticsManager::OnDeviceChanged(
    GameInputCallbackToken /*token*/,
    void* context,
    GameInputDeviceEvent deviceEvent,
    IGameInputDevice* device) noexcept
{
    HapticsManager* manager = reinterpret_cast<HapticsManager*>(context);

    switch(deviceEvent)
    {
        case GameInputDeviceEventHapticInfoUpdated:
        {
            GameInputHapticInfo hapticInfo = {};
            if (FAILED(device->GetHapticInfo(&hapticInfo)))
            {
                return;
            }
            manager->AddHapticsDevice(device, hapticInfo.audioEndpointId, hapticInfo.locationCount, hapticInfo.locations);
            break;
        }

        case GameInputDeviceEventDisconnected:
            if (manager->m_devices.find(device) != manager->m_devices.end())
            {
                manager->m_devices.erase(device);
            }
            break;

        case GameInputDeviceEventConnected:
        case GameInputDeviceEventKeyboardLayoutUpdated:
        case GameInputDeviceEventXInputIndexUpdated:
            break;
    }
}

HRESULT HapticsManager::AddHapticsDevice(IGameInputDevice* gameInputDevice, wchar_t* endpoint, uint32_t locationCount, GUID* locations)
{
    if (m_devices.find(gameInputDevice) == m_devices.end())
    {
        HapticsDevice hapticsDevice;

        HRESULT hr = hapticsDevice.Initialize(endpoint, locationCount, locations);
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
