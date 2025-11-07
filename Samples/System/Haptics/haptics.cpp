///--------------------------------------------------------------------------------------
// haptics.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include <commdlg.h>

#include "haptics.h"
#include "Audio/WASAPIManager.h"
#include "Audio/XAudio2Manager.h"

using namespace Microsoft::WRL;

struct MediaItem
{
    std::wstring filename {};
    std::wstring title {};
};

struct DeviceData
{
    IGameInputDevice* device {};
    GameInputGamepadState lastState {};
    MediaItem mediaItem {};
    std::unique_ptr<WASAPIManager> wasapi {};
    std::unique_ptr<XAudio2Manager> xaudio {};
};

extern HWND g_hWnd;

std::unique_ptr<AppLog> g_appLog {};
static bool g_firstDraw = true;

ComPtr<IGameInput> g_gameInput {};
GameInputCallbackToken g_deviceCallbackToken {};
std::map<IGameInputDevice*, DeviceData> g_devices {};

std::vector<MediaItem> g_mediaList =
{
    { L"media\\AhoogaHorn.wav",  L"Ahooga Horn" },
    { L"media\\car.wav",         L"Car" },
    { L"media\\fireworks.wav",   L"Fireworks" },
    { L"media\\gun_shot.wav",    L"Gun Shot" },
    { L"media\\PenScratch.wav",  L"Pen Scratch" },
    { L"media\\ShakeEffect.wav", L"Shake" },
    { L"media\\TommyGun.wav",    L"Tommy Gun" },

};

static void InitializeHaptics(IGameInputDevice* device, wchar_t* endpoint, uint32_t locationCount, GUID* locations)
{
    const GameInputDeviceInfo* di = nullptr;
    LOG_IF_FAILED_AND_RETURN(device->GetDeviceInfo(&di));

    LOG("Initialize Haptics\n  VID / PID: %04X / %04X\n  Device Id: %s\n  Audio Endpoint: %ws\n  Location Count: %d\n", di->vendorId, di->productId, StringifyDeviceId(di->deviceId), endpoint, locationCount);

    // create a WASAPI engine for the device
    auto wm = std::make_unique<WASAPIManager>();
    HRESULT hr = wm->InitializeDevice(endpoint, locationCount, locations);
    if (FAILED(hr))
    {
        LOG("Failed to initialize WASAPI endpoint: %08X\n", hr);
    }
    else
    {
        g_devices[device].wasapi = std::move(wm);
    }

    // create an XAudio2 engine for the device
    auto xm = std::make_unique<XAudio2Manager>();
    hr = xm->InitializeDevice(endpoint, locationCount, locations);
    if (FAILED(hr))
    {
        LOG("Failed to initialize XAudio2 endpoint: %08X\n", hr);
    }
    else
    {
        g_devices[device].xaudio = std::move(xm);
    }
}

static void DeviceCallback(GameInputCallbackToken, void*, IGameInputDevice* device, uint64_t, GameInputDeviceStatus currentStatus, GameInputDeviceStatus previousStatus) noexcept
{
    const GameInputDeviceInfo* di = nullptr;
    LOG_IF_FAILED_AND_RETURN(device->GetDeviceInfo(&di));

    LOG("DeviceCallback\n  VID / PID: %04X / %04X\n  Device Id: %s\n  Previous: %08X, Current: %08X\n", di->vendorId, di->productId, StringifyDeviceId(di->deviceId), previousStatus, currentStatus);

    bool wasConnected = (previousStatus & GameInputDeviceConnected) != 0;
    bool isConnected = (currentStatus & GameInputDeviceConnected) != 0;

    // newly connected device, add to our list
    if(isConnected && !wasConnected)
    {
        DeviceData dd;
        dd.device = device;
        dd.mediaItem = g_mediaList[0];
        g_devices[device] = std::move(dd);
    }

    // newly connected haptics device, initialize haptics
    if(isConnected && currentStatus & GameInputDeviceStatus::GameInputDeviceHapticInfoReady)
    {
        GameInputHapticInfo hapticInfo;
        LOG_IF_FAILED_AND_RETURN(device->GetHapticInfo(&hapticInfo));
        InitializeHaptics(device, hapticInfo.audioEndpointId, hapticInfo.locationCount, hapticInfo.locations);
    }

    // newly disconnected device, remove from our list
    if(wasConnected && !isConnected && g_devices[device].device != nullptr)
    {
        g_devices[device].wasapi.reset();
        g_devices[device].xaudio.reset();
        g_devices.erase(device);
    }
}

static void PlayWasapiEffect(DeviceData* dd)
{
    if(!dd)
    {
        return;
    }

    LOG_IF_FAILED_AND_RETURN(dd->wasapi->ConfigureWaveSource(dd->mediaItem.filename.c_str()));
    LOG_IF_FAILED_AND_RETURN(dd->wasapi->Play());
}

static void PlayXAudioEffect(DeviceData* dd)
{
    if(!dd)
    {
        return;
    }

    LOG_IF_FAILED_AND_RETURN(dd->xaudio->ConfigureWaveSource(dd->mediaItem.filename.c_str()));
    LOG_IF_FAILED_AND_RETURN(dd->xaudio->Play());
}

static void StopHapticEffect(DeviceData* dd)
{
    if(!dd)
    {
        return;
    }
    
    if(dd->wasapi)
    {
        LOG_IF_FAILED_AND_RETURN(dd->wasapi->Stop());
    }

    if(dd->xaudio)
    {
        LOG_IF_FAILED_AND_RETURN(dd->xaudio->Stop());
    }
}

static void StopAllHapticEffects()
{
    for(auto& dd : g_devices)
    {
        StopHapticEffect(&dd.second);
    }
}

void Sample_Initialize()
{
    g_appLog = std::make_unique<AppLog>();

    // add the current directory to all of the media file paths
    wchar_t exePath[MAX_PATH];
    if(GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0)
    {
        // remove the exe name
        auto path = std::wstring(exePath);
        path = path.substr(0, path.find_last_of(L"\\"));

        // add the full path to the media directory to each media item
        for(auto &mi : g_mediaList)
        {
            mi.filename = path + L"\\" + mi.filename;
        }
    }
    else
    {
        LOG("Unable to get current exe path\n");
    }

    // initialize GameInput and setup callbacks for all controller devices
    LOG_IF_FAILED_AND_RETURN(GameInputCreate(&g_gameInput));
    LOG_IF_FAILED_AND_RETURN(g_gameInput->RegisterDeviceCallback(nullptr,
        GameInputKind::GameInputKindController,
        GameInputDeviceStatus::GameInputDeviceAnyStatus,
        GameInputEnumerationKind::GameInputBlockingEnumeration,
        nullptr,
        DeviceCallback, &g_deviceCallbackToken));
}

void Sample_Draw(float uiScale)
{
    ImGui::SetNextWindowPos(ImVec2(5 * uiScale, 5 * uiScale), ImGuiCond_None);
    ImGui::SetNextWindowSize(ImVec2(700 * uiScale, 810 * uiScale), ImGuiCond_None);

    ImGui::Begin("Advanced Haptics", nullptr, ImGuiWindowFlags_NoCollapse);

    if(g_devices.size() == 0)
    {
        ImGui::Text("Please connect a haptics-enabled controller");
    }
    else
    {
        // draw a section for each device attached
        for(auto& dd : g_devices)
        {
            char header[512];
            ComPtr<IGameInputReading> reading;
            GameInputGamepadState state;

            g_gameInput->GetCurrentReading(GameInputKindGamepad, dd.first, &reading);
            reading->GetGamepadState(&state);

            const GameInputDeviceInfo* di = nullptr;
            LOG_IF_FAILED_AND_RETURN(dd.first->GetDeviceInfo(&di));

            sprintf_s(header, "VID/PID:  %04X / %04X\nDeviceID: %s", di->vendorId, di->productId, StringifyDeviceId(di->deviceId));
    
            static bool isSelected = false;

            if(ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
            {
                ImGui::Dummy(ImVec2(0, 16.0f));
                ImGui::Indent(16.0f);
                ImGui::PushID(dd.first);

                if(dd.second.wasapi == nullptr && dd.second.xaudio == nullptr)
                {
                    ImGui::Text("No haptic device(s) found for this controller");
                }
                else
                {
                    ImGui::Text("Select a haptic effect to play");
                    if(ImGui::BeginCombo("##effect", DX::WideToUtf8(dd.second.mediaItem.title).c_str()))
                    {
                        for(auto& mi : g_mediaList)
                        {
                            isSelected = (dd.second.mediaItem.filename == mi.filename);
                            if(ImGui::Selectable(DX::WideToUtf8(mi.title).c_str(), &isSelected))
                            {
                                dd.second.mediaItem = mi;
                            }

                            if(isSelected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    if (ImGui::Button("Browse for file...", ImVec2(160*uiScale, 50*uiScale)))
                    {
                        std::wstring selectedFile = OpenWavFileDialog(g_hWnd);
                        if (!selectedFile.empty())
                        {
                            MediaItem mi;
                            mi.filename = selectedFile;
                            mi.title = selectedFile;
                            g_mediaList.push_back(mi);
                            dd.second.mediaItem = mi;
                        }
                    }

                    ImGui::Dummy(ImVec2(0, 20));

                    ImGui::BeginDisabled(dd.second.wasapi->IsPlaying() || (dd.second.xaudio && dd.second.xaudio->IsPlaying()));
                        if(ImGui::Button("Play WASAPI (L1)", ImVec2(160*uiScale, 50*uiScale)) || IsButtonPressed(state.buttons, dd.second.lastState.buttons, GameInputGamepadButtons::GameInputGamepadLeftShoulder))
                        {
                            PlayWasapiEffect(&dd.second);
                        }
                        ImGui::SameLine();

                        if(ImGui::Button("Play XAudio2 (R1)", ImVec2(160*uiScale, 50*uiScale)) || IsButtonPressed(state.buttons, dd.second.lastState.buttons, GameInputGamepadButtons::GameInputGamepadRightShoulder))
                        {
                            PlayXAudioEffect(&dd.second);
                        }
                    ImGui::EndDisabled();

                    ImGui::SameLine();

                    if (ImGui::Button("Stop (B/Square)", ImVec2(160*uiScale, 50*uiScale)) || IsButtonPressed(state.buttons, dd.second.lastState.buttons, GameInputGamepadButtons::GameInputGamepadB))
                    {
                        StopHapticEffect(&dd.second);
                    }
                }

                ImGui::PopID();
                ImGui::Unindent(16.0f);
                ImGui::Dummy(ImVec2(0, 16.0f));
            }

            dd.second.lastState = state;
        }

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 16.0f));

        // don't show Stop All if we have no haptics devices
        auto it = std::find_if(g_devices.begin(), g_devices.end(),
            []( const std::pair<IGameInputDevice* const, DeviceData>& d)
            {
                return d.second.wasapi != nullptr && d.second.xaudio != nullptr;
            }
        );

        if(it != g_devices.end() && g_devices.size() > 1)
        {
            ImGui::Indent(16.0f);
            if(ImGui::Button("Stop All", ImVec2(120*uiScale, 50*uiScale)))
            {
                StopAllHapticEffects();
            }
        }
    }

    ImGui::SetNextWindowPos(ImVec2(710 * uiScale, 5 * uiScale), ImGuiCond_None);
    ImGui::SetNextWindowSize(ImVec2(800 * uiScale, 810 * uiScale), ImGuiCond_None);
    g_appLog->Draw("Log", -200.0f * uiScale);

    ImGui::End();

    // set the main window to focus when the app starts
    if(g_firstDraw)
    {
        ImGui::SetWindowFocus("Advanced Haptics");
        g_firstDraw = false;
    }

}

void Sample_Shutdown()
{
    StopAllHapticEffects();
    g_gameInput->UnregisterCallback(g_deviceCallbackToken);
}

std::wstring OpenWavFileDialog(HWND owner)
{
    wchar_t szFile[MAX_PATH]{};

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"WAV Files (*.wav)\0*.wav\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if(GetOpenFileNameW(&ofn))
    {
        return szFile;
    }
    return {};
}

const char* StringifyDeviceId(_In_ const APP_LOCAL_DEVICE_ID& deviceId) noexcept
{
    static char buffer[APP_LOCAL_DEVICE_ID_SIZE * 2 + 1] = {};

    sprintf_s(
        buffer,
        sizeof(buffer),
        "%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x"
        "%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x"
        "%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x"
        "%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x",
        deviceId.value[0], deviceId.value[1], deviceId.value[2], deviceId.value[3],
        deviceId.value[4], deviceId.value[5], deviceId.value[6], deviceId.value[7],
        deviceId.value[8], deviceId.value[9], deviceId.value[10], deviceId.value[11],
        deviceId.value[12], deviceId.value[13], deviceId.value[14], deviceId.value[15],
        deviceId.value[16], deviceId.value[17], deviceId.value[18], deviceId.value[19],
        deviceId.value[20], deviceId.value[21], deviceId.value[22], deviceId.value[23],
        deviceId.value[24], deviceId.value[25], deviceId.value[26], deviceId.value[27],
        deviceId.value[28], deviceId.value[29], deviceId.value[30], deviceId.value[31]);

    return buffer;
}

bool IsButtonPressed(GameInputGamepadButtons buttons, GameInputGamepadButtons lastButtons, GameInputGamepadButtons button)
{
    return (buttons & button) && !(lastButtons & button);
}

void Sample_Update()
{
}

