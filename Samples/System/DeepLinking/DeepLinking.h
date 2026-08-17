//--------------------------------------------------------------------------------------
// DeepLinking.h
//
// Header for sample
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <XTaskQueue.h>

namespace ImGuiAtg { class DeviceContext; }

class Sample
{
public:
    enum class GameMode
    {
        None,
        Lobby,
        Campaign,
        Shop,
    };

    Sample() = default;
    ~Sample() = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    void Initialize(HWND hWnd);
    void Update();
    void Draw();
    void Shutdown();
    void Activated();
    void Deactivated();
    LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifdef _GAMING_XBOX
    void Suspend(ImGuiAtg::DeviceContext* dc);
    void Resume(ImGuiAtg::DeviceContext* dc);
#endif

private:
    void NavigateTo(const std::string& uri, const std::string& path, const std::map<std::string, std::string>& queryStrings);

    // The lookup table is shared across instances and never mutated after construction.
    static inline const std::unordered_map<std::string, GameMode> s_pathToGameMode
    {
        { "none",     GameMode::None },
        { "lobby",    GameMode::Lobby },
        { "campaign", GameMode::Campaign },
        { "shop",     GameMode::Shop },
    };

    XTaskQueueRegistrationToken m_activationToken{};
    std::string m_uri;
    std::string m_path;
    std::map<std::string, std::string> m_queryStrings;
    GameMode m_mode = GameMode::None;
};
