//--------------------------------------------------------------------------------------
// GameScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Managers.h"
#include "GameScreen.h"
#include "OptionsPopUpScreen.h"

using namespace NetRumble;
using namespace DirectX;

GameScreen::GameScreen() :
    m_exitWhenHidden(true),
    m_isPopup(false),
    m_transitionOnTime(0.0f),
    m_transitionOffTime(0.0f),
    m_transitionPosition(1.0f),
    m_state(ScreenStateType::TransitionOn),
    m_controllingPlayer(-1),
    m_isExiting(false),
    m_otherScreenHasFocus(false)
{
}

GameScreen::~GameScreen() = default;

void GameScreen::LoadContent()
{
    // Nothing for the base class to load
}

void GameScreen::Reset()
{
    // Nothing for the base class to reset
}

void GameScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
    UNREFERENCED_PARAMETER(totalTime);

    m_otherScreenHasFocus = otherScreenHasFocus;

    if (m_isExiting)
    {
        m_state = ScreenStateType::TransitionOff;
        if (!UpdateTransition(elapsedTime, m_transitionOffTime, 1))
        {
            // When the transition finishes, remove the screen.
            Managers::Get<ScreenManager>()->RemoveScreen(this);
        }
    }
    else if (coveredByOtherScreen)
    {
        // If the screen is covered by another, it should transition off.
        if (UpdateTransition(elapsedTime, m_transitionOffTime, 1))
        {
            // Still busy transitioning.
            m_state = ScreenStateType::TransitionOff;
        }
        else
        {
            // Transition finished!
            m_state = ScreenStateType::Hidden;
            if (m_exitWhenHidden)
            {
                Managers::Get<ScreenManager>()->RemoveScreen(this);
            }
        }
    }
    else
    {
        // Otherwise the screen should transition on and become active.
        if (UpdateTransition(elapsedTime, m_transitionOnTime, -1))
        {
            // Still busy transitioning.
            m_state = ScreenStateType::TransitionOn;
        }
        else
        {
            // Transition finished!
            m_state = ScreenStateType::Active;
        }
    }
}

void GameScreen::HandleInput()
{
    // Nothing for the base class to handle
    auto inputManager = Managers::Get<InputManager>();

    if (inputManager->IsNewKeyPress(DirectX::Keyboard::OemTilde))
    {
        Managers::Get<ScreenManager>()->SetForegroundsVisible(!Managers::Get<ScreenManager>()->GetForegroundsVisible());
    }
    else if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::Menu) || inputManager->IsNewKeyPress(DirectX::Keyboard::Tab))
    {
        Managers::Get<ScreenManager>()->AddGameScreen<OptionsPopUpScreen>();
    }

    if (Managers::Get<PlayFabPartyManager>()->IsConnected())
    {
        if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::View) || inputManager->IsNewKeyPress(DirectX::Keyboard::Space))
        {
            auto async = new XAsyncBlock{};

            async->callback = [](XAsyncBlock* async)
            {
                uint32_t textSize = 0;

                auto hr = XGameUiShowTextEntryResultSize(async, &textSize);
                if (SUCCEEDED(hr))
                {
                    std::string resultText;

                    resultText.reserve(textSize + 1);

                    hr = XGameUiShowTextEntryResult(async, textSize, resultText.data(), nullptr);
                    if (SUCCEEDED(hr))
                    {
                        if (Managers::Get<PlayFabPartyManager>()->IsCognitiveServicesEnabled())
                        {
                            Managers::Get<PlayFabPartyManager>()->SendTextAsVoice(resultText);
                        }

                        Managers::Get<PlayFabPartyManager>()->SendTextMessage(resultText);
                    }
                }

                delete async;
            };

            auto hr = XGameUiShowTextEntryAsync(
                async,
                nullptr,
                nullptr,
                nullptr,
                XGameUiTextEntryInputScope::Alphanumeric,
                255);

            if (FAILED(hr))
            {
                delete async;
            }
        }
        else if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::DPadUp))
        {
            Managers::Get<PlayFabPartyManager>()->SendTextMessage("Great job!");
        }
        else if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::DPadLeft))
        {
            Managers::Get<PlayFabPartyManager>()->SendTextMessage("Help!");
        }
        else if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::DPadRight))
        {
            Managers::Get<PlayFabPartyManager>()->SendTextMessage("Look out!");
        }
        else if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::DPadDown))
        {
            Managers::Get<PlayFabPartyManager>()->SendTextMessage("Hello!");
        }
    }
}

void GameScreen::Draw(float, float)
{
    // Nothing for the base class to draw
}

void GameScreen::ReadCurrentEntry()
{
}

void GameScreen::ExitScreen(bool immediate)
{
    if (immediate || m_transitionOffTime == 0.0f)
    {
        // If the screen has a zero transition time, remove it immediately.
        Managers::Get<ScreenManager>()->RemoveScreen(this);
    }
    else
    {
        // Otherwise flag that it should transition off and then exit.
        m_isExiting = true;
    }
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
