//--------------------------------------------------------------------------------------
// haptics.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include <commdlg.h>

#include "haptics.h"
#include "HapticsManager/HapticsManager.h"
#include "HapticsManager/Audio/WASAPIManager.h"
#include "HapticsManager/Audio/XAudio2Manager.h"

using namespace Microsoft::WRL;
using namespace ATG;

struct MediaItem
{
    std::wstring filename {};
    std::wstring title {};
};

struct DeviceContext
{
    MediaItem mediaItem;
    GameInputGamepadState lastGamepadState = {};
};

ComPtr<IGameInput> g_gameInput {};
GameInputCallbackToken g_deviceCallbackToken {};

std::unique_ptr<HapticsManager> g_hapticsManager;
std::vector<IGameInputDevice*> g_devices {};
std::map<IGameInputDevice*, DeviceContext> g_deviceContext {};

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
        g_devices.push_back(device);
    }

    // newly disconnected device, remove from our list
    if(wasConnected && !isConnected)
    {
        g_devices.erase(std::remove(g_devices.begin(), g_devices.end(), device), g_devices.end());
    }
}

static void StopAllHapticEffects()
{
    for(auto& giDevice : g_devices)
    {
        auto hd = g_hapticsManager->GetHapticsDevice(giDevice);
        hd->Stop();
    }
}

void Sample::Initialize(HWND hWnd)
{
    m_window = hWnd;

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

    // initialize GameInput and setup callbacks for all gamepad devices
    LOG_IF_FAILED_AND_RETURN(GameInputCreate(&g_gameInput));
    LOG_IF_FAILED_AND_RETURN(g_gameInput->RegisterDeviceCallback(nullptr,
        GameInputKind::GameInputKindGamepad,
        GameInputDeviceStatus::GameInputDeviceAnyStatus,
        GameInputEnumerationKind::GameInputBlockingEnumeration,
        nullptr,
        DeviceCallback, &g_deviceCallbackToken));

    // initialize our HapticsManager class
    g_hapticsManager = std::make_unique<HapticsManager>();
    LOG_IF_FAILED_AND_RETURN(g_hapticsManager->Initialize(g_gameInput.Get()));
}

void Sample::Draw()
{
    ImGuiAtg::BeginFullscreenLayout();

    // Content on left, log on right, with draggable splitter
    ImGuiAtg::BeginSplitV("##LogSplit", 800.0f);

    ImGui::CollapsingHeader("Attached Gamepads", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf);

    if(g_devices.size() == 0)
    {
        ImGui::Text("Please connect a haptics-enabled controller");
    }
    else
    {
        // draw a section for each device attached
        for(auto& giDevice : g_devices)
        {
            ComPtr<IGameInputReading> reading;
            GameInputGamepadState state;
            const GameInputDeviceInfo* di = nullptr;

            // get the current GameInput device info and state
            if(FAILED(g_gameInput->GetCurrentReading(GameInputKindGamepad, giDevice, &reading)))
            {
                continue;
            }
            reading->GetGamepadState(&state);
            LOG_IF_FAILED_AND_RETURN(giDevice->GetDeviceInfo(&di));

            // get the last state for this device that our sample uses
            DeviceContext& context = g_deviceContext[giDevice];

            // get the haptics device for this controller
            const HapticsDevice* hapticsDevice = g_hapticsManager->GetHapticsDevice(giDevice);

            char header[512]{};
            sprintf_s(header, "Name: %s\nVID/PID:  %04X / %04X\nDeviceID: %s", di->displayName, di->vendorId, di->productId, StringifyDeviceId(di->deviceId));
            ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf);
            ImGui::Dummy(ImVec2(0, 16.0f));
            ImGui::Indent(16.0f);
            ImGui::PushID(giDevice);

            if(!hapticsDevice)
            {
                ImGui::Text("No haptic device found for this controller");
            }
            else
            {
                ImGui::Text("Select a haptic effect to play...");
                if(ImGui::BeginCombo("##effect", DX::WideToUtf8(context.mediaItem.title).c_str()))
                {
                    for(auto& mi : g_mediaList)
                    {
                        bool isSelected = (context.mediaItem.filename == mi.filename);
                        if(ImGui::Selectable(DX::WideToUtf8(mi.title).c_str(), &isSelected))
                        {
                            context.mediaItem = mi;
                        }

                        if(isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::Button("Browse for file...", ImVec2(ImGuiAtg::Scaled(160), ImGuiAtg::Scaled(50))))
                {
                    std::wstring selectedFile = OpenWavFileDialog(m_window);
                    if (!selectedFile.empty())
                    {
                        MediaItem mi;
                        mi.filename = selectedFile;
                        mi.title = selectedFile;
                        g_mediaList.push_back(mi);
                        context.mediaItem = mi;
                    }
                }

                ImGui::Dummy(ImVec2(0, 20));

                ImGui::BeginDisabled(hapticsDevice->IsPlaying());
                    if(ImGui::Button("Play WASAPI (L1)", ImVec2(ImGuiAtg::Scaled(160), ImGuiAtg::Scaled(50))) ||
                        (IsButtonPressed(state.buttons, context.lastGamepadState.buttons, GameInputGamepadButtons::GameInputGamepadLeftShoulder) && !hapticsDevice->IsPlaying()))
                    {
                        hapticsDevice->PlayWAVFile(context.mediaItem.filename.c_str(), HapticPlaybackEngine::WASAPI);
                    }
                    ImGui::SameLine();

                    if(ImGui::Button("Play XAudio2 (R1)", ImVec2(ImGuiAtg::Scaled(160), ImGuiAtg::Scaled(50))) ||
                        (IsButtonPressed(state.buttons, context.lastGamepadState.buttons, GameInputGamepadButtons::GameInputGamepadRightShoulder) && !hapticsDevice->IsPlaying()))
                    {
                        hapticsDevice->PlayWAVFile(context.mediaItem.filename.c_str(), HapticPlaybackEngine::XAudio2);
                    }
                ImGui::EndDisabled();

                ImGui::SameLine();

                if (ImGui::Button("Stop (B/Circle)", ImVec2(ImGuiAtg::Scaled(160), ImGuiAtg::Scaled(50))) || IsButtonPressed(state.buttons, context.lastGamepadState.buttons, GameInputGamepadButtons::GameInputGamepadB))
                {
                    hapticsDevice->Stop();
                }
            }

            ImGui::PopID();
            ImGui::Unindent(16.0f);
            ImGui::Dummy(ImVec2(0, 16.0f));

            context.lastGamepadState = state;
        }

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 16));

        // Only show Stop All if we have more than 1 haptics device
        if(g_hapticsManager->GetDeviceCount() > 1)
        {
            ImGui::Indent(16.0f);
            if(ImGui::Button("Stop All", ImVec2(ImGuiAtg::Scaled(120), ImGuiAtg::Scaled(50))))
            {
                StopAllHapticEffects();
            }
        }
    }

    ImGuiAtg::SplitNext();
    ImGuiAtg::DrawLogPanel();
    ImGuiAtg::EndSplit();

    ImGuiAtg::EndFullscreenLayout();
}

void Sample::Shutdown()
{
    StopAllHapticEffects();
    g_gameInput->UnregisterCallback(g_deviceCallbackToken);
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

void Sample::Update()
{
}

