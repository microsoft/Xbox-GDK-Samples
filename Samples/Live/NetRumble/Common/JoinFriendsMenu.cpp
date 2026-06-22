//--------------------------------------------------------------------------------------
// ErrorScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "JoinFriendsMenu.h"
#include "MainMenuScreen.h"
#include "Managers.h"
#include "Game.h"

using namespace NetRumble;
using namespace DirectX;

JoinFriendsMenu::JoinFriendsMenu()
{
    m_transitionOnTime = 1.0;
    m_transitionOffTime = 1.0;

    auto asyncManager = Managers::Get<AsyncTaskManager>();

    Managers::Get<OnlineManager>()->GetInGameFriendsAsync(asyncManager->GetDefaultQueue().get(), [this](std::vector<std::shared_ptr<OnlineUser>> &users) 
    {
        m_friendsGames = users;
    });
}

void JoinFriendsMenu::Draw(float totalTime, float elapsedTime)
{
    UNREFERENCED_PARAMETER(totalTime);
    UNREFERENCED_PARAMETER(elapsedTime);

    m_menuEntries.clear();

    if (m_friendsGames.empty())
    {
        m_menuEntries.push_back(MenuEntry{ "Loading Friends' Games" });
    }
    else
    {
        for (size_t i = 0; i < 5 && i < m_friendsGames.size(); ++i)
        {
            auto user = m_friendsGames[i];
            m_menuEntries.push_back(MenuEntry(user->Name, [user]()
            {
                Managers::Get<OnlineManager>()->JoinMultiplayerGame(user.get());
            }));
        }
    }

    ReadCurrentEntry();

    auto renderContext = Managers::Get<RenderManager>()->GetRenderContext();

    auto contentManager = Managers::Get<ContentManager>();

    auto spriteFont = contentManager->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");
    float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
    float viewportHeight = static_cast<float>(g_game->GetWindowHeight());

    // //calculate position and size of error message
    auto errorMsgColor = Colors::Yellow;
    float scale = 0.5f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

    renderContext->Begin();

    // draw a background color for the rectangle
    auto color = Colors::DarkSlateGray;
    color.f[3] = .25f;

    if (m_state == ScreenStateType::Active)
    {
        // draw error message in the middle of the screen
        SimpleMath::Vector2 errorMsgPosition = SimpleMath::Vector2(0, viewportHeight / 2.0f - 10);
        XMVECTOR size = spriteFont->MeasureString("Select Friend to Join");
        errorMsgPosition.x = viewportWidth / 2.0f - XMVectorGetX(size) / 2.0f * scale;
        renderContext->DrawString(spriteFont, "Select Friend to Join", errorMsgPosition, errorMsgColor, 0, DirectX::XMFLOAT2{ 0,0 }, scale);
    }
    renderContext->End();

    MenuScreen::Draw(totalTime, elapsedTime);

    DrawCurrentUser();
}

void JoinFriendsMenu::OnCancel()
{
    ExitScreen(false);
}

