// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "pch.h"

#include "GameScreen.h"
#include "ScreenManager.h"
#include "InputState.h"

using namespace DirectX;

namespace GameSaveSample {

GameScreen::GameScreen(ScreenManager& screenManager) :
    m_exitWhenHidden(true),
    m_isPopup(false),
    m_transitionOnTime(0.0f),
    m_transitionOffTime(0.0f),
    m_transitionPosition(1.0f),
    m_state(ScreenState::TransitionOn),
    m_controllingPlayer(-1),
    m_isExiting(false),
    m_otherScreenHasFocus(false),
    m_screenManager(screenManager)
{
}

GameScreen::~GameScreen()
{
}

void GameScreen::LoadContent( ATG::AssetLoadResources& /*unused: loadRes*/ )
{
    // Nothing for the base class to load
}

void GameScreen::UnloadContent()
{
    // Nothing for the base class to unload
}

void GameScreen::Reset()
{
    // Nothing for the base class to reset
}

void GameScreen::Update(float /*totalTime*/, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
    m_otherScreenHasFocus = otherScreenHasFocus;

    if (m_isExiting)
    {
        m_state = ScreenState::TransitionOff;
        if (!UpdateTransition(elapsedTime, m_transitionOffTime, 1))
        {
            // When the transition finishes, remove the screen.
            m_screenManager.RemoveScreen(this);
        }
    }
    else if (coveredByOtherScreen)
    {
        if (m_exitWhenHidden)
        {
            // If the screen is covered by another, it should transition off.
            if (UpdateTransition(elapsedTime, m_transitionOffTime, 1))
            {
                // Still busy transitioning.
                m_state = ScreenState::TransitionOff;
            }
            else
            {
                // Transition finished!
                m_state = ScreenState::Hidden;
                m_screenManager.RemoveScreen(this);
            }
        }
        else
        {
            m_state = ScreenState::Hidden;
        }
    }
    else
    {
        // Otherwise the screen should transition on and become active.
        if (UpdateTransition(elapsedTime, m_transitionOnTime, -1))
        {
            // Still busy transitioning.
            m_state = ScreenState::TransitionOn;
        }
        else
        {
            // Transition finished!
            m_state = ScreenState::Active;
        }
    }
}

void GameScreen::HandleInput(const DirectX::InputState&)
{
    // Nothing for the base class to handle
}

void GameScreen::Draw( ID3D12GraphicsCommandList* /*unused:commandList*/, float /*unused:totalTime*/, float /*unused:elapsedTimefloat*/ )
{
   // This is called AFTER child classes take a crack at drawing.

   // Nothing for the base class to draw
}

void GameScreen::ExitScreen(bool immediate)
{
    if (immediate || m_transitionOffTime == 0.0f)
    {
        // If the screen has a zero transition time, remove it immediately.
        m_screenManager.RemoveScreen(this);
    }
    else
    {
        // Otherwise flag that it should transition off and then exit.
        m_isExiting = true;
    }
}

void GameScreen::UpdateControllingPlayer(int newControllingPlayer)
{
   std::lock_guard<std::recursive_mutex> lock(m_controllingPlayerLock);
    m_controllingPlayer = newControllingPlayer;
}

bool GameScreen::UpdateTransition(float elapsedTime, float time, int direction)
{
    // How much should we move by?
    float transitionDelta = 0.0f;

    if (time == 0.0f)
    {
        transitionDelta = 1;
    }
    else
    {
        transitionDelta = elapsedTime / time;
    }

    // Update the transition position.
    m_transitionPosition += transitionDelta * direction;

    // Did we reach the end of the transition?
    if (((direction < 0) && (m_transitionPosition <= 0)) ||
        ((direction > 0) && (m_transitionPosition >= 1)))
    {
        m_transitionPosition = std::min(std::max(m_transitionPosition, 0.0f), 1.0f);
        return false;
    }

    // Otherwise we are still busy transitioning.
    return true;
}
} // namespace GameSaveSample
