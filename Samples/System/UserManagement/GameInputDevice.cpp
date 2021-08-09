//--------------------------------------------------------------------------------------
// GameInputDevice.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "GameInputDevice.h"

using namespace ATG;

namespace
{
    float ApplyLinearDeadZone(float value, float maxValue, float deadZoneSize)
    {
        if (value < -deadZoneSize)
        {
            // Increase negative values to remove the deadzone discontinuity.
            value += deadZoneSize;
        }
        else if (value > deadZoneSize)
        {
            // Decrease positive values to remove the deadzone discontinuity.
            value -= deadZoneSize;
        }
        else
        {
            // Values inside the deadzone come out zero.
            return 0;
        }

        // Scale into 0-1 range.
        float scaledValue = value / (maxValue - deadZoneSize);
        return std::max(-1.f, std::min(scaledValue, 1.f));
    }
}

GameInputDevice::GameInputDevice(IGameInputDevice* inputDevice)
    : m_gameInputDevice(inputDevice)
{
    assert(m_gameInputDevice);

    // Acquire the device Id which is immutable and consistent
    auto deviceInfo = m_gameInputDevice->GetDeviceInfo();

    m_deviceId = deviceInfo->deviceId;
}

GameInputDevice::~GameInputDevice()
{
}

IGameInputDevice* GameInputDevice::GetGameInputDevice() const
{
    return m_gameInputDevice;
}

const APP_LOCAL_DEVICE_ID& GameInputDevice::GetDeviceId() const
{
    return m_deviceId;
}

void GameInputDevice::UpdateState(IGameInput* gameInput)
{
    DirectX::GamePad::State newState;
    ReadDevice(gameInput, newState);
    m_buttonStateTracker.Update(newState);
}

bool GameInputDevice::ReadDevice(IGameInput* gameInput, DirectX::GamePad::State& state) const
{
    // Only implementing gamepad input currently
    IGameInputReading* reading = nullptr;
    if (SUCCEEDED(gameInput->GetCurrentReading(GameInputKindGamepad, m_gameInputDevice, &reading)))
    {
        GameInputGamepadState pad;
        if (reading->GetGamepadState(&pad))
        {
            state.connected = true;
            state.packet = reading->GetTimestamp();

            state.buttons.a = (pad.buttons & GameInputGamepadA) != 0;
            state.buttons.b = (pad.buttons & GameInputGamepadB) != 0;
            state.buttons.x = (pad.buttons & GameInputGamepadX) != 0;
            state.buttons.y = (pad.buttons & GameInputGamepadY) != 0;
            state.buttons.leftStick = (pad.buttons & GameInputGamepadLeftThumbstick) != 0;
            state.buttons.rightStick = (pad.buttons & GameInputGamepadRightThumbstick) != 0;
            state.buttons.leftShoulder = (pad.buttons & GameInputGamepadLeftShoulder) != 0;
            state.buttons.rightShoulder = (pad.buttons & GameInputGamepadRightShoulder) != 0;
            state.buttons.view = (pad.buttons & GameInputGamepadView) != 0;
            state.buttons.menu = (pad.buttons & GameInputGamepadMenu) != 0;

            state.dpad.up = (pad.buttons & GameInputGamepadDPadUp) != 0;
            state.dpad.down = (pad.buttons & GameInputGamepadDPadDown) != 0;
            state.dpad.right = (pad.buttons & GameInputGamepadDPadRight) != 0;
            state.dpad.left = (pad.buttons & GameInputGamepadDPadLeft) != 0;

            state.thumbSticks.leftX = ApplyLinearDeadZone(pad.leftThumbstickX, 1.0f, 0.24f);
            state.thumbSticks.leftY = ApplyLinearDeadZone(pad.leftThumbstickY, 1.0f, 0.24f);

            state.thumbSticks.rightX = ApplyLinearDeadZone(pad.rightThumbstickX, 1.0f, 0.24f);
            state.thumbSticks.rightY = ApplyLinearDeadZone(pad.rightThumbstickY, 1.0f, 0.24f);

            state.triggers.left = pad.leftTrigger;
            state.triggers.right = pad.rightTrigger;

            return true;
        }
    }

    return false;
}

const DirectX::GamePad::ButtonStateTracker& GameInputDevice::GetCurrentState() const
{
    return m_buttonStateTracker;
}

GameInputCollection::GameInputCollection()
{
    // Create game input
    DX::ThrowIfFailed(GameInputCreate(&m_gameInput));

    // Create a manual dispatcher for game input events. This gives the application full control over
    // when and where events are executed for game input. To enforce thread-safe behavior and simple
    // understanding, this will dispatch all events on the main thread.
    DX::ThrowIfFailed(m_gameInput->CreateDispatcher(&m_gameInputDispatcher));

    // Register a callback to listen for device change events which will be fired with the game input dispatcher.
    // The callback will be set to enumerate the initial connected devices such that this collection can get a proper
    // view of all input devices connected and update accordingly
    DX::ThrowIfFailed(m_gameInput->RegisterDeviceCallback(
        nullptr,                    // Don't filter to events from a specific device
        GameInputKindGamepad,       // Only tracking gamepads
        GameInputDeviceConnected,   // Only track connection events
        GameInputAsyncEnumeration,  // Enumerate devices to get an initial list, use our dispatcher to handle the event timing
        this,                       // Pass ptr to self for context
        &GameInputDeviceCallback,   // Callback function
        nullptr                     // Callback token, if desired for removing the callback later
    ));
}

GameInputCollection::~GameInputCollection()
{
    // Clear list of devices first as those will cleanup their device references
    m_inputDevices.clear();
    m_gameInputDispatcher->Release();
    m_gameInput->Release();
}

void GameInputCollection::Update()
{
    // Dispatch any events that have potentially queued up
    bool bMore = false;
    do
    {
        bMore = m_gameInputDispatcher->Dispatch(0);
    } while (bMore);

    // Update the state of all devices
    for (auto& device : m_inputDevices)
    {
        device->UpdateState(m_gameInput);
    }
}

void GameInputCollection::OnSuspend()
{
    // Nothing to do
}

void GameInputCollection::OnResume()
{
    // Nothing to do
}

std::vector<APP_LOCAL_DEVICE_ID> GameInputCollection::GetAllDevices() const
{
    std::vector<APP_LOCAL_DEVICE_ID> outDevices;
    outDevices.reserve(m_inputDevices.size());
    for (auto& device : m_inputDevices)
    {
        outDevices.push_back(device->GetDeviceId());
    }
    return outDevices;
}

bool GameInputCollection::GetDeviceState(const APP_LOCAL_DEVICE_ID& deviceId, DirectX::GamePad::ButtonStateTracker* outDeviceState) const
{
    assert(outDeviceState);

    for (auto& trackedDevice : m_inputDevices)
    {
        if (trackedDevice->GetDeviceId() == deviceId)
        {
            *outDeviceState = trackedDevice->GetCurrentState();
            return true;
        }
    }

    return false;
}

bool GameInputCollection::CheckInput(const std::vector<APP_LOCAL_DEVICE_ID>& devicesToCheck, std::function<bool(const DirectX::GamePad::ButtonStateTracker&)> inputCheckLambda)
{
    assert(inputCheckLambda);

    if (devicesToCheck.size() == 0)
    {
        return false;
    }

    for (auto& trackedDevice : m_inputDevices)
    {
        for (auto& deviceIdToCheck : devicesToCheck)
        {
            if (trackedDevice->GetDeviceId() == deviceIdToCheck)
            {
                if (inputCheckLambda(trackedDevice->GetCurrentState()))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool GameInputCollection::CheckInputAllDevices(std::function<bool(const DirectX::GamePad::ButtonStateTracker&)> inputCheckLambda)
{
    for (auto& trackedDevice : m_inputDevices)
    {
        if (inputCheckLambda(trackedDevice->GetCurrentState()))
        {
            return true;
        }
    }

    return false;
}

void GameInputCollection::AddDevice(IGameInputDevice* device)
{
#ifdef _DEBUG
    // There should not be an instance of this device already
    for (auto& existingDevice : m_inputDevices)
    {
        assert(existingDevice->GetGameInputDevice() != device);
    }
#endif

    std::unique_ptr<GameInputDevice> newDevice = std::make_unique<GameInputDevice>(device);
    m_inputDevices.push_back(std::move(newDevice));
}

void GameInputCollection::RemoveDevice(IGameInputDevice* device)
{
    for (unsigned int deviceIndex = 0; deviceIndex < m_inputDevices.size(); ++deviceIndex)
    {
        // The pointer for a device is guaranteed to be the same as long as the device remains on.
        // However, the next time it is turned on, it might change. Since this is a "turning-off"
        // event, the pointer will be the same.
        if (m_inputDevices[deviceIndex]->GetGameInputDevice() == device)
        {
            m_inputDevices.erase(m_inputDevices.begin() + deviceIndex);
            return;
        }
    }

    // Device must exist in list if it's being removed
    assert(0);
}

void GameInputCollection::GameInputDeviceCallback(
    _In_ GameInputCallbackToken /*callbackToken*/,
    _In_ void * context,
    _In_ IGameInputDevice * device,
    _In_ uint64_t /*timestamp*/,
    _In_ GameInputDeviceStatus currentStatus,
    _In_ GameInputDeviceStatus previousStatus)
{
    assert(context);
    GameInputCollection* pThis = static_cast<GameInputCollection*>(context);

    // Log the state change
    {
        auto deviceInfo = device->GetDeviceInfo();
        APP_LOCAL_DEVICE_ID deviceId = deviceInfo->deviceId;

        char debugString[512] = {};
        sprintf_s(debugString, 512, u8"GameInputDeviceCallback() : device = 0x%p, deviceId = %08x-%08x-%08x-%08x-%08x-%08x-%08x-%08x, Previous:%d, Current:%d\n",
            device,
            *reinterpret_cast<unsigned int*>(&deviceId.value[0]),
            *reinterpret_cast<unsigned int*>(&deviceId.value[4]),
            *reinterpret_cast<unsigned int*>(&deviceId.value[8]),
            *reinterpret_cast<unsigned int*>(&deviceId.value[12]),
            *reinterpret_cast<unsigned int*>(&deviceId.value[16]),
            *reinterpret_cast<unsigned int*>(&deviceId.value[20]),
            *reinterpret_cast<unsigned int*>(&deviceId.value[24]),
            *reinterpret_cast<unsigned int*>(&deviceId.value[28]),
            previousStatus,
            currentStatus
        );
        OutputDebugStringA(debugString);
    }

    if (currentStatus & GameInputDeviceConnected)
    {
        pThis->AddDevice(device);
    }
    else
    {
        pThis->RemoveDevice(device);
    }
}
