//--------------------------------------------------------------------------------------
// Gamepad.cpp
//
// GameInput initialization, device callbacks, and frame orchestration.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Gamepad.h"

using Microsoft::WRL::ComPtr;

//--------------------------------------------------------------------------------------
// Initialize GameInput and register callbacks.
//--------------------------------------------------------------------------------------
void Sample::Initialize(HWND hWnd, ImGuiAtg::DeviceContext* deviceContext)
{
    m_hWnd = hWnd;

    HRESULT hr = GameInputCreate(&m_gameInput);

    if (FAILED(hr))
    {
#ifdef _GAMING_XBOX
        ImGuiAtg::Log(
            "GameInput creation failed with error: %08X\n\n"
            "Verify that you are using GDK 2610 or newer, or that GameInputRedist.dll "
            "from the NuGet package has deployed next to the executable for older GDK usage.",
            static_cast<unsigned int>(hr));
#else
        ImGuiAtg::Log(
            "GameInput creation failed with error: %08X\n\n"
            "Verify that the GameInput runtime has been installed that is included with "
            "the redist in the NuGet package or via 'winget install Microsoft.GameInput'",
            static_cast<unsigned int>(hr));
#endif
        return;
    }

    ImGuiAtg::Log("GameInput created successfully\n");

#ifndef _GAMING_XBOX
    // Set background input explicitly so the runtime state matches the UI at startup.
    m_gameInput->EnableBackgroundInput(m_enableBackgroundInput);
#endif

    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_perfFrequency));

    if (deviceContext)
    {
        InitializeModelRenderer(deviceContext);
    }

    hr = m_gameInput->RegisterDeviceCallback(
        nullptr,
        GameInputKindGamepad | GameInputKindSensors,
        this,
        OnDeviceChanged,
        &m_deviceCallbackToken);

    if (FAILED(hr))
    {
        ImGuiAtg::Log("Failed to register device callback: %08X\n", static_cast<unsigned int>(hr));
    }

    InitializeHaptics();
}

//--------------------------------------------------------------------------------------
// Track device connection and disconnection.
//
// Devices that are already connected when the callback is registered are reported
// asynchronously through GameInputDeviceEventConnected, the same as newly connected
// devices. GameInput device pointers have stable identity and can be compared directly.
// Static device properties are read once here and cached in GamepadDevice.
//--------------------------------------------------------------------------------------
void CALLBACK Sample::OnDeviceChanged(
    GameInputCallbackToken /*token*/,
    void* context,
    GameInputDeviceEvent deviceEvent,
    IGameInputDevice* device) noexcept
{
    auto* sample = static_cast<Sample*>(context);

    std::lock_guard<std::mutex> lock(sample->m_gamepadsMutex);

    switch (deviceEvent)
    {
        case GameInputDeviceEventConnected:
        {
            GamepadDevice newDevice = {};
            newDevice.device = device;
            newDevice.displayName = GetDeviceDisplayName(device);
            newDevice.pnpPath = GetDevicePnpPath(device);

            // Only track devices that report gamepad info.
            if (!device->GetGamepadInfo(&newDevice.deviceInfo))
            {
                ImGuiAtg::Log("Ignoring non-gamepad device: %s (VID: %04X, PID: %04X)\n", newDevice.displayName.c_str(), device->GetVendorId(), device->GetProductId());
                break;
            }

            ImGuiAtg::Log("Connected: %s (VID: %04X, PID: %04X)\n", newDevice.displayName.c_str(), device->GetVendorId(), device->GetProductId());

            sample->m_gamepads.push_back(std::move(newDevice));
            sample->m_selectedGamepad = static_cast<int>(sample->m_gamepads.size() - 1);
            break;
        }

        case GameInputDeviceEventDisconnected:
            for (auto it = sample->m_gamepads.begin(); it != sample->m_gamepads.end(); ++it)
            {
                if (it->device.Get() == device)
                {
                    ImGuiAtg::Log("Disconnected: %s (VID: %04X, PID: %04X)\n", it->displayName.c_str(), device->GetVendorId(), device->GetProductId());
                    sample->m_gamepads.erase(it);
                    break;
                }
            }
            break;

        case GameInputDeviceEventHapticInfoUpdated:
        case GameInputDeviceEventKeyboardLayoutUpdated:
        case GameInputDeviceEventXInputIndexUpdated:
            break;
    }
}

//--------------------------------------------------------------------------------------
// Update readings, vibration, lighting, and model orientation.
//--------------------------------------------------------------------------------------
void Sample::Update()
{
    if (!m_gameInput)
        return;

    std::lock_guard<std::mutex> lock(m_gamepadsMutex);

    for (auto& gamepad : m_gamepads)
    {
        // Callback mode updates state on the GameInput worker thread.
        if (!m_useCallbackMode)
        {
            PollGamepadReading(gamepad);
        }

        UpdateVibration(gamepad);
        UpdateLighting(gamepad);
    }

    // render the 3D model for the visible tab
    if (m_selectedGamepad >= 0 && m_selectedGamepad < static_cast<int>(m_gamepads.size()))
    {
        auto& gamepad = m_gamepads[static_cast<size_t>(m_selectedGamepad)];
        if (gamepad.hasSensorsState)
        {
            RenderModel(gamepad.sensorsState);
        }
    }
}

//--------------------------------------------------------------------------------------
// Draw the sample UI.
//--------------------------------------------------------------------------------------
void Sample::Draw()
{
    HandleSampleInput();

    ImGuiAtg::BeginFullscreenLayout();

    float footerH = ImGuiAtg::GetFooterHeight();
    ImGui::BeginChild("##SplitArea", ImVec2(0, ImGui::GetContentRegionAvail().y - footerH));

    ImGuiAtg::BeginSplitH("##LogSplit", 200.0f);

    if (ImGui::IsWindowAppearing())
    {
        ImGui::SetWindowFocus();
    }

    // Switch between polling and callback-driven readings.
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Reading Mode:");
        ImGui::SameLine();

        ImGuiAtg::BeginNavigationGroup("ReadingMode");

        if (ImGui::RadioButton("Polling", !m_useCallbackMode))
        {
            if (m_useCallbackMode)
            {
                UnregisterReadingCallbacks();
                m_useCallbackMode = false;
                ImGuiAtg::Log("Switched to polling mode\n");
            }
        }

        ImGui::SameLine();

        if (ImGui::RadioButton("Callback", m_useCallbackMode))
        {
            if (!m_useCallbackMode)
            {
                if (RegisterReadingCallbacks())
                {
                    m_useCallbackMode = true;
                    ImGuiAtg::Log("Switched to callback mode\n");
                }
            }
        }

        ImGuiAtg::EndNavigationGroup();
    }

#ifndef _GAMING_XBOX
    // On PC, GameInput only delivers input while the title's window has focus unless
    // background input is enabled. Console titles do not use this setting.
    {
        ImGui::SameLine(0.0f, ImGuiAtg::Scaled(30));

        ImGuiAtg::BeginNavigationGroup("BackgroundInput");

        if (ImGui::Checkbox("Enable background input", &m_enableBackgroundInput) && m_gameInput)
        {
            m_gameInput->EnableBackgroundInput(m_enableBackgroundInput);
            ImGuiAtg::Log("Background input %s\n", m_enableBackgroundInput ? "enabled" : "disabled");
        }

        ImGuiAtg::EndNavigationGroup();
    }
#endif

    {
        std::lock_guard<std::mutex> lock(m_gamepadsMutex);

        if(m_gameInput == nullptr)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "GameInputCreate failed, see log below for more details...");
        }
        else if (m_gamepads.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No gamepad connected.");
            ImGui::TextWrapped("Connect a gamepad to see input readings.");
        }
        else
        {
            if (ImGui::BeginTabBar("GamepadTabs"))
            {
                for (size_t i = 0; i < m_gamepads.size(); i++)
                {
                    auto& gamepad = m_gamepads[i];

                    ImGui::PushID(static_cast<int>(i));
                    ImGuiTabItemFlags tabFlags = (m_requestedGamepadTab == static_cast<int>(i))
                        ? ImGuiTabItemFlags_SetSelected
                        : ImGuiTabItemFlags_None;
                    if (ImGui::BeginTabItem(gamepad.displayName.c_str(), nullptr, tabFlags))
                    {
                        m_selectedGamepad = static_cast<int>(i);

                        float availWidth = ImGui::GetContentRegionAvail().x;
                        float spacing = ImGui::GetStyle().ItemSpacing.x;
                        float colWidth = (availWidth - spacing * 2) / 3.0f;
                        float colHeight = ImGui::GetContentRegionAvail().y - ImGuiAtg::Scaled(30);

                        ImGui::BeginChild("LeftCol", ImVec2(colWidth, colHeight), ImGuiChildFlags_None);
                        DrawDeviceInfoSection(gamepad);
                        ImGui::Spacing();
                        DrawButtonsSection(gamepad);
                        ImGui::Spacing();
                        DrawAnalogSection(gamepad.gamepadState);
                        ImGui::EndChild();

                        ImGui::SameLine();

                        ImGui::BeginChild("CenterCol", ImVec2(colWidth, colHeight), ImGuiChildFlags_None);
                        DrawSensorsSection(gamepad);
                        ImGui::Spacing();
                        DrawModelSection(gamepad);
                        ImGui::EndChild();

                        ImGui::SameLine();

                        ImGui::BeginChild("RightCol", ImVec2(colWidth, colHeight), ImGuiChildFlags_None);
                        DrawVibrationSection(gamepad);
                        ImGui::Spacing();
                        DrawHapticsSection(gamepad);
                        ImGui::Spacing();
                        DrawLightingSection(gamepad);

                        ImGui::EndChild();

                        ImGui::EndTabItem();
                    }
                    ImGui::PopID();
                }

                ImGui::EndTabBar();
                m_requestedGamepadTab = -1;
            }
        }
    }

    ImGuiAtg::SplitNext();
    ImGuiAtg::DrawLogPanel(0);
    ImGuiAtg::EndSplit();

    ImGui::EndChild();

    ImGuiAtg::BeginFooter();
        if (ImGuiAtg::FooterItem("[F3] / [LB]+[RB]+[X] Toggle Gamepad Nav"))
            ToggleGamepadNavigation();

        ImGuiAtg::FooterItem("[LB]+[RB]+[DPadUpDown] Prev/Next Item", false);
        ImGuiAtg::FooterItem("[LB]+[RB]+[DPadLeftRight] Prev/Next Tab", false);

    ImGuiAtg::EndFooter();

    ImGuiAtg::EndFullscreenLayout();
}

//--------------------------------------------------------------------------------------
// Display properties returned by GetDeviceInfo.
//--------------------------------------------------------------------------------------
void Sample::DrawDeviceInfoSection(const GamepadDevice& gamepad)
{
    if (ImGui::CollapsingHeader("Device Info", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::BeginTable("DeviceInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, ImGuiAtg::Scaled(180));
            ImGui::TableSetupColumn("Value");

            ImGuiAtg::DrawNameValueTable("Name", "%s", gamepad.displayName.c_str());
            ImGuiAtg::DrawNameValueTable("VID / PID", "%04X / %04X", gamepad.device->GetVendorId(), gamepad.device->GetProductId());

            ImGuiAtg::DrawNameValueTable("Device Family", "%s", GameInputFamilyToString(gamepad.device->GetDeviceFamily()));

            std::string supportedInputText = GameInputKindToString(gamepad.device->GetSupportedInput());
            ImGuiAtg::DrawNameValueTable("Supported Input", "%s", supportedInputText.c_str());

            std::string supportedLightingText = GameInputLightingGroupsToString(gamepad.device->GetSupportedLightingGroups());
            ImGuiAtg::DrawNameValueTable("Lighting Groups", "%s", supportedLightingText.c_str());

            ImGuiAtg::DrawNameValueTable("Last Reading Timestamp", "%llu \xc2\xb5s", gamepad.readingTimestamp);

#ifndef _GAMING_XBOX
            ImGuiAtg::DrawNameValueTable("PnP Path", "%s", gamepad.pnpPath.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(gamepad.pnpPath.c_str());
                ImGui::EndTooltip();
            }
#endif

            ImGui::EndTable();
        }
    }
}

//--------------------------------------------------------------------------------------
// Stop output and unregister callbacks.
//--------------------------------------------------------------------------------------
void Sample::Shutdown()
{
    UnregisterReadingCallbacks();

    if (m_hapticsManager)
    {
        m_hapticsManager->StopAllDevices();
        m_hapticsManager.reset();
    }

    StopVibration();
    RestoreSystemLighting();

    if (m_gameInput && m_deviceCallbackToken)
    {
        m_gameInput->UnregisterCallback(m_deviceCallbackToken);
        m_deviceCallbackToken = 0;
    }

    m_gamepads.clear();
    m_gameInput.Reset();
}

#ifdef _GAMING_XBOX
//--------------------------------------------------------------------------------------
// Xbox suspend and resume.
//--------------------------------------------------------------------------------------
void Sample::Suspend(ImGuiAtg::DeviceContext* deviceContext)
{
    StopVibration();
    RestoreSystemLighting();

    if (deviceContext)
    {
        deviceContext->Suspend();
    }
}

void Sample::Resume(ImGuiAtg::DeviceContext* deviceContext)
{
    if (deviceContext)
    {
        deviceContext->Resume();
    }
}
#endif

//--------------------------------------------------------------------------------------
// Sample input helpers.
//--------------------------------------------------------------------------------------
void Sample::ToggleGamepadNavigation()
{
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags ^= ImGuiConfigFlags_NavEnableGamepad;
}

void Sample::HandleSampleInput()
{
    const bool gamepadChord = ImGui::IsKeyDown(ImGuiKey_GamepadL1)
        && ImGui::IsKeyDown(ImGuiKey_GamepadR1);

    if ((!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F3, false)) ||
        (gamepadChord && ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft, false)))
    {
        ToggleGamepadNavigation();
    }

    int tabOffset = 0;
    if (gamepadChord && ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, false))
    {
        tabOffset = -1;
    }
    else if (gamepadChord && ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, false))
    {
        tabOffset = 1;
    }

    if (tabOffset != 0)
    {
        std::lock_guard<std::mutex> lock(m_gamepadsMutex);
        if (m_gamepads.size() > 1)
        {
            const int gamepadCount = static_cast<int>(m_gamepads.size());
            m_requestedGamepadTab = (m_selectedGamepad + tabOffset + gamepadCount) % gamepadCount;
        }
    }
}

std::string Sample::GetDeviceDisplayName(IGameInputDevice* device)
{
    char buff[1024]{};
    device->GetDisplayName(std::size(buff), buff);
    return buff[0] != '\0' ? buff : "Unknown";
}

std::string Sample::GetDevicePnpPath(IGameInputDevice* device)
{
    char buff[1024]{};
    device->GetPnpPath(std::size(buff), buff);
    return buff;
}

namespace
{
    template<typename T>
    struct EnumName
    {
        T value;
        const char* name;
    };

    constexpr EnumName<GameInputDeviceFamily> c_deviceFamilyNames[] =
    {
        { GameInputFamilyVirtual,   "Virtual" },
        { GameInputFamilyUnknown,   "Unknown" },
        { GameInputFamilyXboxOne,   "Xbox One" },
        { GameInputFamilyXbox360,   "Xbox 360" },
        { GameInputFamilyHid,       "HID" },
        { GameInputFamilyI8042,     "I8042" },
        { GameInputFamilyAggregate, "Aggregate" }
    };

    constexpr EnumName<GameInputKind> c_inputKindNames[] =
    {
        { GameInputKindGamepad,      "Gamepad" },
        { GameInputKindSensors,      "Sensors" },
        { GameInputKindLighting,     "Lighting" },
        { GameInputKindSystemButton, "SystemButton" }
    };

    // Flag tables contain individual bits, not composite values such as All.
    constexpr EnumName<GameInputLightingGroups> c_lightingGroupNames[] =
    {
        { GameInputLightingGroupLeftThumbstick,  "LeftThumbstick" },
        { GameInputLightingGroupRightThumbstick, "RightThumbstick" },
        { GameInputLightingGroupDPad,            "DPad" },
        { GameInputLightingGroupFaceButtons,     "FaceButtons" },
        { GameInputLightingGroupCenterLight,     "CenterLight" },
        { GameInputLightingGroupKeyboardKey,     "KeyboardKey" },
        { GameInputLightingGroupLeftHeadset,     "LeftHeadset" },
        { GameInputLightingGroupRightHeadset,    "RightHeadset" }
    };

    template<typename T, size_t N>
    const char* EnumToString(T value, const EnumName<T> (&names)[N], const char* fallback)
    {
        for (const auto& entry : names)
        {
            if (value == entry.value)
                return entry.name;
        }

        return fallback;
    }

    template<typename T, size_t N>
    std::string FlagsToString(T value, const EnumName<T> (&names)[N], const char* fallback)
    {
        std::string result;
        for (const auto& entry : names)
        {
            if (value & entry.value)
            {
                if (!result.empty())
                    result += " | ";
                result += entry.name;
            }
        }

        return result.empty() ? fallback : result;
    }
}

const char* Sample::GameInputFamilyToString(GameInputDeviceFamily family)
{
    return EnumToString(family, c_deviceFamilyNames, "Unknown");
}

std::string Sample::GameInputKindToString(GameInputKind kind)
{
    return FlagsToString(kind, c_inputKindNames, "Unknown");
}

std::string Sample::GameInputLightingGroupsToString(GameInputLightingGroups groups)
{
    return FlagsToString(groups, c_lightingGroupNames, "None / Unknown");
}

void Sample::Activated()
{
}

void Sample::Deactivated()
{
}

LRESULT Sample::WndProcHandler(HWND /*hWnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return 0;
}
