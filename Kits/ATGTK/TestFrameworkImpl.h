//--------------------------------------------------------------------------------------
// File: ATGTestFramework.h
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright(c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <vector>
#include <queue>
#include <map>

#include "GamePad.h"
#include "Json.h"
#include "ScreenGrab.h"
#include "DirectXHelpers.h"

#include <XSystem.h>
#include <fstream>
#include <wincodec.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// These need to match the values in the Automation code
constexpr wchar_t TestArtifactsDirW[] = L"TestArtifacts";
constexpr char    TestConfigFilename[] = "TestConfig.json";

namespace ATG
{
    class ATGTestGamePad
    {
    public:
        ATGTestGamePad() noexcept(false) {}
        ~ATGTestGamePad() {}

        ATGTestGamePad(ATGTestGamePad&&) = default;
        ATGTestGamePad& operator= (ATGTestGamePad&&) = default;

        ATGTestGamePad(ATGTestGamePad const&) = delete;
        ATGTestGamePad& operator= (ATGTestGamePad const&) = delete;


        void SetState(DirectX::GamePad::State state) { m_gamePadState = state; };
        DirectX::GamePad::State GetState(int player, DirectX::GamePad::DeadZone deadZoneMode = DirectX::GamePad::DeadZone::DEAD_ZONE_INDEPENDENT_AXES)
        {
            UNREFERENCED_PARAMETER(player);
            UNREFERENCED_PARAMETER(deadZoneMode);
            return m_gamePadState;
        }

    private:
        DirectX::GamePad::State m_gamePadState { };
    };
};

using ATGGamePad = ATG::ATGTestGamePad;

namespace ATG
{
    class TestFramework
    {
        public:
            TestFramework() noexcept(false) :
                m_frameCount(0)
            {
                m_gamePadState.connected = true;
            }
            virtual ~TestFramework() = default;

            TestFramework(TestFramework&&) = default;
            TestFramework& operator= (TestFramework&&) = default;

            TestFramework(TestFramework const&) = delete;
            TestFramework& operator= (TestFramework const&) = delete;

            virtual ID3D12CommandQueue* GetCommandQueue() = 0;
            virtual ID3D12Resource*     GetRenderTarget() = 0;
            virtual ATGGamePad*         GetGamePad() = 0;

            void TestInitialize()
            {
                if (!ParseTestConfig())
                {
                    return;
                }

                double rate = 0;
                HRESULT hr = GetRefreshRate(&rate);
                if (SUCCEEDED(hr))
                {
                    // round up to nearest whole number
                    rate += 0.5f;
                    m_refreshRate = static_cast<uint32_t>(rate);
                    if (m_refreshRate != 60)
                    {
                        Log("Monitor refresh set at %dHz, interpolating to 60Hz\n", m_refreshRate);
                    }
                }
                else
                {
                    Log("Failed to get monitor refresh, assuming 60Hz: %08X\n", hr);
                    m_refreshRate = 60;
                }

                if (!CreateDirectory(TestArtifactsDirW, nullptr))
                {
                    DWORD err = GetLastError();
                    if (err != ERROR_ALREADY_EXISTS)
                    {
                        throw std::exception("Failed to create test artifacts directory: %08X\n", HRESULT_FROM_WIN32(err));
                    }
                }
            }

            void TestTick()
            {
                if (!IsTestEnabled())
                {
                    return;
                }

                HRESULT hr = S_OK;

                m_frameCount++;

                // interpolate frame count, if required
                float frameCount60 = static_cast<float>(m_frameCount) / (m_refreshRate / 60.0f);

                // release any Press'ed keys from last frame
                if (!m_needsRelease.empty())
                {
                    for (auto& nr : m_needsRelease)
                    {
                        Log("Releasing gamepad state: %d %d\n", frameCount60, nr);
                        SetGamePadStateForEvent(&m_gamePadState, nr, InputState::Release);
                    }

                    m_needsRelease.clear();
                }

                // if there are no more events, we're done
                if (m_testEvents.empty())
                {
                    return;
                }

                // get the first event
                auto& te = m_testEvents.front();

                // if it's this frame, handle it
                while (static_cast<uint32_t>(frameCount60) == te.Frame)
                {
                    switch (te.Action)
                    {
                    case TestAction::Input:
                        for (auto& input : te.InputParams.Inputs)
                        {
                            Log("Setting gamepad state: %d %d %d\n", frameCount60, input, te.InputParams.Action);
                            SetGamePadStateForEvent(&m_gamePadState, input, te.InputParams.Action);
                        }

                        GetGamePad()->SetState(m_gamePadState);

                        break;

                    case TestAction::Screenshot:
                        wchar_t filename[256];
                        swprintf(filename, 256, L"%ws\\%hs", TestArtifactsDirW, te.ScreenshotParams.Filename.c_str());

                        Log("Saving screenshot to `%ws`\n", filename);
                        hr = DirectX::SaveWICTextureToFile(GetCommandQueue(), GetRenderTarget(), GUID_ContainerFormatPng,
                            filename, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT, nullptr, nullptr, true);
                        if(FAILED(hr))
                        {
                            throw std::exception("Could not take screenshot");
                        }
                        break;

                    default:
                        throw std::exception("Unknown Action type");
                    }

                    // get the next event (if any) and loop
                    m_testEvents.pop();
                    if (m_testEvents.empty())
                    {
                        return;
                    }

                    te = m_testEvents.front();
                }
            }

            bool IsTestEnabled() const { return !m_testEvents.empty(); }

        private:
            enum InputType
            {
                A = 1,B,X,Y,
                LB,RB,
                View,Menu,
                LS,RS,
                Up,Down,Left,Right
            };

            enum InputState
            {
                Press = 1,
                Hold,
                Release,
            };

            enum TestAction
            {
                Input = 1,
                Screenshot,
            };

            struct InputParams
            {
                std::vector<InputType> Inputs;
                InputState Action;
            };

            struct ScreenshotParams
            {
                std::string Filename;
            };

            struct TestEvent
            {
                uint32_t Frame;
                TestAction Action;
                InputParams InputParams;
                ScreenshotParams ScreenshotParams;
            };

            const std::map<std::string, InputType> m_inputTypeMap =
            {
                { "A",     InputType::A },
                { "B",     InputType::B },
                { "X",     InputType::X },
                { "Y",     InputType::Y },
                { "LB",    InputType::LB },
                { "RB",    InputType::RB },
                { "Menu",  InputType::Menu },
                { "View",  InputType::View },
                { "LS",    InputType::LS },
                { "RS",    InputType::RS },
                { "Up",    InputType::Up },
                { "Down",  InputType::Down },
                { "Left",  InputType::Left },
                { "Right", InputType::Right },
            };

            const std::map<std::string, InputState> m_inputStateMap =
            {
                { "Press",   InputState::Press },
                { "Hold",    InputState::Hold },
                { "Release", InputState::Release },
            };

            const std::map<std::string, TestAction> m_testActionMap =
            {
                { "Input",      TestAction::Input },
                { "Screenshot", TestAction::Screenshot },
            };

            DirectX::GamePad::State m_gamePadState {};
            uint64_t m_frameCount;
            uint32_t m_refreshRate;

            std::queue<TestEvent> m_testEvents {};
            std::vector<InputType> m_needsRelease {};

            void SetGamePadStateForEvent(DirectX::GamePad::State* state, InputType input, InputState action)
            {
                bool pressed = (action == InputState::Press || action == InputState::Hold);

                switch (input)
                {
                case InputType::A:
                    state->buttons.a = pressed;
                    break;
                case InputType::B:
                    state->buttons.b = pressed;
                    break;
                case InputType::X:
                    state->buttons.x = pressed;
                    break;
                case InputType::Y:
                    state->buttons.y = pressed;
                    break;
                case InputType::LB:
                    state->buttons.leftShoulder = pressed;
                    break;
                case InputType::RB:
                    state->buttons.rightShoulder = pressed;
                    break;
                case InputType::View:
                    state->buttons.view = pressed;
                    break;
                case InputType::Menu:
                    state->buttons.menu = pressed;
                    break;
                case InputType::LS:
                    state->buttons.leftStick = pressed;
                    break;
                case InputType::RS:
                    state->buttons.rightStick = pressed;
                    break;
                case InputType::Up:
                    state->dpad.up = pressed;
                    break;
                case InputType::Down:
                    state->dpad.down = pressed;
                    break;
                case InputType::Left:
                    state->dpad.left = pressed;
                    break;
                case InputType::Right:
                    state->dpad.right = pressed;
                    break;
                default:
                    throw std::exception("Unrecognized input type specified");
                }

                if (action == InputState::Press)
                {
                    m_needsRelease.push_back(input);
                }
            }

            InputType StringToInputType(std::string input)
            {
                return m_inputTypeMap.at(input);
            }

            InputState StringToInputState(std::string state)
            {
                return m_inputStateMap.at(state);
            }

            TestAction StringToTestAction(std::string action)
            {
                return m_testActionMap.at(action);
            }

            bool ParseTestConfig()
            {
                // TODO: PC testing is disabled for now
                XSystemDeviceType device = XSystemGetDeviceType();
                //if(device == XSystemDeviceType::Pc && !IsDebuggerPresent())
                //    return false;

                std::ifstream f(TestConfigFilename);
                if (f.good())
                {
                    Log("Test script '%s' found\n", TestConfigFilename);

                    json jsonData = json::parse(f, nullptr, true, true);
                    json excluded = jsonData["Configuration"]["ExcludedPlatforms"];
                    if (excluded != nullptr)
                    {
                        if (!excluded.is_array())
                        {
                            throw new std::exception("Configuration.ExcludedPlatforms is not an array");
                        }

                        for (auto& e : excluded)
                        {
                            if ((e == "Desktop" && device == XSystemDeviceType::Pc) ||
                                (e == "XboxOne" && (device == XSystemDeviceType::XboxOne ||
                                    device == XSystemDeviceType::XboxOneS ||
                                    device == XSystemDeviceType::XboxOneX ||
                                    device == XSystemDeviceType::XboxOneXDevkit)) ||
                                (e == "Scarlett" && (device == XSystemDeviceType::XboxScarlettLockhart ||
                                    device == XSystemDeviceType::XboxScarlettAnaconda ||
                                    device == XSystemDeviceType::XboxScarlettDevkit)))
                            {
                                Log("Platform excluded, no tests will be run");
                                return false;
                            }
                        }
                    }

                    Log("Platform not excluded, continuing...\n");

                    json testActions = jsonData["TestActions"];
                    if (!testActions.is_array())
                    {
                        throw std::exception("TestActions is not an array");
                    }

                    for (auto& te : testActions)
                    {
                        if (!te.is_object())
                        {
                            throw std::exception("TestAction is not an object");
                        }

                        TestEvent e{};
                        e.Frame = te["Frame"].get<uint32_t>();
                        e.Action = StringToTestAction(te["Action"].get<std::string>());

                        switch (e.Action)
                        {
                        case TestAction::Input:
                        {
                            InputState ia = StringToInputState(te["InputParams"]["State"]);
                            auto& inputs = te["InputParams"]["Inputs"];

                            if (!inputs.is_array())
                            {
                                throw std::exception("InputParams.InputEvents is not an array or is missing");
                            }

                            for (auto& input : inputs)
                            {
                                e.InputParams.Inputs.push_back(StringToInputType(input));
                                e.InputParams.Action = ia;
                            }
                        }
                        break;

                        case TestAction::Screenshot:
                        {
                            auto& ssp = te["ScreenshotParams"];
                            e.ScreenshotParams.Filename = ssp["Filename"].get<std::string>();
                        }
                        break;

                        default:
                            throw std::exception("Unrecognized TestType");
                        }

                        m_testEvents.push(e);
                    }
                    return true;
                }
                return false;
            }

            HRESULT GetRefreshRate(double* outRefreshRate)
            {
                HRESULT hr = S_OK;

#ifdef _GAMING_DESKTOP
                if (outRefreshRate == nullptr)
                {
                    return E_INVALIDARG;
                }

                DEVMODE dm{};
                dm.dmSize = sizeof(dm);

                if(EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm))
                {
                    *outRefreshRate = dm.dmDisplayFrequency;
                }
#else
                *outRefreshRate = 60.0f;
#endif
                return hr;
            }

            void Log(const char* format, ...)
            {
                char msg[1024]{};

                va_list arglist;
                va_start(arglist, format);
                vsnprintf_s(msg, std::size(msg), format, arglist);
                va_end(arglist);

                OutputDebugStringA("ATGAUTO: ");
                OutputDebugStringA(msg);
            }
    };
}
