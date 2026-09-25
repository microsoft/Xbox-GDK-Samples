//--------------------------------------------------------------------------------------
// GamepadHaptics.cpp
//
// WAV-driven haptic playback through controller audio endpoints.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Gamepad.h"

#ifndef _GAMING_XBOX
#include <commdlg.h>
#endif

//--------------------------------------------------------------------------------------
// InitializeHaptics -- build the media list and initialize the HapticsManager
//
// HapticsManager registers its own callback for GameInputDeviceHapticInfoReady.
//--------------------------------------------------------------------------------------
void Sample::InitializeHaptics()
{
    wchar_t exePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0)
    {
        std::wstring basePath(exePath);
        basePath = basePath.substr(0, basePath.find_last_of(L'\\'));

        auto addMedia = [&](const wchar_t* relPath, const char* title)
        {
            MediaItem mediaItem;
            mediaItem.filename = basePath + L"\\" + relPath;
            mediaItem.title = title;
            m_mediaList.push_back(std::move(mediaItem));
        };

        addMedia(L"media\\Haptics\\AhoogaHorn.wav",  "Ahooga Horn");
        addMedia(L"media\\Haptics\\car.wav",         "Car");
        addMedia(L"media\\Haptics\\fireworks.wav",   "Fireworks");
        addMedia(L"media\\Haptics\\gun_shot.wav",    "Gun Shot");
        addMedia(L"media\\Haptics\\PenScratch.wav",  "Pen Scratch");
        addMedia(L"media\\Haptics\\ShakeEffect.wav", "Shake");
        addMedia(L"media\\Haptics\\TommyGun.wav",    "Tommy Gun");
    }

    m_hapticsManager = std::make_unique<ATG::HapticsManager>();
    HRESULT hr = m_hapticsManager->Initialize(m_gameInput.Get());
    if (FAILED(hr))
    {
        ImGuiAtg::Log("Failed to initialize HapticsManager: %08X\n", static_cast<unsigned int>(hr));
        m_hapticsManager.reset();
    }
    else
    {
        ImGuiAtg::Log("HapticsManager initialized\n");
    }
}

//--------------------------------------------------------------------------------------
// DrawHapticsSection -- UI for selecting and playing haptic audio effects
//
// Plays the selected WAV through the device's WASAPI or XAudio2 haptic endpoint.
//--------------------------------------------------------------------------------------
void Sample::DrawHapticsSection(GamepadDevice& gamepad)
{
    if (ImGui::CollapsingHeader("Haptics", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
    {
        if (!m_hapticsManager)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "HapticsManager not initialized.");
            return;
        }

        const ATG::HapticsDevice* hapticsDevice = m_hapticsManager->GetHapticsDevice(gamepad.device.Get());

        if (!hapticsDevice)
        {
            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "No haptic endpoint found for this device.");
            return;
        }

        ImGuiAtg::BeginNavigationGroup("Haptics");
        ImGui::TextWrapped("Select a haptic effect to play:");
        ImGui::Spacing();

        const char* currentTitle = (gamepad.selectedMediaIndex < m_mediaList.size())
            ? m_mediaList[gamepad.selectedMediaIndex].title.c_str()
            : "Select...";

        if (ImGui::BeginCombo("##Haptics", currentTitle))
        {
            for (size_t i = 0; i < m_mediaList.size(); i++)
            {
                bool isSelected = (gamepad.selectedMediaIndex == i);
                if (ImGui::Selectable(m_mediaList[i].title.c_str(), isSelected))
                {
                    gamepad.selectedMediaIndex = i;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

#ifndef _GAMING_XBOX
        // The Win32 file dialog is unavailable on Xbox.
        ImGui::SameLine();
        if (ImGui::Button("Load WAV..."))
        {
            std::wstring selectedFile = OpenFileDialog(L"WAV Files (*.wav)\0*.wav\0");
            if (!selectedFile.empty())
            {
                MediaItem mediaItem;
                mediaItem.filename = selectedFile;
                mediaItem.title = DX::WideToUtf8(selectedFile);
                m_mediaList.push_back(std::move(mediaItem));
                gamepad.selectedMediaIndex = m_mediaList.size() - 1;
            }
        }
#endif

        bool hasMedia = gamepad.selectedMediaIndex < m_mediaList.size();
        bool isPlaying = hapticsDevice->IsPlaying();

        ImGui::BeginDisabled(!hasMedia || isPlaying);
        if (ImGui::Button("Play WASAPI"))
        {
            hapticsDevice->PlayWAVFile(m_mediaList[gamepad.selectedMediaIndex].filename.c_str(), ATG::HapticPlaybackEngine::WASAPI);
            ImGuiAtg::Log("Playing via WASAPI: %s\n", m_mediaList[gamepad.selectedMediaIndex].title.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Play XAudio2"))
        {
            hapticsDevice->PlayWAVFile(m_mediaList[gamepad.selectedMediaIndex].filename.c_str(), ATG::HapticPlaybackEngine::XAudio2);
            ImGuiAtg::Log("Playing via XAudio2: %s\n", m_mediaList[gamepad.selectedMediaIndex].title.c_str());
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Stop##haptics"))
        {
            hapticsDevice->Stop();
            ImGuiAtg::Log("Haptics stopped\n");
        }

        ImGuiAtg::EndNavigationGroup();
    }
}

#ifndef _GAMING_XBOX
std::wstring Sample::OpenFileDialog(const wchar_t* filter)
{
    wchar_t filePath[MAX_PATH] = {};
    OPENFILENAMEW openFile = {};
    openFile.lStructSize = sizeof(openFile);
    openFile.hwndOwner = m_hWnd;
    openFile.lpstrFile = filePath;
    openFile.nMaxFile = MAX_PATH;
    openFile.lpstrFilter = filter;
    openFile.nFilterIndex = 1;
    openFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&openFile))
        return filePath;
    return {};
}
#endif
