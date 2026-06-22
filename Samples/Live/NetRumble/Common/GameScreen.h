//--------------------------------------------------------------------------------------
// GameScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{
    enum class ScreenStateType
    {
        TransitionOn,
        Active,
        TransitionOff,
        Hidden
    };

    class GameScreen
    {
    public:
        GameScreen();
        virtual ~GameScreen();

        virtual void LoadContent();
        virtual void Reset();
        virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen);
        virtual void HandleInput();
        virtual void Draw(float totalTime, float elapsedTime);
        virtual void ExitScreen(bool immediate = false);
        virtual void ReadCurrentEntry();

        bool IsPopup() const { return m_isPopup; }
        float TransitionOnTime() const { return m_transitionOnTime; }
        float TransitionOffTime() const { return m_transitionOffTime; }
        float TransitionPosition() const { return m_transitionPosition; }
        float TransitionAlpha() const { return 1.0f - m_transitionPosition; }
        ScreenStateType State() const { return m_state; }
        bool IsExiting() const { return m_isExiting; }
        bool IsActive() const { return !m_otherScreenHasFocus && (m_state == ScreenStateType::TransitionOn || m_state == ScreenStateType::Active); }

    protected:
        bool m_exitWhenHidden;
        bool m_isPopup;
        float m_transitionOnTime;
        float m_transitionOffTime;
        float m_transitionPosition;
        ScreenStateType m_state;

    private:
        bool UpdateTransition(float elapsedTime, float time, int direction);

        int m_controllingPlayer;
        bool m_isExiting;
        bool m_otherScreenHasFocus;

        friend class ScreenManager;
    };

    inline float GetScaleMultiplierForViewport(float viewportWidth, float viewportHeight)
    {
        static constexpr float referenceWidth = 1920.f;
        static constexpr float referenceHeight = 1080.f;
        static constexpr float referenceAspectRatio = referenceWidth / referenceHeight;
        float aspectRatio = viewportWidth / viewportHeight;

        if (aspectRatio <= referenceAspectRatio)
        {
            return viewportWidth / referenceWidth;
        }
        else // aspectRatio > referenceAspectRatio
        {
            return viewportHeight / referenceHeight;
        }
    }
}
