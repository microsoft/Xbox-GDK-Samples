//--------------------------------------------------------------------------------------
// Game.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "NetworkMessages.h"
#include "World.h"
#include "PlayerState.h"

namespace PlayFabMultiplayerRumble
{
    // A basic game implementation that creates a D3D11 device and
    // provides a game loop.
    class Game
    {
    public:

        Game() noexcept;
        ~Game();

        // Initialization and management
        void Initialize(HWND window);

        // Basic game loop
        void Tick();

        // Messages
        void OnActivated();
        void OnDeactivated();
        void OnSuspending();
        void OnResuming();

        int GetWindowWidth() { return m_outputWidth; }
        int GetWindowHeight() { return m_outputHeight; }

        void OnWindowMoved() {};
        void OnWindowSizeChanged(int width, int height);

        // Game World
        void StartGame();

        void DrawWorld(float elapsedTime);
        void UpdateWorld(float totalTime, float elapsedTime);

        // Game Player Management
        std::shared_ptr<PlayerState> GetPlayerState(uint64_t peer);
        std::shared_ptr<PlayerState> GetLocalPlayerState();
        void SetPlayerState(uint64_t id, std::shared_ptr<PlayerState> state);
        std::vector<std::shared_ptr<PlayerState>> GetAllPlayerStates();
        void ClearPlayerScores();

        inline std::string_view GetLocalPlayerName() { return m_localPlayerName; }
        inline bool IsGameWon() { return m_world->IsGameWon; }

        void ResetGameplayData();

        inline std::string_view GetWinnerName() { return m_world->WinnerName; }
        inline DirectX::XMVECTORF32 GetWinningColor() { return m_world->WinningColor; }

    private:
        void CleanupUser();
        void MultiplayerInitialize();
        void MultiplayerShutdown();
        void PartyShutdownAsync(std::function<void()> completionCallback);

        void AddHandleToWaitSet(HANDLE handle);
        void WaitForAndCleanupHandles();

        void ProcessGameNetworkMessage(uint64_t sourceId, GameMessage *message);

        void Update(DX::StepTimer const& timer);
        void Render();

        void PrefetchContent();

        uint32_t m_signInCallbackToken;
        uint32_t m_signOutCallbackToken;

        std::vector<HANDLE>                             m_waitHandles;

        // Device resources.
        int                                             m_outputWidth;
        int                                             m_outputHeight;
        std::shared_ptr<DirectX::DescriptorPile>        m_descriptors;

        // World
        std::unique_ptr<World> m_world;

        // Players
        std::string m_localPlayerName;
        std::map<uint64_t, std::shared_ptr<PlayerState>> m_peers;
        
        // Rendering loop timer.
        DX::StepTimer                                   m_timer;
    };
}

extern std::unique_ptr<PlayFabMultiplayerRumble::Game> g_game;
