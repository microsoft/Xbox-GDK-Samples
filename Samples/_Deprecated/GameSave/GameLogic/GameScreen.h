// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "InputState.h"
#include "Assets.h"

namespace GameSaveSample
{
    enum class ScreenState
    {
        TransitionOn,
        Active,
        TransitionOff,
        Hidden
    };

    class ScreenManager;

    class GameScreen
    {
    public:
        GameScreen() = delete;
        GameScreen(ScreenManager& screenManager);
        virtual ~GameScreen();

        virtual void LoadContent( ATG::AssetLoadResources& loadRes );
        virtual void UnloadContent();
        virtual void Reset();
        virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen);
        virtual void HandleInput(const DirectX::InputState& inputState);
        virtual void Draw(ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime);
        virtual void ExitScreen(bool immediate = false);

        bool IsPopup() const { return m_isPopup; }
        float TransitionOnTime() const { return m_transitionOnTime; }
        float TransitionOffTime() const { return m_transitionOffTime; }
        float TransitionPosition() const { return m_transitionPosition; }
        float TransitionAlpha() const { return 1.0f - m_transitionPosition; }
        ScreenState State() const { return m_state; }
        bool IsExiting() const { return m_isExiting; }
        bool IsActive() const { return !m_otherScreenHasFocus && (m_state == ScreenState::TransitionOn || m_state == ScreenState::Active); }
        ScreenManager& Manager() { return m_screenManager; }

        // index into cached gamepad collection
        int GetControllingPlayer() const { return m_controllingPlayer; }

    protected:
        bool m_exitWhenHidden;
        bool m_isPopup;
        float m_transitionOnTime;
        float m_transitionOffTime;
        float m_transitionPosition;
        ScreenState m_state;
        std::recursive_mutex m_controllingPlayerLock;

    private:
        void UpdateControllingPlayer(int newControllingPlayer);
        bool UpdateTransition(float elapsedTime, float time, int direction);

        int m_controllingPlayer;
        bool m_isExiting;
        bool m_otherScreenHasFocus;
        ScreenManager& m_screenManager;

        friend class ScreenManager;
    };
}
