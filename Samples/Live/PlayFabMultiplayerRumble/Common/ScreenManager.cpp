//--------------------------------------------------------------------------------------
// ScreenManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Game.h"
#include "IManager.h"
#include "Managers.h"
#include "GameScreen.h"
#include "ErrorScreen.h"
#include "STTOverlayScreen.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

ScreenManager::ScreenManager() noexcept :
    m_screenStackCleared(false),
    m_backgroundsVisible(true),
    m_foregroundsVisible(false)
{
    m_STTWindow = std::make_shared<STTOverlayScreen>();
}


ScreenManager::~ScreenManager() noexcept
{
}

void ScreenManager::CreateWindowSizeDependentResources()
{
    for (auto& screen : m_backgroundScreens)
    {
        screen->Reset();
    }

    for (auto& screen : m_gameScreens)
    {
        screen->Reset();
    }

    for (auto& screen : m_foregroundScreens)
    {
        screen->Reset();
    }

#ifndef _XBOX_ONE
//    m_spriteBatch->SetRotation(m_deviceResources->ComputeDisplayRotation());
#endif
}

void ScreenManager::ExitAllScreens()
{
    AddPendingGameScreens();

    while (!m_gameScreens.empty())
    {
        auto& screen = m_gameScreens.back();
        screen->ExitScreen(true);
        RemovePendingGameScreens();
    }

    m_screenStackCleared = true;
}

void ScreenManager::AddGameScreen(std::unique_ptr<GameScreen> screen)
{
    m_pendingGameScreensAdds.emplace_back(std::move(screen));
}

//void ScreenManager::RemoveScreen(std::uniqu_ptr<GameScreen>& screen)
//{
//    // Shell out to our private helper since it's going to do the same thing
//    RemoveScreen(screen.get());
//}

GameScreen* PlayFabMultiplayerRumble::ScreenManager::GetCurrentGameScreen() const
{
    if (!m_gameScreens.empty())
    {
        return m_gameScreens[0].get();
    }

    return nullptr;
}

void PlayFabMultiplayerRumble::ScreenManager::ShowError(std::string_view message, std::function<void(void)> andthen)
{
    AddGameScreen<ErrorScreen>(message, andthen);
}

void ScreenManager::AddBackgroundScreen(std::unique_ptr<GameScreen> screen)
{
    std::lock_guard<std::mutex> lock(m_screenLock);

    screen->m_isExiting = false;
    screen->m_state = ScreenStateType::Active;

    // Allow the screen to load content now that it's being added to the screen manager
    screen->LoadContent();

    m_backgroundScreens.push_back(std::move(screen));
}

void ScreenManager::AddForegroundScreen(std::unique_ptr<GameScreen> screen)
{
    std::lock_guard<std::mutex> lock(m_screenLock);

    screen->m_isExiting = false;
    screen->m_state = ScreenStateType::Active;

    // Allow the screen to load content now that it's being added to the screen manager
    screen->LoadContent();

    m_foregroundScreens.push_back(std::move(screen));
}

void ScreenManager::Update(DX::StepTimer const& timer)
{
    float totalTime = float(timer.GetTotalSeconds());
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: review
    //if (Game->InputManager->IsNewButtonPress(GamepadButtons::View, -1) || Game->InputManager->IsNewKeyPress(Keyboard::Keys::OemTilde))
    //{
    //    isDebugOverlayOn = !isDebugOverlayOn;
    //}

    bool otherScreenHasFocus = false;
    bool coveredByOtherScreen = false;
    
    m_screenStackCleared = false;

    // Draw backgrounds first
    if (m_backgroundsVisible)
    {
        for (ReverseScreensIterator itr = m_backgroundScreens.rbegin(); itr != m_backgroundScreens.rend(); itr++)
        {
            (*itr)->Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);
        }
    }

    // Iterate the screens in reverse order so the last screen added is considered the top of the "stack"
    for (ReverseScreensIterator itr = m_gameScreens.rbegin(); itr != m_gameScreens.rend(); itr++)
    {
        auto &screen = (*itr);

        // Update the screen
        screen->Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);

        if (screen->m_state == ScreenStateType::TransitionOn || screen->m_state == ScreenStateType::Active)
        {
            // If this is the first active screen we came across, give it a chance to handle input
            if (!otherScreenHasFocus)
            {
                screen->HandleInput();
                otherScreenHasFocus = true;
            }

            // If this is an active non-popup, inform any subsequent screens that they are covered by it
            if (!screen->m_isPopup)
            {
                coveredByOtherScreen = true;
            }
        }

        if (m_screenStackCleared)
        {
            // ExitAllScreens() was called by the current screen so don't process any more screens
            break;
        }
    }

    AddPendingGameScreens();
    RemovePendingGameScreens();

    // Draw foregrounds last
    if (m_foregroundsVisible)
    {
        for (ReverseScreensIterator itr = m_foregroundScreens.rbegin(); itr != m_foregroundScreens.rend(); itr++)
        {
            (*itr)->Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);
        }
    }

    // STT Window on top
    m_STTWindow->Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);
}

void ScreenManager::Suspend()
{
    ExitAllScreens();
}

void ScreenManager::Resume()
{
}

void ScreenManager::Render(DX::StepTimer const& timer)
{
    float totalTime = float(timer.GetTotalSeconds());
    float elapsedTime = float(timer.GetElapsedSeconds());

    if (m_backgroundsVisible)
    {
        for (auto& screen : m_backgroundScreens)
        {
            screen->Draw(totalTime, elapsedTime);
        }
    }

    for (auto& screen : m_gameScreens)
    {
        if (screen->m_state != ScreenStateType::Hidden)
        {
            screen->Draw(totalTime, elapsedTime);
        }
    }

    if (m_foregroundsVisible)
    {
        for (auto& screen : m_foregroundScreens)
        {
            screen->Draw(totalTime, elapsedTime);
        }
    }

    m_STTWindow->Draw(totalTime, elapsedTime);
}

void ScreenManager::RemoveScreen(GameScreen* screen)
{
    m_pendingGameScreensRemovals.push_back(screen);
}

void ScreenManager::AddPendingGameScreens()
{
    std::lock_guard<std::mutex> lock(m_screenLock);

    for (auto &screen : m_pendingGameScreensAdds)
    {
        screen->m_isExiting = false;

        // Allow the screen to load content now that it's being added to the screen manager
        screen->LoadContent();

        if (!screen->IsPopup() && m_gameScreens.size() > 0)
        {
            // insert the new screen underneath any existing popup screens
            ScreensIterator it = m_gameScreens.begin();
            for (; it != m_gameScreens.end(); ++it)
            {
                if ((*it)->IsPopup())
                    break;
            }
            m_gameScreens.insert(it, std::move(screen));
        }
        else
        {
            // otherwise just tack on the new screen at the end (top) of the list
            m_gameScreens.emplace_back(std::move(screen));
        }
    }

    m_pendingGameScreensAdds.clear();
}

void ScreenManager::RemovePendingGameScreens()
{
    std::lock_guard<std::mutex> lock(m_screenLock);

    for (auto screen : m_pendingGameScreensRemovals)
    {
        bool found = false;
        for (ScreensIterator itr = m_gameScreens.begin(); itr != m_gameScreens.end(); ++itr)
        {
            if ((*itr).get() == screen)
            {
                m_gameScreens.erase(itr);
                found = true;
                break;
            }
        }

        if (found) continue;

        for (ScreensIterator itr = m_backgroundScreens.begin(); itr != m_backgroundScreens.end(); ++itr)
        {
            if ((*itr).get() == screen)
            {
                m_backgroundScreens.erase(itr);
                found = true;
                break;
            }
        }

        if (found) continue;

        for (ScreensIterator itr = m_foregroundScreens.begin(); itr != m_foregroundScreens.end(); ++itr)
        {
            if ((*itr).get() == screen)
            {
                m_foregroundScreens.erase(itr);
                found = true;
                break;
            }
        }
    }

    m_pendingGameScreensRemovals.clear();
}

