//--------------------------------------------------------------------------------------
// HandheldBestPractices.cpp
//
// Sample implementation
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HandheldBestPractices.h"

using Microsoft::WRL::ComPtr;

// Headers/lib for network connectivity
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

// Bluetooth headers
#include <dbt.h>
#include <bthdef.h>

// GameInput header + namespaces for v1+
#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#elif GAMEINPUT_API_VERSION == 3
using namespace GameInput::v3;
#endif

// Device snippets
#include "GetDeviceOEMInfo.cpp"
#include "GetGPUInfo.cpp"
#include "GetMemoryInfo.cpp"
#include "IsBluetoothRadioEnabled.cpp"
#include "IsDeviceHandheld.cpp"
#include "IsDeviceDockedOrOnBatteryPower.cpp"
#include "Processor.h"

// Networking snippets
#include "ConnectionType.cpp"

// Display snippets
#include "GetDeviceDpi.cpp"
#include "GetDeviceHDRStatus.cpp"
#include "GetDeviceResolutionAndRefresh.cpp"
#include "ScreenDimensions.cpp"

// Input snippets
#include "GetActiveInput.cpp"
#include "IsDeviceTouchEnabled.cpp"

// NOTE: Only include one of these.
// The CppWinRT implementation includes winrt/base.h, which under /std:c++17 pulls in the MSVC
// <experimental/coroutine> header. That header hard-errors under clang-cl, so when building with
// clang we fall back to the ABI/WRL "Alt" implementation, which provides the same helpers without
// CppWinRT. MSVC builds keep the CppWinRT path.
#ifdef __clang__
#include "VirtualKeyboardAlt.cpp"     // Downlevel-compatible, no-CppWinRT implementation (clang-safe)
#else
#include "VirtualKeyboard.cpp"        // CppWinRT-based implementation
#endif

// Text entry via dialog box
#include "TextEntry.cpp"

// Audio snippets
#include "AudioDeviceManager.cpp"

// Forward decls
static void ConnectivityHintChangedCallback(PVOID, NL_NETWORK_CONNECTIVITY_HINT);
static std::wstring ActiveInputTypeToString(ActiveInputType t);
static std::wstring GetWindowsBuildInfo();

namespace
{
    // constants
    constexpr float    DefaultDpi = 96.0f;
    constexpr uint32_t OneMegabyte = (1024*1024);
    // ImGui globals
    static ImGuiStyle              g_imGuiStyle{};

    // GameInput object
    static ComPtr<IGameInput> g_gameInput = nullptr;

    // handles
    static HWND               g_hWnd = nullptr;
    static HANDLE             g_connectivityChangedHandle = nullptr;
    static HDEVNOTIFY         g_deviceNotifyHandle = nullptr;
    
    // string backing the edit box
    static char g_inputText[1024] = "Enter text here...";

    // device information
    static bool   g_isHandheld = false;
    static bool   g_isPowered = false;
    static bool   g_isTouchEnabled = false;
    static bool   g_isBluetoothEnabled = false;
    static double g_screenSize = 0;
    static size_t g_totalMemory = 0, g_availableMemory = 0;
    static DWORD  g_pageFaultCount = 0;
    static size_t g_workingSetSize = 0;
    static std::wstring g_manufacturer{}, g_productName{}, g_systemFamily{}, g_baseboardProduct{};
    static std::vector<ResolutionInfo> g_resolutions;

    // display information
    static std::wstring g_displayAdapterName{};
    static size_t       g_dedicatedVideoRAM = 0, g_sharedVideoRAM = 0;
    static UINT         g_minWave = 0, g_maxWave = 0, g_lanes = 0;
    static UINT         g_vendorId = 0, g_deviceId = 0, g_revision = 0;
    static bool         g_resetUI = false;
    static bool         g_hdrAvailable = false, g_hdrEnabled = false;
    static UINT         g_dpiX = 0, g_dpiY = 0;
    static DWORD        g_resWidth = 0, g_resHight = 0, g_refresh = 0;

    // network information + hint handles
    static std::vector<NetworkAdapterInfo> g_networkAdapterList{};
    static std::wstring                    g_connectivity{};
};

void Sample::Initialize(HWND hWnd)
{
    g_hWnd = hWnd;

    // get current/default style for later use with DPI and resolution changes
    g_imGuiStyle = ImGui::GetStyle();

    // setup processor info parser for later use
    ATG::SetupProcessorData();

    // initialize GameInput
    LOG_IF_FAILED(GameInputCreate(&g_gameInput));

    // get device properties
    g_isHandheld         = IsDeviceHandheld();     // see IsDeviceHandheld.cpp for a downlevel-compilable version of this function
    g_isPowered          = IsDevicePowered();      // see Sample::WndProcHandler and WM_POWERBROADCAST handler for power state change handling
    g_isTouchEnabled     = IsDeviceTouchEnabled(); // see Sample::WndProcHandler and WM_POINTERDEVICECHANGE handler for touch capability change handling
    g_isBluetoothEnabled = IsBluetoothEnabled();   // see below for event registration, and Sample::WndProcHandler and WM_DEVICECHANGE handler for bluetooth capability change handling

    // snapshots, use the refresh button in the sample UI to get latest memory information
    LOG_IF_FAILED(GetMemoryInfo(&g_totalMemory, &g_availableMemory));
    LOG_IF_FAILED(GetProcessMemory(&g_pageFaultCount, &g_workingSetSize));

    // retrieve make/model of device, if available
    GetDeviceOEMInfo(g_manufacturer, g_productName, g_systemFamily, g_baseboardProduct);

    // get audio devices and setup callbacks for changes
    LOG_IF_FAILED(StartAudioDeviceMonitoring());   // see AudioDeviceManager.cpp for setting up callbacks on default audio endpoint changes

    // get display properties
    LOG_IF_FAILED(GetDeviceScreenDiagonalSizeInInches(&g_screenSize));
    LOG_IF_FAILED(GetGPUInfo(g_displayAdapterName, &g_vendorId, &g_deviceId, &g_revision, &g_dedicatedVideoRAM, &g_sharedVideoRAM, &g_minWave, &g_maxWave, &g_lanes));
    LOG_IF_FAILED(GetDeviceHDRStatus(&g_hdrAvailable, &g_hdrEnabled));
    GetDeviceScreenResolutionAndRefresh(&g_resWidth, &g_resHight, &g_refresh); // see Sample::WndProcHandler for WM_DISPLAYCHANGE handler
    g_resolutions = GetAllScreenResolutions();

    // get current DPI and set UI scale
    GetDeviceDpi(&g_dpiX, &g_dpiY); // see WndProc WM_DPICHANGED handler for responding to DPI changes
    // get network properties and register callback for network changes
    g_networkAdapterList = ListNetworkAdapters();
    LOG_IF_FAILED(NotifyNetworkConnectivityHintChange(ConnectivityHintChangedCallback, nullptr, true, &g_connectivityChangedHandle));

    // setup notifications for changes in Bluetooth state
    // this will broadcast WM_DEVICECHANGE messages, see Sample::WndProcHandler below for more info
    DEV_BROADCAST_DEVICEINTERFACE ndi {};
    ndi.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    ndi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    ndi.dbcc_classguid  = GUID_BTHPORT_DEVICE_INTERFACE;
    g_deviceNotifyHandle = RegisterDeviceNotification(hWnd, &ndi, DEVICE_NOTIFY_WINDOW_HANDLE);

    // setup virtual keyboard show/hide events
    RegisterKeyboardShowingEvent([]() { LOG("VK Showing\n"); });
    RegisterKeyboardHidingEvent([]()  { LOG("VK Hiding\n");  });

     // For internal debugging purposes only
    std::wstring build = GetWindowsBuildInfo();
    LOG("%ws\n", build.c_str());
}

void Sample::Draw()
{
    ImGuiAtg::BeginFullscreenLayout();

    // Reserve space for the standard sample footer below the split
    float footerH = ImGuiAtg::GetFooterHeight();
    ImGui::BeginChild("##SplitArea", ImVec2(0, ImGui::GetContentRegionAvail().y - footerH));

    // Content on top, log on bottom, with draggable splitter
    ImGuiAtg::BeginSplitH("##LogSplit", 180.0f);

    // Three-column layout: Interactive | Device/CPU/GPU/Display | Integrated/Resolutions/Input/Network/Audio
    float availWidth = ImGui::GetContentRegionAvail().x;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float colWidth = (availWidth - spacing * 2) / 3.0f;
    float colHeight = ImGui::GetContentRegionAvail().y;

    // Column 1: Interactive controls
    ImGui::BeginChild("Interactive", ImVec2(colWidth, colHeight), ImGuiChildFlags_None);
    if (ImGui::CollapsingHeader("Interactive", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Click, tap, or highlight with DPad and press Y to edit text");
        ImGui::InputText("<-- TextBox", g_inputText, ARRAYSIZE(g_inputText), 0, nullptr, nullptr);

        // "true" returned when the user starts text entry in the textbox via touch or gamepad
        if(ImGui::IsItemActivated())
        {
            bool b = ShowVirtualKeyboard();
            LOG("Virtual keyboard: %d\n", b);
        }

        if (ImGui::Button("Open Text Entry Dialog", ImVec2(ImGuiAtg::Scaled(300), ImGuiAtg::Scaled(50))))
        {
            // disable gamepad input for main window
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;

            ATG::ShowTextEntry("A Simple Text Entry Dialog", "This is an area to enter some text.  Try it!", "Default Text", 0,
                               [](void* /*userContext*/, bool confirmed, const char* resultTextBuffer, uint32_t resultTextBufferUsed)
            {
                if(confirmed)
                {
                    LOG("Entered text: %s - Buffer Size: %d\n", resultTextBuffer, resultTextBufferUsed);
                }
                else
                {
                    LOG("Text entry canceled\n");
                }

                // re-enable gamepad input for main window
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            }, nullptr);
        }

        ImGui::Dummy(ImVec2(0, ImGuiAtg::Scaled(5)));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, ImGuiAtg::Scaled(5)));

        // buttons to refresh information or exit the application
        if(ImGui::Button("Refresh Network Adapters", ImVec2(ImGuiAtg::Scaled(200), ImGuiAtg::Scaled(50))))
        {
            LOG("Refreshing network adapter list...\n");
            g_networkAdapterList = ListNetworkAdapters();
        }

        ImGui::SameLine();

        if(ImGui::Button("Refresh Memory Info", ImVec2(ImGuiAtg::Scaled(200), ImGuiAtg::Scaled(50))))
        {
            LOG("Refreshing memory info...\n");
            LOG_IF_FAILED(GetMemoryInfo(&g_totalMemory, &g_availableMemory));
            LOG_IF_FAILED(GetProcessMemory(&g_pageFaultCount, &g_workingSetSize));
        }

        if(ImGui::Button("Reset UI", ImVec2(ImGuiAtg::Scaled(200), ImGuiAtg::Scaled(50))))
        {
            LOG("Resetting window sizes and positions...\n");
            g_resetUI = true;
        }

        ImGui::SameLine();

        if(ImGui::Button("Exit", ImVec2(ImGuiAtg::Scaled(200), ImGuiAtg::Scaled(50))))
        {
            LOG("Exiting sample...\n");
            PostQuitMessage(0);
        }

        ImGui::Dummy(ImVec2(0, ImGuiAtg::Scaled(15)));

        if(ImGui::CollapsingHeader("Gamepad Controls##Header", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("Gamepad Controls##Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInner);
                ImGuiAtg::DrawNameValueTable("Dpad",           "Move between controls");
                ImGuiAtg::DrawNameValueTable("A",              "Activate");
                ImGuiAtg::DrawNameValueTable("Y",              "Edit text");
                ImGuiAtg::DrawNameValueTable("X + LB/RB",      "Select Window");
                ImGuiAtg::DrawNameValueTable("X + Left Stick", "Move Selected Window");
                ImGuiAtg::DrawNameValueTable("X + DPad",       "Resize Selected Window");
            ImGui::EndTable();
        }
    }
    ImGui::EndChild(); // end Interactive

    ImGui::SameLine();

    // Column 2: Device Info, CPU Info, GPU Info
    ImGui::BeginChild("DeviceProperties", ImVec2(colWidth, colHeight), ImGuiChildFlags_None);

        if(ImGui::CollapsingHeader("Device Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("DeviceInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                ImGuiAtg::DrawNameValueTable("Manufacturer",             "%ws",             g_manufacturer.c_str());
                ImGuiAtg::DrawNameValueTable("Product Name",             "%ws",             g_productName.c_str());
                ImGuiAtg::DrawNameValueTable("System Family",            "%ws",             g_systemFamily.c_str());
                ImGuiAtg::DrawNameValueTable("Baseboard Product",        "%ws",             g_baseboardProduct.c_str());
                ImGuiAtg::DrawNameValueTable("Total / Available Memory", "%zu MB / %zu MB", g_totalMemory / OneMegabyte, g_availableMemory / OneMegabyte);
                ImGuiAtg::DrawNameValueTable("Working Set Size",         "%zu MB",          g_workingSetSize / OneMegabyte);
                ImGuiAtg::DrawNameValueTable("Page Faults",              "%d",              g_pageFaultCount);
                ImGuiAtg::DrawNameBoolValueTable("IsHandheld",                              g_isHandheld);
                ImGuiAtg::DrawNameBoolValueTable("IsPowered",                               g_isPowered);
                ImGuiAtg::DrawNameBoolValueTable("IsTouchEnabled",                          g_isTouchEnabled);
                ImGuiAtg::DrawNameBoolValueTable("IsBluetoothEnabled",                      g_isBluetoothEnabled);
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("CPU Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("CPUInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                ImGuiAtg::DrawNameValueTable("CPU Name",                 "%ws",     ATG::GetProcessorName().c_str());
                ImGuiAtg::DrawNameValueTable("Physical / Logical Cores", "%d / %d", ATG::GetNumberPhysicalCores(), ATG::GetNumberLogicalCores());
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("GPU Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("GPUInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                ImGuiAtg::DrawNameValueTable("GPU Name",            "%ws",              g_displayAdapterName.c_str());
                ImGuiAtg::DrawNameValueTable("VEN / DEV / Rev",     "%04X / %04X / %d", g_vendorId, g_deviceId, g_revision);
                ImGuiAtg::DrawNameValueTable("Dedicated VRAM",      "%zu MB",           g_dedicatedVideoRAM / OneMegabyte);
                ImGuiAtg::DrawNameValueTable("Shared VRAM",         "%zu MB",           g_sharedVideoRAM / OneMegabyte);
                ImGuiAtg::DrawNameValueTable("Min / Max Wave Size", "%d / %d",          g_minWave, g_maxWave);
                ImGuiAtg::DrawNameValueTable("Total Lanes",         "%d",               g_lanes);
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("Current Display Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("DisplayInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                ImGuiAtg::DrawNameValueTable("Resolution",       "%dx%d @ %dHz",        g_resWidth, g_resHight, g_refresh);
                ImGuiAtg::DrawNameValueTable("Effective DPI",    "%dx%d, scale %.02fx", g_dpiX, g_dpiY, ImGuiAtg::GetCurrentScale());
            ImGui::EndTable();
        }

    ImGui::EndChild(); // end DeviceProperties

    ImGui::SameLine();

    // Column 3: Integrated Display, Resolutions, Input, Network, Audio
    ImGui::BeginChild("DisplayNetwork", ImVec2(colWidth, colHeight), ImGuiChildFlags_None);

        if(ImGui::CollapsingHeader("Integrated Display Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("DisplayInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                ImGuiAtg::DrawNameValueTable("Screen Size", "%.2f inches", g_screenSize);
                ImGuiAtg::DrawNameBoolValueTable("HDR Capable",            g_hdrAvailable);
                ImGuiAtg::DrawNameBoolValueTable("HDR Enabled",            g_hdrEnabled);
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("Supported Resolutions", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushItemWidth(-1);
            if(ImGui::BeginListBox("##resolutions", ImVec2(0, ImGuiAtg::Scaled(100))))
            {
                for(auto& r : g_resolutions)
                {
                    ImGui::Text("%lux%lu @ %luHz", r.Width, r.Height, r.Refresh);
                }
                ImGui::EndListBox();
            }
            ImGui::PopItemWidth();
        }

        if(ImGui::CollapsingHeader("Input Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if(g_gameInput == nullptr)
            {
                ImGui::Text("GameInput initialization failed\nEnsure latest GameInputRedist.msi is installed");
            }
            else
            {
                // use GameInput to get the current states of keyboard, mouse, and gamepads
                ComPtr<IGameInputReading> gpReading, kbReading, mReading;
                GameInputGamepadState gpState{};
                GameInputKeyState keyState[16]{};
                GameInputMouseState mState{};

                ImGui::BeginTable("InputInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);

                    // get state of Gamepad and display 1 property. Standard footer combos
                    // are handled centrally by ImGuiAtg::HandleStandardInput in Main.cpp.
                    HRESULT hr = g_gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &gpReading);
                    if(SUCCEEDED(hr))
                    {
                        gpReading->GetGamepadState(&gpState);
                    }
                    ImGuiAtg::DrawNameValueTableHRESULT("LThumbstick X/Y", hr, "%f, %f", gpState.leftThumbstickX, gpState.leftThumbstickY);

                    // get state of keyboard, display first pressed key, if any
                    hr = g_gameInput->GetCurrentReading(GameInputKindKeyboard, nullptr, &kbReading);
                    if(SUCCEEDED(hr))
                    {
                        kbReading->GetKeyState(_countof(keyState), keyState);
                    }
                    ImGuiAtg::DrawNameValueTableHRESULT("Key Pressed", hr, "%02X", keyState[0].scanCode);

                    // get state of mouse, display absolute X/Y coords
                    hr = g_gameInput->GetCurrentReading(GameInputKindMouse, nullptr, &mReading);
                    if(SUCCEEDED(hr))
                    {
                        mReading->GetMouseState(&mState);
                    }
#ifdef GAMEINPUT_API_VERSION
                    ImGuiAtg::DrawNameValueTableHRESULT("Mouse X/Y", hr, "%d, %d", mState.absolutePositionX, mState.absolutePositionY);
#endif
                    // "calculate" active/last used input and display
                    ActiveInputType last = GetActiveInputType(g_gameInput.Get());
                    ImGuiAtg::DrawNameValueTable("Active input", "%ws", ActiveInputTypeToString(last).c_str());
                ImGui::EndTable();
            }
        }

        if(ImGui::CollapsingHeader("Network Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("NetworkInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                ImGuiAtg::DrawNameValueTable("Connectivity", "%ws", g_connectivity.c_str());
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("Network Adapters", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushItemWidth(-1);
            if(ImGui::BeginListBox("##netadapters", ImVec2(0, ImGuiAtg::Scaled(75))))
            {
                ImGui::Columns(2);
                for(auto& i : g_networkAdapterList)
                {
                    // draw the currently used and active network adapter(s) in green, draw others in red
                    ImGui::PushStyleColor(ImGuiCol_Text, i.operStatus ? ImVec4(0, 1.0f, 0, 1.0f) : ImVec4(1.0f, 0, 0, 1.0f));
                    ImGui::Text("%ws", i.description.c_str());
                    ImGui::NextColumn();
                    ImGui::Text("%ws", i.name.c_str());
                    ImGui::NextColumn();
                    ImGui::PopStyleColor();
                }
                ImGui::EndListBox();
            }
            ImGui::PopItemWidth();
        }

        if(ImGui::CollapsingHeader("Audio Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushItemWidth(-1);
            if(ImGui::BeginListBox("##audiodevices", ImVec2(0, ImGuiAtg::Scaled(75))))
            {
                for(auto& a : g_audioDevices)
                {
                    // draw the default audio device in green
                    if(a.isDefault)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1.0f, 0, 1.0f));
                    }
                    ImGui::Text("%ws", a.friendlyName);
                    if(a.isDefault)
                    {
                        ImGui::PopStyleColor();
                    }
                }
                ImGui::EndListBox();
            }
            ImGui::PopItemWidth();
        }
    ImGui::EndChild(); // end DisplayNetwork

    ImGuiAtg::SplitNext();

    // Log panel
    ImGuiAtg::DrawLogPanel(0);

    ImGuiAtg::EndSplit();
    ImGui::EndChild();

    // Standard exit / theme-toggle footer
    ImGuiAtg::DrawFooter();

    ImGuiAtg::EndFullscreenLayout();
}

void Sample::Shutdown()
{
    // remove network callback
    if(g_connectivityChangedHandle)
    {
        CancelMibChangeNotify2(g_connectivityChangedHandle);
        g_connectivityChangedHandle = nullptr;
    }

    // remove device notify callback
    if(g_deviceNotifyHandle)
    {
        UnregisterDeviceNotification(g_deviceNotifyHandle);
        g_deviceNotifyHandle = nullptr;
    }

    // remove keyboard show/hide event callbacks
    UnregisterKeyboardShowingEvent();
    UnregisterKeyboardHidingEvent();

    // stop audio device callbacks
    StopAudioDeviceMonitoring();
}

extern ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wParam, LPARAM lParam);

LRESULT Sample::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        // WM_DPICHANGED handled in Main.cpp

        // fired when device resolution, HDR mode, or refresh changes
        // https://learn.microsoft.com/windows/win32/gdi/wm-displaychange
        case WM_DISPLAYCHANGE:
            LOG("WM_DISPLAYCHANGE\n");
            GetDeviceScreenResolutionAndRefresh(&g_resWidth, &g_resHight, &g_refresh);
            LOG_IF_FAILED(GetDeviceHDRStatus(&g_hdrAvailable, &g_hdrEnabled));
            break;

        // fired when a power property is changed
        // https://learn.microsoft.com/windows/wisn32/power/wm-powerbroadcast
        case WM_POWERBROADCAST:
            LOG("WM_POWERBROADCAST\n");
            g_isPowered = IsDevicePowered();
            break;

        // fired when touch screen property is changed
        // https://learn.microsoft.com/windows/win32/inputmsg/wm-pointerdevicechange
        case WM_POINTERDEVICECHANGE:
            LOG("WM_POINTERDEVICECHANGE\n");
            g_isTouchEnabled = IsDeviceTouchEnabled();
            break;

        // Used to determine if/when Bluetooth goes offline/online
        // https://learn.microsoft.com/windows/win32/devio/wm-devicechange
        case WM_DEVICECHANGE:
            {
                PDEV_BROADCAST_DEVICEINTERFACE bdi = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
                if(bdi && bdi->dbcc_classguid == GUID_BTHPORT_DEVICE_INTERFACE)
                {
                    switch(wParam)
                    {
                        case DBT_DEVICEARRIVAL:
                            LOG("WM_DEVICECHANGE: Bluetooth arrival\n");
                            g_isBluetoothEnabled = true;
                            break;

                        case DBT_DEVICEREMOVECOMPLETE:
                            LOG("WM_DEVICECHANGE: Bluetooth remove complete\n");
                            g_isBluetoothEnabled = false;
                            break;
                    }
                }
            }
            break;

        // simple handler for keyboard with textbox and virtual keyboard
        // see ImGui for full implementation
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            {
                const bool isKeyDown = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                const ImGuiKey key = ImGui_ImplWin32_KeyEventToImGuiKey(wParam, lParam);
                if (key != ImGuiKey_None)
                {
                    ImGui::GetIO().AddKeyEvent(key, isKeyDown);
                }

                // hide the keyboard if user presses escape or return
                if(wParam == VK_ESCAPE || wParam == VK_RETURN)
                {
                    HideVirtualKeyboard();
                }
            }
            return 0;

        case WM_CHAR:
            // add the currently pressed keyboard key to ImGui's processor which will put it in the textbox
            // code is the same for physical or virtual keyboards
            if (IsWindowUnicode(hWnd))
            {
                if (wParam > 0 && wParam < 0x10000)
                {
                    ImGui::GetIO().AddInputCharacterUTF16((unsigned short)wParam);
                }
            }
            else
            {
                wchar_t wch = 0;
                MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, (char*)&wParam, 1, &wch, 1);
                ImGui::GetIO().AddInputCharacter(wch);
            }
            return 0;
    }

    // continue processing by ImGui and DefWindowProc
    return 0;
}

static void ConnectivityHintChangedCallback(PVOID, NL_NETWORK_CONNECTIVITY_HINT hint)
{
    switch(hint.ConnectivityLevel)
    {
        case NetworkConnectivityLevelHintNone:
            g_connectivity = L"None";
            break;
        case NetworkConnectivityLevelHintLocalAccess:
            g_connectivity = L"Local";
            break;
        case NetworkConnectivityLevelHintInternetAccess:
            g_connectivity = L"Internet";
            break;
        case NetworkConnectivityLevelHintConstrainedInternetAccess:
            g_connectivity = L"Constrained Internet";
            break;
        case NetworkConnectivityLevelHintHidden:
            g_connectivity = L"Hidden";
            break;
        case NetworkConnectivityLevelHintUnknown:
        default:
            g_connectivity = L"Unknown";
            break;
    }
}

static std::wstring ActiveInputTypeToString(ActiveInputType t)
{
    switch(t)
    {
        case ActiveInputType::Keyboard:
            return L"Keyboard";
        case ActiveInputType::Mouse:
            return L"Mouse";
        case ActiveInputType::Gamepad:
            return L"Gamepad";
        case ActiveInputType::Unknown:
        default:
            return L"Unknown";
    }
}

// For Debugging purposes only...
static std::wstring GetWindowsBuildInfo()
{
    wchar_t build[1024]{};
    DWORD dataSize = sizeof(wchar_t) * _countof(build);

    LSTATUS status = RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SBI", L"SourceBuild", RRF_RT_REG_SZ, nullptr, build, &dataSize);
    return (status == ERROR_SUCCESS) ? build : L"";
}

void Sample::Update()
{
}

void Sample::Activated()
{
}

void Sample::Deactivated()
{
}
