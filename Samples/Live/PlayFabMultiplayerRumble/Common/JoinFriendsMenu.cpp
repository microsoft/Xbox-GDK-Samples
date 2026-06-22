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

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

JoinFriendsMenu::JoinFriendsMenu()
{
    m_transitionOnTime = 1.0;
    m_transitionOffTime = 1.0;

    std::vector<uint64_t> friendsPlayingThisGame = Managers::Get<FriendsManager>()->GetFriendsInGame();

    if (friendsPlayingThisGame.empty() == false)
    {
        ATG::XboxHandle<XblContextHandle> xblContext = Managers::Get<OnlineManager>()->GetXboxLiveContext();

        Managers::Get<MPAManager>()->GetActivities(xblContext, friendsPlayingThisGame, [this](bool bSuccess, std::vector<std::shared_ptr<OnlineUser>> &users)
        {
            if (bSuccess)
            {
                m_friendsGames = users;

                std::vector<uint64_t> xuids;

                for (const auto& user : m_friendsGames)
                {
                    if (user->Name.empty())
                    {
                        xuids.push_back(user->Id);
                    }
                }

                if (xuids.empty() == false)
                {
                    FriendsManager* friendsManager = Managers::Get<FriendsManager>();
                    Managers::Get<FriendsManager>()->ReadUserDisplayNamesAsync(xuids.data(), xuids.size(), [this, friendsManager](bool bSuccess, const std::vector<uint64_t>&)
                    {
                        if (bSuccess)
                        {
                            for (const auto& user : m_friendsGames)
                            {
                                user->Name = friendsManager->GetUserDisplayName(user->Id);
                            }
                        }
                        else
                        {
                            DEBUGLOG("JoinFriendsMenu::JoinFriendsMenu: Failed to read display names for friends");
                        }
                    });
                }
            }
            else
            {
                DEBUGLOG("JoinFriendsMenu::JoinFriendsMenu: failed to get activities for friends");
            }
        });
    }
}

void JoinFriendsMenu::Draw(float totalTime, float elapsedTime)
{
    UNREFERENCED_PARAMETER(totalTime);
    UNREFERENCED_PARAMETER(elapsedTime);

    m_menuEntries.clear();

    if (m_friendsGames.empty())
    {
        m_menuEntries.emplace_back("Loading Friends' Games");
    }
    else
    {
        for (size_t i = 0; i < 5 && i < m_friendsGames.size(); ++i)
        {
            auto host = m_friendsGames[i];

            if (XboxLiveOnlineUser* xblUser = host ? static_cast<XboxLiveOnlineUser*>(host.get()) : nullptr)
            {
                if (xblUser->Name.empty() == false)
                {
                    std::string connectionString = xblUser->connectionString;
                    m_menuEntries.emplace_back(xblUser->Name, [connectionString]()
                    {
                        Managers::Get<OnlineManager>()->JoinMultiplayerGame(connectionString);
                    });
                }
            }
        }
    }

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

