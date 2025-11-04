//--------------------------------------------------------------------------------------
// GameInputManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "GameInputManager.h"

bool GameInputManager::Init()
{
    if (FAILED(GameInputCreate(m_gameInput.GetAddressOf())))
        return false;

    HRESULT hr = m_gameInput->RegisterDeviceCallback(
        nullptr,
        GameInputKindGamepad,
        GameInputDeviceConnected,
        GameInputBlockingEnumeration,
        nullptr,
        &DeviceCallback,
        &m_deviceToken);

    return SUCCEEDED(hr);
}

void GameInputManager::Shutdown()  
{
    // Unregister GameInput device callback
    if (m_gameInput && m_deviceToken)
    {
        m_gameInput->UnregisterCallback(m_deviceToken, 100000);
        m_deviceToken = 0;
    }

    // Unregister all devices
    for (auto& registeredDevice : m_devices)
    {
        registeredDevice.Reset();
    }

    m_gameInput.Reset();  
    m_devices.clear();  
    m_activeKind = static_cast<GameInputKind>(0);  
}

GameInputKind GameInputManager::GetActiveGameInputKind()
{
    std::scoped_lock lock(m_mutex);
    return m_activeKind;
}

void CALLBACK GameInputManager::DeviceCallback(
    GameInputCallbackToken,
    void*,
    IGameInputDevice* device,
    uint64_t,
    GameInputDeviceStatus current,
    GameInputDeviceStatus previous)
{
    std::scoped_lock lock(m_mutex);

    bool wasConnected = (previous & GameInputDeviceConnected) != 0;
    bool isConnected = (current & GameInputDeviceConnected) != 0;

    if (!wasConnected && isConnected)
    {
        // Device connected
        m_devices.emplace_back(device);
        m_activeKind |= device->GetDeviceInfo()->supportedInput;
    }
    else if (wasConnected && !isConnected)
    {
        // 1) Remove the device from our vector
        m_devices.erase(
            std::remove_if(
                m_devices.begin(),
                m_devices.end(),
                [&](auto& d) { return d.Get() == device; }
            ),
            m_devices.end());

        // 2) Re-compute which input kinds are still available
        m_activeKind = static_cast<GameInputKind>(0);
        for (auto& d : m_devices)
        {
            m_activeKind |= d->GetDeviceInfo()->supportedInput;
        }
    }
}
