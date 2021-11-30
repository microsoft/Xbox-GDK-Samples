//--------------------------------------------------------------------------------------
// GameInputDevice.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// ATG Toolkit's GamePad implementation is for single-user cases and composes all input devices
// into a single state representation. This file contains classes for treating input from different
// sources separately using the GameInput library from the Microsoft GDK.
//--------------------------------------------------------------------------------------

#pragma once

#include "GamePad.h"

namespace ATG
{

    // Represents an input device
    class GameInputDevice
    {
    public:

        GameInputDevice(IGameInputDevice* inputDevice);
        ~GameInputDevice();

        IGameInputDevice* GetGameInputDevice() const;
        const APP_LOCAL_DEVICE_ID& GetDeviceId() const;

        void UpdateState(IGameInput* gameInput);
        bool ReadDevice(IGameInput* gameInput, DirectX::GamePad::State& state) const;
        const DirectX::GamePad::ButtonStateTracker& GetCurrentState() const;

    protected:

        // GameInput data
        IGameInputDevice*                       m_gameInputDevice;
        APP_LOCAL_DEVICE_ID                     m_deviceId;

        // State tracking
        DirectX::GamePad::ButtonStateTracker    m_buttonStateTracker;
    };

    // Tracks and managed multiple game input devices
    class GameInputCollection
    {
    public:

        GameInputCollection();
        ~GameInputCollection();

        void Update();

        void OnSuspend();
        void OnResume();

        // Returns a list of all devices
        std::vector<APP_LOCAL_DEVICE_ID> GetAllDevices() const;

        // Gets the current state of a device by id
        bool GetDeviceState(const APP_LOCAL_DEVICE_ID& deviceId, DirectX::GamePad::ButtonStateTracker* outDeviceState) const;

        // Simplifies checking input across multiple controllers by allowing for a lambda to be
        // passed which checks for the desired input across a filtered set of input devices.
        // If checking multiple devices, then only one device has to succeed for the function to return true.
        //
        // Example usage:
        //
        // std::vector<APP_LOCAL_DEVICE_ID> devices;
        // devices.push_back(myUserDevice1);
        // devices.push_back(myUserDevice2);
        // bool inputMatched = CheckInput(devices, [](DirectX::GamePad::ButtonStateTracker& tracker)
        // {
        //     return tracker.a == DirectX::GamePad::ButtonStateTracker::PRESSED;
        // });
        // if (inputMatched)
        // {
        //     DoSomething();
        // }
        bool CheckInput(const std::vector<APP_LOCAL_DEVICE_ID>& devicesToCheck, std::function<bool(const DirectX::GamePad::ButtonStateTracker&)> inputCheckLambda);
        bool CheckInputAllDevices(std::function<bool(const DirectX::GamePad::ButtonStateTracker&)> inputCheckLambda);

    protected:

        void AddDevice(IGameInputDevice* device);
        void RemoveDevice(IGameInputDevice* device);

    private:

        // Callback for device events
        static void CALLBACK GameInputDeviceCallback(
            _In_ GameInputCallbackToken callbackToken,
            _In_ void * context,
            _In_ IGameInputDevice * device,
            _In_ uint64_t timestamp,
            _In_ GameInputDeviceStatus currentStatus,
            _In_ GameInputDeviceStatus previousStatus);

    protected:

        // GameInput data
        IGameInput*                                     m_gameInput;
        IGameInputDispatcher*                           m_gameInputDispatcher;

        // Devices
        std::vector<std::unique_ptr<GameInputDevice>>   m_inputDevices;
    };

}
