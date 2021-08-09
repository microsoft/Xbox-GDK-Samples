//--------------------------------------------------------------------------------------
// GameInputInterfacing.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameInputInterfacing.h"

#include "ATGColors.h"

extern void ExitSample();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

static void CALLBACK OnGameInputDeviceAddedRemoved(
    _In_ GameInputCallbackToken,
    _In_ void * context,
    _In_ IGameInputDevice * device,
    _In_ uint64_t,
    _In_ GameInputDeviceStatus currentStatus,
    _In_ GameInputDeviceStatus) noexcept
{
    auto sample = reinterpret_cast<Sample*>(context);

    if (currentStatus & GameInputDeviceConnected)
    {
        size_t i = 0;

        while (i < c_maxDevices)
        {
            if (!sample->m_devices[i].device)
            {
                sample->m_devices[i].device = device;
                break;
            }

            i++;
        }
    }
    else
    {
        for (size_t k = 0; k < c_maxDevices; ++k)
        {
            if (sample->m_devices[k].device.Get() == device)
            {
                sample->m_devices[k].needDelete = true;
                break;
            }
        }
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_deviceToken(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    DX::ThrowIfFailed(GameInputCreate(&m_gameInput));
    DX::ThrowIfFailed(m_gameInput->RegisterDeviceCallback(nullptr, GameInputKindAny, GameInputDeviceConnected, GameInputBlockingEnumeration, this, OnGameInputDeviceAddedRemoved, &m_deviceToken));

    m_uiManager.GetRootElement()->AddChildFromLayout("Assets/layout.json");
}

Sample::~Sample()
{
    if (m_deviceToken)
    {
        if (m_gameInput)
        {
            (void)m_gameInput->UnregisterCallback(m_deviceToken, UINT64_MAX);
        }

        m_deviceToken = 0;
    }
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    for (int i = 0; i < c_maxDevices; i++)
    {
        if (m_devices[i].needDelete)
        {
            m_uiManager.Detach(m_devices[i].deviceElement);
            m_devices[i].Reset();
        }

        if (m_devices[i].device)
        {
            UpdateDeviceUI(i);

            //Get all input types
            HRESULT hr = m_gameInput->GetCurrentReading(GameInputKindAny, m_devices[i].device.Get(), &m_reading);

            if (hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            {
                DX::ThrowIfFailed(hr);
                uint32_t inputCount;
                uint32_t readCount;
                UIElementPtr currentElement;

                //Controller axis
                inputCount = m_reading->GetControllerAxisCount();
                if (inputCount > 0)
                {
                    currentElement = GetAxisUI(i);

                    auto axisReading = std::make_unique<float[]>(inputCount);
                    readCount = m_reading->GetControllerAxisState(inputCount, axisReading.get());

                    std::string currentIndex;
                    uint32_t k = 0;
                    while (k < 16)
                    {
                        currentIndex = std::to_string(k + 1);
                        if (k < readCount)
                        {
                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Axis" + currentIndex))->
                                SetDisplayText(currentIndex + ": " + std::to_string(axisReading[k]));
                        }
                        else
                        {
                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Axis" + currentIndex))->
                                SetDisplayText("");
                        }
                        k++;
                    }
                }

                //Controller buttons
                inputCount = m_reading->GetControllerButtonCount();
                if (inputCount > 0)
                {
                    currentElement = GetButtonsUI(i);

                    auto buttonReading = std::make_unique<bool[]>(inputCount);
                    readCount = m_reading->GetControllerButtonState(inputCount, buttonReading.get());

                    std::string currentIndex;
                    uint32_t k = 0;
                    while (k < 21)
                    {
                        currentIndex = std::to_string(k + 1);
                        if (k < readCount)
                        {
                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Button" + currentIndex))->
                                SetDisplayText(currentIndex + ": " + std::to_string(buttonReading[k]));
                        }
                        else
                        {
                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Button" + currentIndex))->
                                SetDisplayText("");
                        }
                        k++;
                    }
                }

                //Controller switches
                inputCount = m_reading->GetControllerSwitchCount();
                if (inputCount > 0)
                {
                    currentElement = GetSwitchesUI(i);

                    auto switchReading = std::make_unique<GameInputSwitchPosition[]>(inputCount);
                    readCount = m_reading->GetControllerSwitchState(inputCount, switchReading.get());

                    std::string switchString;
                    currentElement = GetSwitchesUI(i);

                    std::string currentIndex;
                    uint32_t k = 0;
                    while (k < 7)
                    {
                        currentIndex = std::to_string(k + 1);
                        if (k < readCount)
                        {
                            switchString = std::to_string(k) + ": ";
                            switch (switchReading[k])
                            {
                            case GameInputSwitchCenter:
                                switchString += "Center";
                                break;
                            case GameInputSwitchUp:
                                switchString += "Up";
                                break;
                            case GameInputSwitchUpRight:
                                switchString += "Up Right";
                                break;
                            case GameInputSwitchRight:
                                switchString += "Right";
                                break;
                            case GameInputSwitchDownRight:
                                switchString += "Down Right";
                                break;
                            case GameInputSwitchDown:
                                switchString += "Down";
                                break;
                            case GameInputSwitchDownLeft:
                                switchString += "Down Left";
                                break;
                            case GameInputSwitchLeft:
                                switchString += "Left";
                                break;
                            case GameInputSwitchUpLeft:
                                switchString += "Up Left";
                                break;
                            }

                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + currentIndex))->SetDisplayText(switchString);
                        }
                        else
                        {
                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + currentIndex))->SetDisplayText("");
                        }

                        k++;
                    }
                }

                //Key states
                inputCount = m_reading->GetKeyCount();
                if (inputCount > 0)
                {
                    currentElement = GetKeysUI(i);

                    auto keyReading = std::make_unique<GameInputKeyState[]>(inputCount);
                    readCount = m_reading->GetKeyState(inputCount, keyReading.get());

                    std::string currentIndex;
                    uint32_t k = 0;
                    while (k < 7)
                    {
                        currentIndex = std::to_string(k + 1);
                        if (k < readCount)
                        {
                            std::stringstream ss;
                            ss << "Scan: " << std::hex << keyReading[k].scanCode;
                            ss << " CodePoint: " << std::hex << keyReading[k].codePoint;
                            ss << " VKey: " << std::hex << keyReading[k].virtualKey;

                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + currentIndex))->SetDisplayText(ss.str());
                        }
                        else
                        {
                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + currentIndex))->SetDisplayText("");
                        }
                        k++;
                    }
                }

                //Touch states
                //PC ONLY
                inputCount = m_reading->GetTouchCount();
                if (inputCount > 0)
                {
                    currentElement = GetTouchUI(i);

                    auto touchReading = std::make_unique<GameInputTouchState[]>(inputCount);
                    readCount = m_reading->GetTouchState(inputCount, touchReading.get());

                    std::string currentIndex;
                    uint32_t k = 0;
                    while (k < 7)
                    {
                        currentIndex = std::to_string(k + 1);
                        if (k < readCount)
                        {
                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + currentIndex))
                                ->SetDisplayText(currentIndex + ": X " + std::to_string(touchReading[k].positionX) +
                                    " Y " + std::to_string(touchReading[k].positionY) +
                                    " Pres " + std::to_string(touchReading[k].pressure) +
                                    " Prox " + std::to_string(touchReading[k].proximity));
                        }
                        else
                        {
                            currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + currentIndex))->SetDisplayText("");
                        }
                        k++;
                    }
                }

                //Mouse state
                GameInputMouseState mouseState;
                if (m_reading->GetMouseState(&mouseState))
                {
                    std::string mouseString;
                    currentElement = GetMouseUI(i);

                    if (mouseState.buttons & GameInputMouseLeftButton)
                    {
                        mouseString += "Left ";
                    }
                    if (mouseState.buttons & GameInputMouseRightButton)
                    {
                        mouseString += "Right ";
                    }
                    if (mouseState.buttons & GameInputMouseMiddleButton)
                    {
                        mouseString += "Middle ";
                    }
                    if (mouseState.buttons & GameInputMouseButton4)
                    {
                        mouseString += "Button4 ";
                    }
                    if (mouseState.buttons & GameInputMouseButton5)
                    {
                        mouseString += "Button5 ";
                    }
                    if (mouseState.buttons & GameInputMouseWheelTiltLeft)
                    {
                        mouseString += "WheelTiltLeft ";
                    }
                    if (mouseState.buttons & GameInputMouseWheelTiltRight)
                    {
                        mouseString += "WheelTiltRight ";
                    }

                    //These are the cumulative deltas of raw mouse movement
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Inputs"))->SetDisplayText(mouseString);
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("X Value"))->SetDisplayText(std::to_string(mouseState.positionX));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Y Value"))->SetDisplayText(std::to_string(mouseState.positionY));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("WheelX Value"))->SetDisplayText(std::to_string(mouseState.wheelX));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("WheelY Value"))->SetDisplayText(std::to_string(mouseState.wheelY));
                }

                //Motion states
                //PC ONLY
                GameInputMotionState motionState;
                if (m_reading->GetMotionState(&motionState))
                {
                    currentElement = GetMotionUI(i);

                    switch (motionState.magnetometerAccuracy)
                    {
                    case GameInputMotionAccuracyUnknown:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Mag Acc"))->SetDisplayText("MagnetometerAccuracy: Unknown");
                        break;
                    case GameInputMotionUnavailable:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Mag Acc"))->SetDisplayText("MagnetometerAccuracy: Unavailable");
                        break;
                    case GameInputMotionUnreliable:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Mag Acc"))->SetDisplayText("MagnetometerAccuracy: Unreliable");
                        break;
                    case GameInputMotionApproximate:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Mag Acc"))->SetDisplayText("MagnetometerAccuracy: Approximate");
                        break;
                    case GameInputMotionAccurate:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Mag Acc"))->SetDisplayText("MagnetometerAccuracy: Accurate");
                        break;
                    }

                    switch (motionState.orientationAccuracy)
                    {
                    case GameInputMotionAccuracyUnknown:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Or Acc"))->SetDisplayText("OrientationAccuracy: Unknown");
                        break;
                    case GameInputMotionUnavailable:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Or Acc"))->SetDisplayText("OrientationAccuracy: Unavailable");
                        break;
                    case GameInputMotionUnreliable:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Or Acc"))->SetDisplayText("OrientationAccuracy: Unreliable");
                        break;
                    case GameInputMotionApproximate:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Or Acc"))->SetDisplayText("OrientationAccuracy: Approximate");
                        break;
                    case GameInputMotionAccurate:
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Or Acc"))->SetDisplayText("OrientationAccuracy: Accurate");
                        break;
                    }

                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Acc"))->SetDisplayText("Acc: (" +
                        std::to_string(motionState.accelerationX) + "," +
                        std::to_string(motionState.accelerationY) + "," +
                        std::to_string(motionState.accelerationZ) + ")");

                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Ang"))->SetDisplayText("Ang: (" +
                        std::to_string(motionState.angularVelocityX) + "," +
                        std::to_string(motionState.angularVelocityY) + "," +
                        std::to_string(motionState.angularVelocityZ) + ")");

                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Mag"))->SetDisplayText("Mag: (" +
                        std::to_string(motionState.magneticFieldX) + "," +
                        std::to_string(motionState.magneticFieldY) + "," +
                        std::to_string(motionState.magneticFieldZ) + ")");

                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Motion1"))->SetDisplayText("Motion: (" +
                        std::to_string(motionState.orientationX) + "," +
                        std::to_string(motionState.orientationY) + ",");

                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Motion2"))->SetDisplayText(
                        std::to_string(motionState.orientationZ) + "," +
                        std::to_string(motionState.orientationW) + ")");
                }

                //Arcade stick states
                GameInputArcadeStickState arcadeState;
                if (m_reading->GetArcadeStickState(&arcadeState))
                {
                    int currentIndex = 1;
                    currentElement = GetArcadeUI(i);

                    if (arcadeState.buttons & GameInputArcadeStickMenu)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Menu");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickView)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("View");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickUp)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Up");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickDown)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Down");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickLeft)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Left");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickRight)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Right");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickAction1)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Action1");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickAction2)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Action2");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickAction3)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Action3");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickAction4)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Action4");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickAction5)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Action5");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickAction6)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Action6");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickSpecial1)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Special1");
                        currentIndex++;
                    }
                    if (arcadeState.buttons & GameInputArcadeStickSpecial2)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Special2");
                        currentIndex++;
                    }

                    while (currentIndex < 15)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("");
                        currentIndex++;
                    }
                }

                //Flight stick states
                GameInputFlightStickState flightStickState;
                if (m_reading->GetFlightStickState(&flightStickState))
                {
                    std::string flightStickString;
                    currentElement = GetFlightStickUI(i);

                    if (flightStickState.buttons & GameInputFlightStickMenu)
                    {
                        flightStickString += "Menu ";
                    }
                    if (flightStickState.buttons & GameInputFlightStickView)
                    {
                        flightStickString += "View ";
                    }
                    if (flightStickState.buttons & GameInputFlightStickFirePrimary)
                    {
                        flightStickString += "Primary ";
                    }
                    if (flightStickState.buttons & GameInputFlightStickFireSecondary)
                    {
                        flightStickString += "Secondary ";
                    }

                    switch (flightStickState.hatSwitch)
                    {
                    case GameInputSwitchCenter:
                        flightStickString += "Center ";
                        break;
                    case GameInputSwitchUp:
                        flightStickString += "Up ";
                        break;
                    case GameInputSwitchUpRight:
                        flightStickString += "Up Right ";
                        break;
                    case GameInputSwitchRight:
                        flightStickString += "Right ";
                        break;
                    case GameInputSwitchDownRight:
                        flightStickString += "Down Right ";
                        break;
                    case GameInputSwitchDown:
                        flightStickString += "Down ";
                        break;
                    case GameInputSwitchDownLeft:
                        flightStickString += "Down Left ";
                        break;
                    case GameInputSwitchLeft:
                        flightStickString += "Left ";
                        break;
                    case GameInputSwitchUpLeft:
                        flightStickString += "Up Left ";
                        break;
                    }

                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Inputs"))->SetDisplayText(flightStickString);
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Roll Value"))->SetDisplayText(std::to_string(flightStickState.roll));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Pitch Value"))->SetDisplayText(std::to_string(flightStickState.pitch));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Yaw Value"))->SetDisplayText(std::to_string(flightStickState.yaw));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Throttle Value"))->SetDisplayText(std::to_string(flightStickState.throttle));
                }

                //Racing wheel states
                GameInputRacingWheelState racingWheelstate;
                if (m_reading->GetRacingWheelState(&racingWheelstate))
                {
                    std::string racingWheelString;
                    currentElement = GetWheelUI(i);

                    if (racingWheelstate.buttons & GameInputRacingWheelMenu)
                    {
                        racingWheelString += "Menu ";
                    }
                    if (racingWheelstate.buttons & GameInputRacingWheelView)
                    {
                        racingWheelString += "View ";
                    }
                    if (racingWheelstate.buttons & GameInputRacingWheelPreviousGear)
                    {
                        racingWheelString += "PrevGear ";
                    }
                    if (racingWheelstate.buttons & GameInputRacingWheelNextGear)
                    {
                        racingWheelString += "NextGear ";
                    }
                    if (racingWheelstate.buttons & GameInputRacingWheelDpadUp)
                    {
                        racingWheelString += "Up ";
                    }
                    if (racingWheelstate.buttons & GameInputRacingWheelDpadDown)
                    {
                        racingWheelString += "Down ";
                    }
                    if (racingWheelstate.buttons & GameInputRacingWheelDpadLeft)
                    {
                        racingWheelString += "Left ";
                    }
                    if (racingWheelstate.buttons & GameInputRacingWheelDpadRight)
                    {
                        racingWheelString += "Right ";
                    }

                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Inputs"))->SetDisplayText(racingWheelString);
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Wheel Value"))->SetDisplayText(std::to_string(racingWheelstate.wheel));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Throttle Value"))->SetDisplayText(std::to_string(racingWheelstate.throttle));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Brake Value"))->SetDisplayText(std::to_string(racingWheelstate.brake));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Clutch Value"))->SetDisplayText(std::to_string(racingWheelstate.clutch));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("PatternShifterGear Value"))->SetDisplayText(std::to_string(racingWheelstate.patternShifterGear));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Handbrake Value"))->SetDisplayText(std::to_string(racingWheelstate.handbrake));
                }

                //UI navigation states
                GameInputUiNavigationState uiNavigationstate;
                if (m_reading->GetUiNavigationState(&uiNavigationstate))
                {
                    int currentIndex = 1;
                    currentElement = GetUINavigationUI(i);

                    if (uiNavigationstate.buttons & GameInputUiNavigationMenu)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Menu");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationView)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("View");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationAccept)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Accept");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationCancel)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Cancel");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationUp)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Up");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationDown)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Down");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationLeft)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Left");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationRight)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Right");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationContext1)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Context1");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationContext2)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Context2");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationContext3)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Context3");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationContext4)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("Context4");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationPageUp)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("PageUp");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationPageDown)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("PageDown");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationPageLeft)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("PageLeft");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationPageRight)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("PageRight");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationScrollUp)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("ScrollUp");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationScrollDown)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("ScrollDown");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationScrollLeft)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("ScrollLeft");
                        currentIndex++;
                    }
                    if (uiNavigationstate.buttons & GameInputUiNavigationScrollRight)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("ScrollRight");
                        currentIndex++;
                    }

                    while (currentIndex < 9)
                    {
                        currentElement->GetTypedSubElementById<UIStaticText>(ID("Input" + std::to_string(currentIndex)))->SetDisplayText("");
                        currentIndex++;
                    }
                }

                //Gamepad states
                GameInputGamepadState gamepadState;
                if (m_reading->GetGamepadState(&gamepadState))
                {
                    int exitComboPressed = 0;
                    currentElement = GetGamepadUI(i);
                    std::string gamepadString;

                    if (gamepadState.buttons & GameInputGamepadDPadUp)
                    {
                        gamepadString += "U[Dpad]";
                    }
                    if (gamepadState.buttons & GameInputGamepadDPadDown)
                    {
                        gamepadString += "D[Dpad]";
                    }
                    if (gamepadState.buttons & GameInputGamepadDPadRight)
                    {
                        gamepadString += "R[Dpad]";
                    }
                    if (gamepadState.buttons & GameInputGamepadDPadLeft)
                    {
                        gamepadString += "L[Dpad]";
                    }
                    if (gamepadState.buttons & GameInputGamepadA)
                    {
                        gamepadString += "[A]";
                    }
                    if (gamepadState.buttons & GameInputGamepadB)
                    {
                        gamepadString += "[B]";
                    }
                    if (gamepadState.buttons & GameInputGamepadX)
                    {
                        gamepadString += "[X]";
                    }
                    if (gamepadState.buttons & GameInputGamepadY)
                    {
                        gamepadString += "[Y]";
                    }
                    if (gamepadState.buttons & GameInputGamepadLeftShoulder)
                    {
                        gamepadString += "[LB]";
                        exitComboPressed += 1;
                    }
                    if (gamepadState.buttons & GameInputGamepadRightShoulder)
                    {
                        gamepadString += "[RB]";
                        exitComboPressed += 1;
                    }
                    if (gamepadState.buttons & GameInputGamepadLeftThumbstick)
                    {
                        gamepadString += "[LThumb]";
                    }
                    if (gamepadState.buttons & GameInputGamepadRightThumbstick)
                    {
                        gamepadString += "[RThumb]";
                    }
                    if (gamepadState.buttons & GameInputGamepadMenu)
                    {
                        gamepadString += "[Menu]";
                        exitComboPressed += 1;
                    }
                    if (gamepadState.buttons & GameInputGamepadView)
                    {
                        gamepadString += "[View]";
                        exitComboPressed += 1;
                    }

                    currentElement->GetTypedSubElementById<UIStaticText>(ID("Inputs"))->SetDisplayText(gamepadString);
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("LT Value"))->SetDisplayText(std::to_string(gamepadState.leftTrigger));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("RT Value"))->SetDisplayText(std::to_string(gamepadState.rightTrigger));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("LeftX Value"))->SetDisplayText(std::to_string(gamepadState.leftThumbstickX));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("LeftY Value"))->SetDisplayText(std::to_string(gamepadState.leftThumbstickY));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("RightX Value"))->SetDisplayText(std::to_string(gamepadState.rightThumbstickX));
                    currentElement->GetTypedSubElementById<UIStaticText>(ID("RightY Value"))->SetDisplayText(std::to_string(gamepadState.rightThumbstickY));

                    if (exitComboPressed == 4)
                        ExitSample();
                }
            }
        }
    }

    m_uiManager.Update((float)timer.GetElapsedSeconds(), UIInputState());
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    m_uiManager.Render();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // create the style renderer for the UI manager to use for rendering the UI scene styles
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // notify the UI manager of the current window size
    auto os = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(os.right, os.bottom);
}
#pragma endregion

UIElementPtr Sample::GetArcadeUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Arcade])
    {
        m_devices[index].elements[InputTypes::Arcade] = m_uiManager.InstantiatePrefab("Assets/arcade.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Arcade], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Arcade];
}

UIElementPtr Sample::GetAxisUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Axis])
    {
        m_devices[index].elements[InputTypes::Axis] = m_uiManager.InstantiatePrefab("Assets/axis.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Axis], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Axis];
}

UIElementPtr Sample::GetButtonsUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Buttons])
    {
        m_devices[index].elements[InputTypes::Buttons] = m_uiManager.InstantiatePrefab("Assets/buttons.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Buttons], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Buttons];
}

UIElementPtr Sample::GetFlightStickUI(int index)
{
    if (!m_devices[index].elements[InputTypes::FlightStick])
    {
        m_devices[index].elements[InputTypes::FlightStick] = m_uiManager.InstantiatePrefab("Assets/flightstick.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::FlightStick], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::FlightStick];
}

UIElementPtr Sample::GetGamepadUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Gamepad])
    {
        m_devices[index].elements[InputTypes::Gamepad] = m_uiManager.InstantiatePrefab("Assets/gamepad.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Gamepad], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Gamepad];
}

UIElementPtr Sample::GetKeysUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Keys])
    {
        m_devices[index].elements[InputTypes::Keys] = m_uiManager.InstantiatePrefab("Assets/keys.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Keys], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Keys];
}

UIElementPtr Sample::GetMotionUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Motion])
    {
        m_devices[index].elements[InputTypes::Motion] = m_uiManager.InstantiatePrefab("Assets/motion.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Motion], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Motion];
}

UIElementPtr Sample::GetMouseUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Mouse])
    {
        m_devices[index].elements[InputTypes::Mouse] = m_uiManager.InstantiatePrefab("Assets/mouse.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Mouse], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Mouse];
}

UIElementPtr Sample::GetSwitchesUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Switches])
    {
        m_devices[index].elements[InputTypes::Switches] = m_uiManager.InstantiatePrefab("Assets/switches.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Switches], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Switches];
}

UIElementPtr Sample::GetTouchUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Touch])
    {
        m_devices[index].elements[InputTypes::Touch] = m_uiManager.InstantiatePrefab("Assets/touch.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Touch], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Touch];
}

UIElementPtr Sample::GetUINavigationUI(int index)
{
    if (!m_devices[index].elements[InputTypes::UINavigation])
    {
        m_devices[index].elements[InputTypes::UINavigation] = m_uiManager.InstantiatePrefab("Assets/uinavigation.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::UINavigation], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::UINavigation];
}

UIElementPtr Sample::GetWheelUI(int index)
{
    if (!m_devices[index].elements[InputTypes::Wheel])
    {
        m_devices[index].elements[InputTypes::Wheel] = m_uiManager.InstantiatePrefab("Assets/wheel.json");
        m_uiManager.AttachTo(m_devices[index].elements[InputTypes::Wheel], m_devices[index].deviceElement->GetSubElementById(ID("hsp")));
    }

    return m_devices[index].elements[InputTypes::Wheel];
}

void Sample::UpdateDeviceUI(int index)
{
    if (!m_devices[index].deviceElement)
    {
        m_devices[index].deviceElement = m_uiManager.InstantiatePrefab("Assets/device.json");
        m_devices[index].deviceElement->GetTypedSubElementById<UIStaticText>(ID("Title"))->SetDisplayText("Device " + std::to_string(index + 1));
        m_uiManager.AttachTo(m_devices[index].deviceElement, m_uiManager.FindById(ID("Devices")));
    }
}
