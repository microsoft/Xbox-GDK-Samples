//--------------------------------------------------------------------------------------
// MainMenuScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UserStartupScreen.h"
#include "GameLobbyScreen.h"
#include "JoinFriendsMenu.h"
#include "OptionsPopUpScreen.h"
#include "Managers.h"
#include "Game.h"
#include "SampleConfig.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

UserStartupScreen::UserStartupScreen() :
    m_state{ State::Initializing }
{
    m_transitionOnTime = 1.0;
    m_transitionOffTime = 1.0;
    m_exitWhenHidden = false;

    m_title = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\title.png");

    auto signInCallback = [this](const User&) { WaitForNetworkInitialization(); };
    m_signInCallbackToken = Managers::Get<XboxUserManager>()->AddUserSignedInCallback(signInCallback);
}

void UserStartupScreen::ExitScreen(bool immediate)
{
    Managers::Get<XboxUserManager>()->RemoveUserSignedInCallback(m_signInCallbackToken);

    MenuScreen::ExitScreen(immediate);
}

void UserStartupScreen::HandleInput()
{
    MenuScreen::HandleInput();
}

void UserStartupScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
    MenuScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);

    if (m_state == State::Initializing && Managers::Get<OnlineManager>()->IsNetworkAvailable())
    {
        m_state = State::AcquireUser;

        if (SampleConfig::c_AutoLogIn)
        {
            //For debugging purposes
            BeginUserSignInProcess();
        }
        else
        {
            auto callback = [this]()
            {
                BeginUserSignInProcess();
            };

            m_menuEntries.emplace_back("PRESS A BUTTON", callback);
        }
    }
}

void UserStartupScreen::Draw(float totalTime, float elapsedTime)
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
    }
}

void UserStartupScreen::Reset()
{
    m_state = State::Initializing;
    m_menuEntries.clear();
}

void UserStartupScreen::OnCancel()
{
    // Can't back out of the start menu
}


void UserStartupScreen::BeginUserSignInProcess()
{
    m_state = State::AcquireUser;
    m_menuEntries.clear();
    m_menuEntries.emplace_back("SIGNING IN...");

    Managers::Get<XboxUserManager>()->SignIn();
}

void UserStartupScreen::Intializing()
{
    m_menuEntries.emplace_back("INITIALIZING...");
}

void UserStartupScreen::WaitForNetworkInitialization()
{
    auto onlineMgr = Managers::Get<OnlineManager>();
    if (!onlineMgr->IsNetworkAvailable())
    {
        onlineMgr->SetNetworkAvailableCallback(
            [this, onlineMgr]()
            {
                onlineMgr->SetNetworkAvailableCallback(nullptr);
                SetPlayFabPartyUser();
            });
    }
    else
    {
        SetPlayFabPartyUser();
    }
}

void UserStartupScreen::SetPlayFabPartyUser()
{
    Managers::Get<PlayFabPartyManager>()->Initialize();
    Managers::Get<PlayFabPartyManager>()->SetLocalUser(
        Managers::Get<XboxUserManager>()->GetCurrentUser()->Id(),
        [this](PartyError err)
        {
            if (PARTY_SUCCEEDED(err))
            {
                Managers::Get<OnlineManager>()->SetEntityToken();
                Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
            }
            else
            {
                Reset();
                Managers::Get<XboxUserManager>()->ClearCurrentUser();
                Managers::Get<ScreenManager>()->ShowError("Unable to sign in to PlayFab Party");
            }
        });
}
