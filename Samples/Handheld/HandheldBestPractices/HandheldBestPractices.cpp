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

// GameInput header + namespace for v1+
#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
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
#include "VirtualKeyboard.cpp"

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
    constexpr int      FirstColWidth = 450;

    // ImGui globals
    static std::unique_ptr<AppLog> g_appLog{};
    static ImGuiStyle              g_imGuiStyle{};
    static bool                    g_firstDraw = true;

    // GameInput object
    static ComPtr<IGameInput> g_gameInput = nullptr;

    // handles
    static HWND               g_hWnd = nullptr;
    static HANDLE             g_connectivityChangedHandle = nullptr;
    static HDEVNOTIFY         g_deviceNotifyHandle = nullptr;
    static winrt::event_token g_showingToken{}, g_hidingToken{};

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
    static std::wstring g_manufacturer{}, g_productName{}, g_systemFamily{};
    static std::vector<ResolutionInfo> g_resolutions;

    // display information
    static std::wstring g_displayAdapterName{};
    static size_t       g_dedicatedVideoRAM = 0, g_sharedVideoRAM = 0;
    static UINT         g_minWave = 0, g_maxWave = 0, g_lanes = 0;
    static UINT         g_vendorId = 0, g_deviceId = 0, g_revision = 0;
    static float        g_uiScale = 1.0f;
    static bool         g_dpiChange = true, g_resetUI = false;
    static bool         g_isFullScreen = false;
    static bool         g_hdrAvailable = false, g_hdrEnabled = false;
    static UINT         g_dpiX = 0, g_dpiY = 0;
    static DWORD        g_resWidth = 0, g_resHight = 0, g_refresh = 0;

    // network information + hint handles
    static std::vector<NetworkAdapterInfo> g_networkAdapterList{};
    static std::wstring                    g_connectivity{};
};

void Sample_Initialize(HWND hWnd)
{
    g_hWnd = hWnd;
    g_appLog = std::make_unique<AppLog>();

    LoadFont();

    // get current/default style for later use with DPI and resolution changes
    g_imGuiStyle = ImGui::GetStyle();

    // setup processor info parser for later use
    ATG::SetupProcessorData();

    // initialize GameInput
    LOG_IF_FAILED(GameInputCreate(&g_gameInput));

    // get device properties
    g_isHandheld         = IsDeviceHandheld();
    g_isPowered          = IsDevicePowered();      // see Sample_WndProcHandler and WM_POWERBROADCAST handler for power state change handling
    g_isTouchEnabled     = IsDeviceTouchEnabled(); // see Sample_WndProcHandler and WM_POINTERDEVICECHANGE handler for touch capability change handling
    g_isBluetoothEnabled = IsBluetoothEnabled();   // see below for event registration, and Sample_WndProcHandler and WM_DEVICECHANGE handler for bluetooth capability change handling

    // snapshots, use the refresh button in the sample UI to get latest memory information
    LOG_IF_FAILED(GetMemoryInfo(&g_totalMemory, &g_availableMemory));
    LOG_IF_FAILED(GetProcessMemory(&g_pageFaultCount, &g_workingSetSize));

    // retrieve make/model of device, if available
    GetDeviceOEMInfo(g_manufacturer, g_productName, g_systemFamily);

    // get audio devices and setup callbacks for changes
    LOG_IF_FAILED(StartAudioDeviceMonitoring());   // see AudioDeviceManager.cpp for setting up callbacks on default audio endpoint changes

    // get display properties
    LOG_IF_FAILED(GetDeviceScreenDiagonalSizeInInches(&g_screenSize));
    LOG_IF_FAILED(GetGPUInfo(g_displayAdapterName, &g_vendorId, &g_deviceId, &g_revision, &g_dedicatedVideoRAM, &g_sharedVideoRAM, &g_minWave, &g_maxWave, &g_lanes));
    LOG_IF_FAILED(GetDeviceHDRStatus(&g_hdrAvailable, &g_hdrEnabled));
    GetDeviceScreenResolutionAndRefresh(&g_resWidth, &g_resHight, &g_refresh); // see Sample_WndProcHandler for WM_DISPLAYCHANGE handler
    g_resolutions = GetAllScreenResolutions();

    // get current DPI and set UI scale
    GetDeviceDpi(&g_dpiX, &g_dpiY); // see Sample_WndProcHandler and WM_DPICHANGED / WM_DISPLAYCHANGED handler for responding to DPI and resolution changes
    g_uiScale = (g_dpiX / DefaultDpi);
    SetUIScale(g_uiScale);

    // get network properties and register callback for network changes
    g_networkAdapterList = ListNetworkAdapters();
    LOG_IF_FAILED(NotifyNetworkConnectivityHintChange(ConnectivityHintChangedCallback, nullptr, true, &g_connectivityChangedHandle));

    // setup notifications for changes in Bluetooth state
    // this will broadcast WM_DEVICECHANGE messages, see Sample_WndProcHandler below for more info
    DEV_BROADCAST_DEVICEINTERFACE ndi {};
    ndi.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    ndi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    ndi.dbcc_classguid  = GUID_BTHPORT_DEVICE_INTERFACE;
    g_deviceNotifyHandle = RegisterDeviceNotification(hWnd, &ndi, DEVICE_NOTIFY_WINDOW_HANDLE);

    // setup virtual keyboard show/hide events
    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.primaryviewshowing
    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.primaryviewhiding
    g_showingToken = CoreInputView::GetForCurrentView().PrimaryViewShowing(
        [](CoreInputView const& /*sender*/, CoreInputViewShowingEventArgs const& /*args*/)
        {
            LOG("Virtual keyboard showing\n");
        }
    );

    g_hidingToken = CoreInputView::GetForCurrentView().PrimaryViewHiding(
        [](CoreInputView const& /*sender*/, CoreInputViewHidingEventArgs const& /*args*/)
        {
            LOG("Virtual keyboard hiding\n");
        }
    );

    // For internal debugging purposes only
    std::wstring build = GetWindowsBuildInfo();
    LOG("%ws\n", build.c_str());
}

void Sample_Draw()
{
    // reset window positions and sizes on DPI or other display changes
    if(g_dpiChange || g_resetUI)
    {
        ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_None);
        ImGui::SetNextWindowSize(ImVec2(FirstColWidth * g_uiScale, 450 * g_uiScale), ImGuiCond_None);
        g_resetUI = false;
    }

    ImGui::Begin("Interactive", nullptr, ImGuiWindowFlags_NoCollapse);

        // see Sample_WndProcHandler for WM_CHAR and WM_KEYDOWN handling for this textbox
        ImGui::Text("Click, tap, or highlight with DPad and press Y to edit text");
        ImGui::InputText("<-- TextBox", g_inputText, ARRAYSIZE(g_inputText), 0, nullptr, nullptr);

        // "true" returned when the user starts text entry in the textbox via touch or gamepad
        if(ImGui::IsItemActivated())
        {
            bool b = ShowVirtualKeyboard();
            LOG("Virtual keyboard: %d\n", b);
        }

        ImGui::Dummy(ImVec2(0, 5 * g_uiScale));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5 * g_uiScale));

        // buttons to refresh information or exit the application
        if(ImGui::Button("Refresh Network Adapters", ImVec2(200 * g_uiScale, 50 * g_uiScale)))
        {
            LOG("Refreshing network adapter list...\n");
            g_networkAdapterList = ListNetworkAdapters();
        }

        ImGui::SameLine();

        if(ImGui::Button("Refresh Memory Info", ImVec2(200 * g_uiScale, 50 * g_uiScale)))
        {
            LOG("Refreshing memory info...\n");
            LOG_IF_FAILED(GetMemoryInfo(&g_totalMemory, &g_availableMemory));
            LOG_IF_FAILED(GetProcessMemory(&g_pageFaultCount, &g_workingSetSize));
        }

        if(ImGui::Button("Reset UI", ImVec2(200 * g_uiScale, 50 * g_uiScale)))
        {
            LOG("Resetting window sizes and positions...\n");
            g_resetUI = true;
        }

        ImGui::SameLine();

        if(ImGui::Button("Exit", ImVec2(200 * g_uiScale, 50 * g_uiScale)))
        {
            LOG("Exiting sample...\n");
            PostQuitMessage(0);
        }

        ImGui::Dummy(ImVec2(0, 15 * g_uiScale));

        ImGui::Text("Gamepad Controls");
        ImGui::Separator();

        ImGui::BeginTable("Gamepad Controls", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInner);
            DrawNameValueTable("Dpad",           "Move between controls");
            DrawNameValueTable("A",              "Activate");
            DrawNameValueTable("Y",              "Edit text");
            DrawNameValueTable("X + LB/RB",      "Select Window");
            DrawNameValueTable("X + Left Stick", "Move Selected Window");
            DrawNameValueTable("X + DPad",       "Resize Selected Window");
            DrawNameValueTable("View",           "Exit");
        ImGui::EndTable();
    ImGui::End();

    if(g_dpiChange || g_resetUI)
    {
        ImGui::SetNextWindowPos(ImVec2(5, 455 * g_uiScale), ImGuiCond_None);
        ImGui::SetNextWindowSize(ImVec2(FirstColWidth * g_uiScale, 250 * g_uiScale), ImGuiCond_None);
    }

    g_appLog->Draw("Log");

    if(g_dpiChange || g_resetUI)
    {
        ImGui::SetNextWindowPos(ImVec2((FirstColWidth+5) * g_uiScale, 5), ImGuiCond_None);
        ImGui::SetNextWindowSize(ImVec2(800 * g_uiScale, 700 * g_uiScale), ImGuiCond_None);
    }

    ImGui::Begin("Device Properties", nullptr, ImGuiWindowFlags_NoCollapse);
        ImGui::Columns(2);

        if(ImGui::CollapsingHeader("Device Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("DeviceInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                DrawNameValueTable("Manufacturer",             "%ws",             g_manufacturer.c_str());
                DrawNameValueTable("Product Name",             "%ws",             g_productName.c_str());
                DrawNameValueTable("System Family",            "%ws",             g_systemFamily.c_str());
                DrawNameValueTable("Total / Available Memory", "%zu MB / %zu MB", g_totalMemory / OneMegabyte, g_availableMemory / OneMegabyte);
                DrawNameValueTable("Working Set Size",         "%zu MB",          g_workingSetSize / OneMegabyte);
                DrawNameValueTable("Page Faults",              "%d",              g_pageFaultCount);
                DrawNameBoolValueTable("IsHandheld",                              g_isHandheld);
                DrawNameBoolValueTable("IsPowered",                               g_isPowered);
                DrawNameBoolValueTable("IsTouchEnabled",                          g_isTouchEnabled);
                DrawNameBoolValueTable("IsBluetoothEnabled",                      g_isBluetoothEnabled);
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("CPU Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("CPUInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                DrawNameValueTable("CPU Name",                 "%ws",     ATG::GetProcessorName().c_str());
                DrawNameValueTable("Physical / Logical Cores", "%d / %d", ATG::GetNumberPhysicalCores(), ATG::GetNumberLogicalCores());
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("GPU Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("GPUInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                DrawNameValueTable("GPU Name",        "%ws",              g_displayAdapterName.c_str());
                DrawNameValueTable("VEN / DEV / Rev", "%04X / %04X / %d", g_vendorId, g_deviceId, g_revision);
                DrawNameValueTable("Dedicated VRAM",  "%zu MB",           g_dedicatedVideoRAM / OneMegabyte);
                DrawNameValueTable("Shared VRAM",     "%zu MB",           g_sharedVideoRAM / OneMegabyte);
                DrawNameValueTable("Min Wave Size",   "%d",               g_minWave);
                DrawNameValueTable("Max Wave Size",   "%d",               g_maxWave);
                DrawNameValueTable("Total Lanes",     "%d",               g_lanes);
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("Current Display Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("DisplayInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                DrawNameValueTable("Resolution",       "%dx%d",               g_resWidth, g_resHight);
                DrawNameValueTable("Vertical Refresh", "%dHz",                g_refresh);
                DrawNameValueTable("Effective DPI",    "%dx%d, scale %.02fx", g_dpiX, g_dpiY, g_uiScale);
            ImGui::EndTable();
        }

        ImGui::NextColumn();

        if(ImGui::CollapsingHeader("Integrated Display Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("DisplayInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                DrawNameValueTable("Screen Size", "%.2f inches", g_screenSize);
                DrawNameBoolValueTable("HDR Capable",            g_hdrAvailable);
                DrawNameBoolValueTable("HDR Enabled",            g_hdrEnabled);
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("Supported Resolutions", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushItemWidth(-1);
            if(ImGui::BeginListBox("##resolutions", ImVec2(0, 100 * g_uiScale)))
            {
                for(auto& r : g_resolutions)
                {
                    ImGui::Text("%dx%d @ %dHz", r.Width, r.Height, r.Refresh);
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

                    // get state of Gamepad, display 1 property, handle View button pressed to exit app
                    HRESULT hr = g_gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &gpReading);
                    if(SUCCEEDED(hr))
                    {
                        if(gpReading->GetGamepadState(&gpState))
                        {
                            if(gpState.buttons & GameInputGamepadView)
                            {
                                PostQuitMessage(0);
                            }
                        }
                    }
                    DrawNameValueTableHRESULT("LThumbstick X/Y", hr, "%f, %f", gpState.leftThumbstickX, gpState.leftThumbstickY);

                    // get state of keyboard, display first pressed key, if any
                    hr = g_gameInput->GetCurrentReading(GameInputKindKeyboard, nullptr, &kbReading);
                    if(SUCCEEDED(hr))
                    {
                        kbReading->GetKeyState(_countof(keyState), keyState);
                    }
                    DrawNameValueTableHRESULT("Key Pressed", hr, "%02X", keyState[0].scanCode);

                    // get state of mouse, display absolute X/Y coords
                    hr = g_gameInput->GetCurrentReading(GameInputKindMouse, nullptr, &mReading);
                    if(SUCCEEDED(hr))
                    {
                        mReading->GetMouseState(&mState);
                    }
                    DrawNameValueTableHRESULT("Mouse X/Y", hr, "%d, %d", mState.absolutePositionX, mState.absolutePositionY);

                    // "calculate" active/last used input and display
                    ActiveInputType last = GetActiveInputType(g_gameInput.Get());
                    DrawNameValueTable("Active input", "%ws", ActiveInputTypeToString(last).c_str());
                ImGui::EndTable();
            }
        }

        if(ImGui::CollapsingHeader("Network Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginTable("NetworkInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                DrawNameValueTable("Connectivity", "%ws", g_connectivity.c_str());
            ImGui::EndTable();
        }

        if(ImGui::CollapsingHeader("Network Adapters", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushItemWidth(-1);
            if(ImGui::BeginListBox("##netadapters", ImVec2(0, 50 * g_uiScale)))
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
            if(ImGui::BeginListBox("##audiodevices", ImVec2(0, 50 * g_uiScale)))
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
    ImGui::End();

    // set the "interactive" window to focus when the app starts
    if(g_firstDraw)
    {
        ImGui::SetWindowFocus("Interactive");
        g_firstDraw = false;
    }

    g_dpiChange = false;
}

void Sample_Shutdown()
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

    // unregister keyboard events
    if(g_showingToken.value)
    {
        CoreInputView::GetForCurrentView().PrimaryViewShowing(g_showingToken);
        g_showingToken.value = 0;
    }

    if(g_hidingToken.value)
    {
        CoreInputView::GetForCurrentView().PrimaryViewHiding(g_hidingToken);
        g_hidingToken.value = 0;
    }

    // stop audio device callbacks
    StopAudioDeviceMonitoring();
}

static void LoadFont()
{
    bool useDefault = true;
    char windir[MAX_PATH]{};
    char fontpath[MAX_PATH]{};

    // clear out current font
    ImGuiIO& io = ImGui::GetIO();

    // load font at new scale
    // first try to find SegoeUI, if that fails, use the built-in ImGui font
    if(GetWindowsDirectoryA(windir, MAX_PATH))
    {
        sprintf_s(fontpath, "%s\\fonts\\segoeui.ttf", windir);
        if(std::filesystem::exists(fontpath))
        {
            io.Fonts->AddFontFromFileTTF(fontpath);
            useDefault = false;
        }
    }

    // if we couldn't find it, default to the ImGui native font
    if(useDefault)
    {
        LOG("Font not found, reverting to default\n");
        ImFontConfig ifc{};
        ifc.SizePixels = 16;
        io.Fonts->AddFontDefault(&ifc);
    }
}

static void SetUIScale(float scale)
{
    // get current style and save off copy to restore colors later
    ImGuiStyle& currentStyle = ImGui::GetStyle();
    ImGuiStyle originalStyle = currentStyle;

    // clear out the style and scale a default style + font
    currentStyle = ImGuiStyle();
    currentStyle.ScaleAllSizes(scale);
    currentStyle.FontScaleDpi = scale;

    // replace the colors we saved off
    memcpy(currentStyle.Colors, originalStyle.Colors, sizeof(currentStyle.Colors));

    // used to reset window sizes and positions on the next draw
    g_dpiChange = true;
}

extern ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wParam, LPARAM lParam);

LRESULT Sample_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        // fired when screen DPI changes
        // https://learn.microsoft.com/windows/win32/hidpi/wm-dpichanged
        case WM_DPICHANGED:
            {
                LOG("WM_DPICHANGED\n");
                // place the window at new OS-provided location
                RECT* rect = (RECT*)lParam;
                SetWindowPos(hWnd, NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER);

                // set our scale factor and scale the ImGui controls
                g_dpiX = LOWORD(wParam);
                g_dpiY = HIWORD(wParam);
                g_uiScale = g_dpiX / DefaultDpi;
                SetUIScale(g_uiScale);
            }
            break;

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
            return 1;

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
            return 1;
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

void Sample_Update()
{
}
