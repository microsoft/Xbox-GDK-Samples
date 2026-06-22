//--------------------------------------------------------------------------------------
// RawWheelLibrary.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <GameInput.h>

namespace Xbox
{
    class RawWheel final
    {
    public:

        RawWheel() noexcept(false);

        RawWheel(RawWheel&&) = default;
        RawWheel& operator= (RawWheel&&) = default;

        RawWheel(RawWheel const&) = default;
        RawWheel& operator= (RawWheel const&) = default;

        bool operator==(const RawWheel& lhs) const noexcept
        {
            return m_device.Get() == lhs.m_device.Get();
        }

        bool Init(IGameInput * gameInput, IGameInputDevice * device);

        bool CompareDevice(IGameInputDevice * device) { return device == m_device.Get(); }

        void Update();

        void RequestState();

        const wchar_t* DeviceString() const { return m_deviceString.c_str(); }
        const wchar_t* ButtonString() const { return m_buttonString.c_str(); }
        const wchar_t* RawMessageString() const { return m_rawMessageString; }

        float Wheel() const noexcept { return m_wheelValue; }
        float Clutch() const noexcept { return m_clutchValue; }
        float Brake() const noexcept { return m_brakeValue; }
        float Throttle() const noexcept { return m_throttleValue; }
        float Handbrake() const noexcept { return m_handbrakeValue; }
        int32_t Gear() const noexcept { return m_patternShifterGearValue; }

    private:

        //Input states
        Microsoft::WRL::ComPtr<IGameInput>			m_gameInput;
        Microsoft::WRL::ComPtr<IGameInputReading>   m_lastReading;

        Microsoft::WRL::ComPtr<IGameInputDevice>    m_device;

        wchar_t                                     m_rawMessageString[256];
        std::wstring                                m_deviceString;
        std::wstring                                m_rawReportString;
        uint8_t                                     m_currentSequence;
        bool                                        m_keyDown;

        std::wstring			                    m_buttonString;
        float 						                m_wheelValue;
        float                                       m_clutchValue;
        float                                       m_brakeValue;
        float                                       m_throttleValue;
        float                                       m_handbrakeValue;
        int32_t                                     m_patternShifterGearValue;
    };
}
