//--------------------------------------------------------------------------------------
// RawWheelLibrary.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "RawWheelLibrary.h"

using Microsoft::WRL::ComPtr;
using namespace Xbox;

RawWheel::RawWheel() noexcept(false) :
    m_currentSequence(0),
    m_keyDown(false),
    m_wheelValue(0.f),
    m_clutchValue(0.f),
    m_brakeValue(0.f),
    m_throttleValue(0.f),
    m_handbrakeValue(0.f),
    m_patternShifterGearValue(0)
{
    ZeroMemory(m_rawMessageString, sizeof(m_rawMessageString));
    m_device.Reset();
    m_deviceString.clear();
    m_rawReportString.clear();
}

bool RawWheel::Init(IGameInput * gameInput, IGameInputDevice * device)
{
    if (gameInput != nullptr && device != nullptr)
    {
        m_gameInput = gameInput;

        //Get information about the device
        const auto deviceInfo = device->GetDeviceInfo();

        //Ensure this device matches the capabilities we expect
        //In practice, using the VID and PID to determine a specific device for this library would be common
        GameInputKind requiredKind = GameInputKindRacingWheel | GameInputKindRawDeviceReport;
        if ((deviceInfo->supportedInput & requiredKind) == requiredKind && m_gameInput != nullptr)
        {
            m_device = device;

            m_deviceString.append(L"VendorId: ");
            m_deviceString.append(std::to_wstring(deviceInfo->vendorId));
            m_deviceString.append(L" ProductId: ");
            m_deviceString.append(std::to_wstring(deviceInfo->productId));

            //We have a new device, so get the current reading as a frame of reference for future readings
            if (SUCCEEDED(m_gameInput->GetCurrentReading(GameInputKindRacingWheel | GameInputKindRawDeviceReport, m_device.Get(), &m_lastReading)))
            {
                return true;
            }
        }
    }

    return false;
}

void RawWheel::Update()
{
    if (m_gameInput)
    {
        m_buttonString.clear();

        while (m_lastReading)
        {
            ComPtr<IGameInputReading> nextReading;
            HRESULT hr = m_gameInput->GetNextReading(m_lastReading.Get(), GameInputKindRacingWheel | GameInputKindRawDeviceReport, m_device.Get(), &nextReading);

            // A failure breaks us out of the loop.
            if (FAILED(hr))
            {
                // A "reading not found" error means that we've simply reached
                // the end of the input history for this device.  Other errors
                // mean something worse has happened (e.g. device disconnected),
                // so reset our state if any of those happen.
                if (hr != GAMEINPUT_E_READING_NOT_FOUND)
                {
                    std::ignore = m_gameInput->GetCurrentReading(GameInputKindRacingWheel | GameInputKindRawDeviceReport, m_device.Get(), &m_lastReading);
                }

                break;
            }

            // Otherwise, make this the new last known reading and keep going.
            m_lastReading.Swap(nextReading);

            GameInputRacingWheelState racingWheelState;
            if (m_lastReading->GetRacingWheelState(&racingWheelState))
            {
                if (racingWheelState.buttons & GameInputRacingWheelDpadUp)
                {
                    m_buttonString += L"[DPad]Up ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelDpadDown)
                {
                    m_buttonString += L"[DPad]Down ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelDpadRight)
                {
                    m_buttonString += L"[DPad]Right ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelDpadLeft)
                {
                    m_buttonString += L"[DPad]Left ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelPreviousGear)
                {
                    m_buttonString += L"PrevGear ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelNextGear)
                {
                    m_buttonString += L"NextGear ";
                }

                if (racingWheelState.buttons & GameInputRacingWheelMenu)
                {
                    m_buttonString += L"[Menu] ";

                    if (!m_keyDown)
                    {
                        m_keyDown = true;
                        RequestState();
                    }
                }
                else
                {
                    m_keyDown = false;
                }

                if (racingWheelState.buttons & GameInputRacingWheelView)
                {
                    m_buttonString += L"[View] ";
                }

                m_wheelValue = racingWheelState.wheel;
                m_throttleValue = racingWheelState.throttle;
                m_brakeValue = racingWheelState.brake;
                m_clutchValue = racingWheelState.clutch;
                m_handbrakeValue = racingWheelState.handbrake;
                m_patternShifterGearValue = racingWheelState.patternShifterGear;
            }

            ComPtr<IGameInputRawDeviceReport> state;

            if (m_lastReading->GetRawReport(&state))
            {
                //This is to be sent in response to the Initial Reports Request
                //
                //Byte      Raw Byte    Description                 Value

                //Header
                //0x00                  Wheel Static Configuration  0x21
                //0x01                  Flags                       0x00
                //0x02                  Sequence ID                 incrementing
                //0x03                  Length                      >= 0x0B

                //Raw Data
                //0x04      0x0         Angle Precision             variable
                //0x05      0x1         Throttle Precision          variable
                //0x06      0x2         Brake Precision             variable
                //0x07      0x3         Clutch Precision            variable
                //0x08      0x4         Handbrake Precision         variable
                //0x09      0x5         Minimum Angle Setting       variable (2 bytes)
                //0x0B      0x7         Maximum Angle Setting       variable (2 bytes)
                //0x0D      0x9         Force - Feedback Axis mask  variable

                size_t bufferSize = state->GetRawDataSize();

                //Ensure the message is at least as large as needed and that it is a Wheel Static Configuration message
                if (state->GetReportInfo()->id == 0x21 && bufferSize >= 0x0A)
                {
                    auto rawData = std::make_unique<uint8_t[]>(bufferSize);

                    size_t readSize = state->GetRawData(bufferSize, rawData.get());

                    //Ensure we read the whole buffer
                    if (readSize >= bufferSize)
                    {
                        //Print raw data
                        swprintf_s(m_rawMessageString, 28, L"%02X %02X %02X %02X %02X %02X%02X %02X%02X %02X",
                            rawData[0x0], rawData[0x1], rawData[0x2], rawData[0x3], rawData[0x4],
                            rawData[0x5], rawData[0x6], rawData[0x7], rawData[0x8], rawData[0x9]);
                    }
                    else
                    {
                        wcscpy_s(m_rawMessageString, L"Unable to read data");
                    }
                }
            }
        }
    }
}

void RawWheel::RequestState()
{
    //This request is sent to instruct the wheel to report back the static configuration and variable state via
    //the Wheel Static Configuration Report and the Wheel State Report.
    //
    //Offset   Value         Description
    //0x00     0x0A          GIP Message Identifier
    //0x01     0x00          Flags
    //0x02     incrementing  Sequence ID
    //0x03     0x03          Payload Length
    //0x04     0x00          Initial Reports Request
    //0x05     0x0000        Request does not require additional data - set to 0 (2 bytes)

    uint8_t messagePayload[] = { 0x00, m_currentSequence, 0x03, 0x00, 0x00, 0x00 };

    ComPtr<IGameInputRawDeviceReport> rawReport;
    HRESULT hr = m_device->CreateRawDeviceReport(0x0A, GameInputRawOutputReport, &rawReport);

    if (SUCCEEDED(hr))
    {
        rawReport->SetRawData(sizeof(messagePayload), messagePayload);
        hr = m_device->SendRawDeviceOutput(rawReport.Get());
        m_currentSequence++;
    }
}

