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
        wchar_t buff[256] = {};
#ifdef _GAMING_XBOX
        swprintf_s(buff,
            L"GameInput creation failed with error: %08X\n\n",
            static_cast<unsigned int>(hr));
        OutputDebugStringW(buff);
#else
        swprintf_s(buff,
            L"GameInput creation failed with error: %08X\n\n"
            L"Verify that the GameInput runtime has been installed that is included with\n"
            L"the redist in the NuGet package or via 'winget install Microsoft.GameInput'",
            static_cast<unsigned int>(hr));
        MessageBoxW(hWnd, buff, L"Gamepad", MB_ICONERROR | MB_OK);
#endif
        PostQuitMessage(1);
        return;
    }

    ImGuiAtg::Log("GameInput created successfully\n");

    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_perfFrequency));

    if (deviceContext)
    {
        InitializeModelRenderer(deviceContext);
    }

    // Blocking enumeration reports already-connected matching devices before returning.
    hr = m_gameInput->RegisterDeviceCallback(
        nullptr,
        GameInputKindGamepad | GameInputKindSensors,
        GameInputDeviceAnyStatus,
        GameInputBlockingEnumeration,
        this,
        OnDeviceChanged,
        &m_deviceCallbackToken);

    if (FAILED(hr))
    {
        ImGuiAtg::Log("Failed to register device callback: %08X\n", static_cast<unsigned int>(hr));
    }

    // Guide and Share are system buttons, not GameInputGamepadState buttons.
    hr = m_gameInput->RegisterSystemButtonCallback(
        nullptr,
        GameInputSystemButtonGuide | GameInputSystemButtonShare,
        this,
        OnSystemButtonChanged,
        &m_systemButtonCallbackToken);

    if (FAILED(hr))
    {
        ImGuiAtg::Log("Failed to register system button callback: %08X\n", static_cast<unsigned int>(hr));
    }

    InitializeHaptics();
}

//--------------------------------------------------------------------------------------
// Track device connection and status changes.
//
// GameInput device pointers have stable identity. GetDeviceInfo returns data valid for
// the lifetime of the device.
//--------------------------------------------------------------------------------------
void CALLBACK Sample::OnDeviceChanged(
    GameInputCallbackToken /*token*/,
    void* context,
    IGameInputDevice* device,
    uint64_t /*timestamp*/,
    GameInputDeviceStatus currentStatus,
    GameInputDeviceStatus previousStatus) noexcept
{
    auto* sample = static_cast<Sample*>(context);

    const GameInputDeviceInfo* info = nullptr;
    HRESULT infoResult = device->GetDeviceInfo(&info);

    bool wasConnected = (previousStatus & GameInputDeviceConnected) != 0;
    bool isConnected = (currentStatus & GameInputDeviceConnected) != 0;
    bool hapticsReady = (currentStatus & GameInputDeviceHapticInfoReady) && !(previousStatus & GameInputDeviceHapticInfoReady);

    std::lock_guard<std::mutex> lock(sample->m_gamepadsMutex);

    if (wasConnected && !isConnected)
    {
        if (SUCCEEDED(infoResult))
        {
            ImGuiAtg::Log("Disconnected: %s (VID: %04X, PID: %04X) prev=0x%08X cur=0x%08X\n",
                GetDeviceDisplayName(info), info->vendorId, info->productId, static_cast<unsigned int>(previousStatus), static_cast<unsigned int>(currentStatus));
        }
        else
        {
            ImGuiAtg::Log("Failed to get disconnected device info: %08X\n", static_cast<unsigned int>(infoResult));
        }

        for (auto it = sample->m_gamepads.begin(); it != sample->m_gamepads.end(); ++it)
        {
            if (it->device.Get() == device)
            {
                sample->m_gamepads.erase(it);
                break;
            }
        }

        return;
    }

    if (FAILED(infoResult))
    {
        ImGuiAtg::Log("Failed to get device info: %08X\n", static_cast<unsigned int>(infoResult));
        return;
    }

    if (isConnected && !wasConnected)
    {
        ImGuiAtg::Log("Connected: %s (VID: %04X, PID: %04X) prev=0x%08X cur=0x%08X\n",
            GetDeviceDisplayName(info), info->vendorId, info->productId, static_cast<unsigned int>(previousStatus), static_cast<unsigned int>(currentStatus));

        GamepadDevice newDevice = {};
        newDevice.device = device;
        newDevice.deviceInfo = info;

        sample->m_gamepads.push_back(std::move(newDevice));
        sample->m_selectedGamepad = static_cast<int>(sample->m_gamepads.size() - 1);
    }

    if (hapticsReady)
    {
        // Haptic information may become ready after connection.
        ImGuiAtg::Log("Haptics ready: %s (VID: %04X, PID: %04X) prev=0x%08X cur=0x%08X\n",
            GetDeviceDisplayName(info), info->vendorId, info->productId, static_cast<unsigned int>(previousStatus), static_cast<unsigned int>(currentStatus));
    }
}

//--------------------------------------------------------------------------------------
// Track Guide and Share button state.
//
// Guide and Share require RegisterSystemButtonCallback. Guide delivery also depends
// on the configured focus policy.
//--------------------------------------------------------------------------------------
void CALLBACK Sample::OnSystemButtonChanged(
    GameInputCallbackToken /*token*/,
    void* context,
    IGameInputDevice* device,
    uint64_t /*timestamp*/,
    GameInputSystemButtons currentButtons,
    GameInputSystemButtons /*previousButtons*/) noexcept
{
    auto* sample = static_cast<Sample*>(context);

    std::lock_guard<std::mutex> lock(sample->m_gamepadsMutex);

    for (auto& gamepad : sample->m_gamepads)
    {
        if (gamepad.device.Get() == device)
        {
            gamepad.systemButtons = currentButtons;
            break;
        }
    }
}

//--------------------------------------------------------------------------------------
// Update readings, vibration, and model orientation.
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

    // Focus policy controls foreground/background input and system-button routing.
#ifndef _GAMING_XBOX
    if (ImGui::CollapsingHeader("Focus Policy and Reading Mode", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
    {
        ImGuiAtg::BeginNavigationGroup("FocusPolicy");

        bool changed = false;
        changed |= DrawFocusPolicyCheckbox("ExclusiveForegroundInput",       GameInputExclusiveForegroundInput);
        changed |= DrawFocusPolicyCheckbox("ExclusiveForegroundGuideButton", GameInputExclusiveForegroundGuideButton);
        changed |= DrawFocusPolicyCheckbox("ExclusiveForegroundShareButton", GameInputExclusiveForegroundShareButton);
        changed |= DrawFocusPolicyCheckbox("EnableBackgroundInput",          GameInputEnableBackgroundInput);
        changed |= DrawFocusPolicyCheckbox("EnableBackgroundGuideButton",    GameInputEnableBackgroundGuideButton);
        changed |= DrawFocusPolicyCheckbox("EnableBackgroundShareButton",    GameInputEnableBackgroundShareButton);

        if (changed && m_gameInput)
        {
            m_gameInput->SetFocusPolicy(m_focusPolicy);
            ImGuiAtg::Log("Focus policy changed to: 0x%04X\n", static_cast<unsigned int>(m_focusPolicy));
        }

        ImGuiAtg::EndNavigationGroup();
    }
#endif

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

    {
        std::lock_guard<std::mutex> lock(m_gamepadsMutex);

        if (m_gamepads.empty())
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
                    if (ImGui::BeginTabItem(GetDeviceDisplayName(gamepad.deviceInfo), nullptr, tabFlags))
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

            ImGuiAtg::DrawNameValueTable("Name", "%s", GetDeviceDisplayName(gamepad.deviceInfo));
            ImGuiAtg::DrawNameValueTable("VID / PID", "%04X / %04X", gamepad.deviceInfo->vendorId, gamepad.deviceInfo->productId);

            const char* familyStr = "Unknown";
            switch (gamepad.deviceInfo->deviceFamily)
            {
                case GameInputFamilyVirtual:
                    familyStr = "Virtual";
                    break;
                case GameInputFamilyXboxOne:
                    familyStr = "Xbox One";
                    break;
                case GameInputFamilyXbox360:
                    familyStr = "Xbox 360";
                    break;
                case GameInputFamilyHid:
                    familyStr = "HID";
                    break;
                case GameInputFamilyI8042:
                    familyStr = "I8042";
                    break;
                case GameInputFamilyAggregate:
                    familyStr = "Aggregate";
                    break;
                case GameInputFamilyUnknown:
                    break;
            }
            ImGuiAtg::DrawNameValueTable("Device Family", "%s", familyStr);

            std::string supportedInputText = GameInputKindToString(gamepad.deviceInfo->supportedInput);
            ImGuiAtg::DrawNameValueTable("Supported Input", "%s", supportedInputText.c_str());
            ImGuiAtg::DrawNameValueTable("Last Reading Timestamp", "%llu \xc2\xb5s", gamepad.readingTimestamp);

#ifndef _GAMING_XBOX
            ImGuiAtg::DrawNameValueTable("PnP Path", "%s", gamepad.deviceInfo->pnpPath);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(gamepad.deviceInfo->pnpPath);
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

    if (m_gameInput && m_deviceCallbackToken)
    {
        m_gameInput->UnregisterCallback(m_deviceCallbackToken);
        m_deviceCallbackToken = 0;
    }

    if (m_gameInput && m_systemButtonCallbackToken)
    {
        m_gameInput->UnregisterCallback(m_systemButtonCallbackToken);
        m_systemButtonCallbackToken = 0;
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
        (gamepadChord &&
         ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft, false)))
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

const char* Sample::GetDeviceDisplayName(const GameInputDeviceInfo* deviceInfo)
{
    return deviceInfo->displayName[0] != '\0' ? deviceInfo->displayName : "Xbox Gamepad";
}

bool Sample::DrawFocusPolicyCheckbox(const char* label, GameInputFocusPolicy flag)
{
    const float rightEdge = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
    const float width = ImGui::CalcTextSize(label).x + ImGuiAtg::Scaled(30);
    if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width < rightEdge)
        ImGui::SameLine();

    bool enabled = (m_focusPolicy & flag) != 0;
    if (ImGui::Checkbox(label, &enabled))
    {
        if (enabled)
            m_focusPolicy = static_cast<GameInputFocusPolicy>(m_focusPolicy | flag);
        else
            m_focusPolicy = static_cast<GameInputFocusPolicy>(m_focusPolicy & ~flag);
        return true;
    }
    return false;
}

std::string Sample::GameInputKindToString(GameInputKind kind)
{
    std::string result;
    auto append = [&](GameInputKind flag, const char* name)
    {
        if (kind & flag)
        {
            if (!result.empty())
                result += " | ";
            result += name;
        }
    };

    append(GameInputKindGamepad, "Gamepad");
    append(GameInputKindKeyboard, "Keyboard");
    append(GameInputKindMouse, "Mouse");
    append(GameInputKindArcadeStick, "ArcadeStick");
    append(GameInputKindFlightStick, "FlightStick");
    append(GameInputKindRacingWheel, "RacingWheel");
    append(GameInputKindSensors, "Sensors");

    return result.empty() ? "Unknown" : result;
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

