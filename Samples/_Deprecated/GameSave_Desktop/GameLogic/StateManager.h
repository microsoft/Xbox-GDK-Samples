// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once
#include "..\Helpers\AsyncAction.h"
#include "..\Helpers\ScopedLockWrappers.h"
#include "..\Helpers\User.h"
#include "LiveResources.h"
#include "ScreenManager.h"

namespace GameSaveSample
{
    enum class GameState
    {
        Reset,
        Initialize,
        AcquireUser,
        InitializeGameSaveSystem,
        InGame,
        Suspended,
        Resume
    };

    const char* StateToString(GameState state);

    class StateManager 
    {
    public:
        StateManager(ScreenManager& screenManager);

        void Suspend();
        void Resume();
        void ResetGame();
        void Update();

        void PopupError(const char* errorMsg, bool revertToPriorState = false);
        bool IsCurrentOrPendingState(GameState state);
        void RevertToPriorState();
        void SwitchState(GameState newState);

        GameState GetState() const noexcept
        { 
           return m_state; 
        }

        bool IsInGame() const noexcept
        {
           return m_state == GameState::InGame;
        }

    private:
        ATG::AsyncAction< void >& InitializeGameSaveSystem() noexcept;
        void InitializeGameSaveSystemTask() noexcept;

    public:
        // Event Handlers
        void OnSignOutStarted( std::shared_ptr<ATG::User>& user);
        void OnSignOutStartedTaskBody();
        void OnUserChanged( std::shared_ptr<ATG::User>& oldUser, std::shared_ptr<ATG::User>& newUser );

    private:

        // Private Member Data
        std::recursive_mutex m_lock;
        ScreenManager&  m_screenManager;

        GameState m_state;
        GameState m_pendingState;
        GameState m_priorState;

        ATG::AsyncAction< void > m_asyncSignOutTask;
        ATG::AsyncAction< void > m_initSaveGameSystemTask;

        XUserSignOutDeferralHandle m_deferral = nullptr;
    };
}
