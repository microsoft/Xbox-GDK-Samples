//--------------------------------------------------------------------------------------
// MainMenuScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MainMenuScreen.h"
#include "GameLobbyScreen.h"
#include "JoinFriendsMenu.h"
#include "OptionsPopUpScreen.h"
#include "Managers.h"
#include "Game.h"

using namespace NetRumble;
using namespace DirectX;

MainMenuScreen::MainMenuScreen() : 
    m_state{ State::Initializing }
{
    m_transitionOnTime = 1.0;
    m_transitionOffTime = 1.0;
    m_exitWhenHidden = false;

    m_title = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\title.png");
}

void MainMenuScreen::HandleInput()
{
    MenuScreen::HandleInput();
}

void MainMenuScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
    MenuScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);

    if (m_state == State::Initializing)
    {
        if (Managers::Get<OnlineManager>()->IsNetworkAvailable())
        {
            MainMenu();
        }
    }

    if (Managers::Get<InputManager>()->IsNewButtonPress(InputManager::GamepadButtons::Y) ||
        Managers::Get<InputManager>()->IsNewKeyPress(Keyboard::Keys::Y))
    {
        auto async = new XAsyncBlock{};

        async->callback = [](XAsyncBlock* async)
        {
            auto hr = XGameUiShowPlayerProfileCardResult(async);

            DEBUGLOG("Show Profile complted with: 0x%x\n", hr);

            delete async;
        };

        auto user = Managers::Get<XboxUserManager>()->GetCurrentUser();
        auto userHandle = *user;

        auto hr = XGameUiShowPlayerProfileCardAsync(async, userHandle, user->Id());
        if (FAILED(hr))
        {
            DEBUGLOG("Show Profile Card failed: 0x%x\n", hr);
            delete async;
        }
    }
}

void MainMenuScreen::Draw(float totalTime, float elapsedTime)
{
    if (IsActive())
    {
        // draw the title texture
        if (m_title.Texture)
        {
            auto renderManager = Managers::Get<RenderManager>();
            auto renderContext = renderManager->GetRenderContext(BlendMode::NonPremultiplied);

            float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
            float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
            float scale = GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
            SimpleMath::Vector2 titlePosition = SimpleMath::Vector2(viewportWidth / 2.0f, (viewportHeight / 2.0f) - (185.f * scale));
            titlePosition.y -= powf(TransitionPosition(), 2) * titlePosition.y;
            SimpleMath::Vector4 color = { 1.0f, 1.0f, 1.f, TransitionAlpha() };

            renderContext->Begin();
            renderContext->Draw(
                m_title,
                titlePosition,
                0.0f,
                scale,
                color);
            renderContext->End();
        }

        MenuScreen::Draw(totalTime, elapsedTime);

        DrawCurrentUser();
    }
}

void NetRumble::MainMenuScreen::Reset()
{
    MainMenu();
}

void MainMenuScreen::OnCancel()
{
    // Can't back out of the main menu
}

void MainMenuScreen::MainMenu()
{
    m_state = State::MainMenu;
    m_menuEntries.clear();

    if (Managers::Get<OnlineManager>()->HasMultiplayerPrivileges() && Managers::Get<OnlineManager>()->HasCrossplayPrivileges())
    {
        m_menuEntries.emplace_back("QUICK PLAY", []()
        {
            Managers::Get<GameStateManager>()->SwitchToState(GameState::MPMatchmaking);
        });

        m_menuEntries.emplace_back("HOST GAME", []()
        {
            Managers::Get<GameStateManager>()->SwitchToState(GameState::MPHostGame);
        });

        m_menuEntries.emplace_back("JOIN GAME", []()
        {
            Managers::Get<ScreenManager>()->AddGameScreen<JoinFriendsMenu>();
        });
    }
    else
    {
        m_menuEntries.emplace_back("Your account does not have multiplayer privileges");
    }

    m_menuEntries.emplace_back("OPTIONS", []()
    {
        Managers::Get<ScreenManager>()->AddGameScreen<OptionsPopUpScreen>();
    });

    ReadCurrentEntry();
}

void MainMenuScreen::Intializing()
{
    m_menuEntries.emplace_back("INITIALIZING");
}
