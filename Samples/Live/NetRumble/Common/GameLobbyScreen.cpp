//--------------------------------------------------------------------------------------
// GameLobbyScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameLobbyScreen.h"
#include "GamePlayScreen.h"
#include "Managers.h"
#include "PlayerState.h"
#include "Game.h"

using namespace NetRumble;
using namespace DirectX;

const char* gameSessionInstructions = "Press X to toggle ready, Y to invite, LB/RB to change ship color/style, Press B to exit";

GameLobbyScreen::GameLobbyScreen() noexcept :
    m_ready{false},
    m_exiting{false},
    m_countdownTimer{5.0f},
    m_lastState{ GameState::Initialize }
{
    m_transitionOnTime = 1.0;
    m_transitionOffTime = 1.0;

    Managers::Get<AudioManager>()->PlaySoundTrack(false);
    
    m_inGameTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\Xbox_One_Controller_Front.png");
    m_readyTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\ready.png");

    Managers::Get<ScreenManager>()->SetBackgroundsVisible(true);
}

void GameLobbyScreen::HandleInput()
{
    if (!m_exiting)
    {
        MenuScreen::HandleInput();

        if (Managers::Get<GameStateManager>()->GetState() != GameState::Lobby)
        {
            return;
        }

        auto inputManager = Managers::Get<InputManager>();
        auto localPlayerState = g_game->GetLocalPlayerState();

        if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::LeftShoulder) || inputManager->IsNewKeyPress(Keyboard::Keys::Left))
        {
            // change ship color
            byte newColor = (localPlayerState->ShipColor() + 1) % Ship::Colors.size();
            localPlayerState->ShipColor(newColor);

            Managers::Get<OnlineManager>()->SendGameMessage(
                GameMessage(
                    GameMessageType::PlayerState,
                    localPlayerState->SerializePlayerStateData()
                )
            );

        }
        else if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::RightShoulder) || inputManager->IsNewKeyPress(Keyboard::Keys::Right))
        {
            // change ship design
            byte newShip = (localPlayerState->ShipVariation() + 1) % Ship::MaxVariations;
            localPlayerState->ShipVariation(newShip);

            Managers::Get<OnlineManager>()->SendGameMessage(
                GameMessage(
                    GameMessageType::PlayerState,
                    localPlayerState->SerializePlayerStateData()
                )
            );
        }
        else if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::Y) || inputManager->IsNewKeyPress(Keyboard::Keys::Y))
        {
            Managers::Get<OnlineManager>()->ShowInviteUI();
        }
    }
}

void GameLobbyScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
    auto state = Managers::Get<GameStateManager>()->GetState();

    if (!m_exiting)
    {
        m_menuEntries.clear();

        switch (state)
        {
        case GameState::MPHostGame:
            m_menuEntries.emplace_back("Starting host session");
            break;
        case GameState::MPMatchmaking:
            m_menuEntries.emplace_back("Matchmaking: Waiting for players");
            break;
        case GameState::MPMatchmakingCancel:
            m_menuEntries.emplace_back("Matchmaking: Cancelling...");
            break;
        case GameState::ExitingGame:
            m_menuEntries.emplace_back("Leaving game...");
            break;
        case GameState::MPJoinGame:
            m_menuEntries.emplace_back("Joining Game");
            break;
        case GameState::Lobby:
            if (m_lastState != state)
            {
                Managers::Get<OnlineManager>()->SendGameMessage(
                    GameMessage(
                        GameMessageType::PlayerInfo,
                        g_game->GetLocalPlayerState()->DisplayName
                    )
                );

                Managers::Get<OnlineManager>()->SendGameMessage(
                    GameMessage(
                        GameMessageType::PlayerState,
                        g_game->GetLocalPlayerState()->SerializePlayerStateData()
                    )
                );
            }

            if (m_ready)
            {
                auto players = g_game->GetAllPlayerStates();

                std::string message = players.size() == 1 ? "Waiting for more players" : "Waiting for game to start";

                m_menuEntries.emplace_back(message,[this]()
                {
                    m_ready = false;
                    g_game->GetLocalPlayerState()->LobbyReady = false;

                    Managers::Get<OnlineManager>()->SendGameMessage(
                        GameMessage(
                            GameMessageType::PlayerState,
                            g_game->GetLocalPlayerState()->SerializePlayerStateData()
                        )
                    );
                });
            }
            else
            {
                m_menuEntries.emplace_back("Press (A) to ready", [this]()
                {
                    m_ready = true;
                    g_game->GetLocalPlayerState()->LobbyReady = true;

                    Managers::Get<OnlineManager>()->SendGameMessage(
                        GameMessage(
                            GameMessageType::PlayerState,
                            g_game->GetLocalPlayerState()->SerializePlayerStateData()
                        )
                    );
                });
            }


            if (Managers::Get<OnlineManager>()->IsHost())
            {
                auto allReady = true;
                auto players = g_game->GetAllPlayerStates();

                if (players.size() > 1)
                {
                    for (auto player : players)
                    {
                        if (!player->LobbyReady)
                        {
                            allReady = false;
                            break;
                        }
                    }
                }
                else
                {
                    allReady = false;
                }

                if (allReady)
                {
                    Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::GameCountdown, 0));
                    Managers::Get<GameStateManager>()->SwitchToState(GameState::StartingGame);
                }
            }
            break;
        case GameState::StartingGame:
            {
                m_countdownTimer -= elapsedTime;

                std::string message;

                if (m_countdownTimer < 0.5f)
                {
                    if (Managers::Get<OnlineManager>()->IsHost())
                    {
                        Managers::Get<PlayFabPartyManager>()->PopulatePartyRegionLatencies(false);
                        Managers::Get<GameStateManager>()->SwitchToState(GameState::EvaluatingNetwork);
                    }
                    else
                    {
                        m_countdownTimer = 0.0f;
                    }

                    message = "Game starting...";
                }
                else
                {
                    message = "Match starts in ";
                    message += std::to_string((int)m_countdownTimer);
                    message += " seconds";
                }

                m_menuEntries.emplace_back(message);

                break;
            }
        case GameState::EvaluatingNetwork:
            m_menuEntries.emplace_back("Evaluating Network...");
            break;
        case GameState::MigratingNetwork:
            m_menuEntries.emplace_back("Migrating network...");
            break;
        case GameState::WaitingForPeerMigration:
            m_menuEntries.emplace_back("Waiting for peers...");
            break;
        default:
            break;

        }
    }

    if (state != m_lastState)
    {
        ReadCurrentEntry();
    }

    m_lastState = state;

    MenuScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);
}

void GameLobbyScreen::Draw(float totalTime, float elapsedTime)
{
    if (!IsActive())
    {
        return;
    }

    if (State() == ScreenStateType::Active && !m_exiting)
    {
        auto renderManager = Managers::Get<RenderManager>();
        auto renderContext = renderManager->GetRenderContext(BlendMode::NonPremultiplied);
        auto spriteFont = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");
        auto viewportWidth = static_cast<float>(g_game->GetWindowWidth());
        float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
        float scale = 0.35f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

        renderContext->Begin();

        auto playerStates = g_game->GetAllPlayerStates();
        if (playerStates.size() > 0)
        {
            // draw session members centered
            XMFLOAT2 memberPositions[] =
            {
                XMFLOAT2(viewportWidth * 0.15f, viewportHeight * 0.7f),
                XMFLOAT2(viewportWidth * 0.38f, viewportHeight * 0.7f),
                XMFLOAT2(viewportWidth * 0.61f, viewportHeight * 0.7f),
                XMFLOAT2(viewportWidth * 0.84f, viewportHeight * 0.7f)
            };

            float inGameTextureScale = 0.9f * scale * spriteFont->GetLineSpacing() / m_inGameTexture.Texture->Height();
            XMFLOAT2 inGameTextureOrigin = XMFLOAT2((float)m_inGameTexture.Texture->Width(), (float)m_inGameTexture.Texture->Height() / 2.0f);

            float readyTextureScale = 0.9f * scale * spriteFont->GetLineSpacing() / m_readyTexture.Texture->Height();
            XMFLOAT2 readyTextureOrigin = XMFLOAT2((float)m_readyTexture.Texture->Width(), (float)m_readyTexture.Texture->Height() / 2.0f);

            UINT memberNumber = 0;
            for (const auto& playerState : playerStates)
            {
                if (!playerState)
                {
                    continue;
                }

                // get member display name
                std::string memberName = "(Reserved)";
                if (!playerState->DisplayName.empty())
                {
                    memberName = playerState->DisplayName;

                    if (playerState->IsLocalPlayer)
                    {
                        memberName.append(" (You)");
                    }
                }

                auto memberNameLen = spriteFont->MeasureString(memberName.c_str());
                XMFLOAT2 memberOrigin = XMFLOAT2(XMVectorGetX(memberNameLen) / 2.0f, spriteFont->GetLineSpacing() / 2.0f);

                // draw member display name
                if (playerState != nullptr && !playerState->IsInactive())
                {
                    renderContext->DrawString(spriteFont, memberName.c_str(), memberPositions[memberNumber % 4], Ship::Colors[playerState->ShipColor()], 0, memberOrigin, scale);
                }
                else
                {
                    renderContext->DrawString(spriteFont, memberName.c_str(), memberPositions[memberNumber % 4], Colors::LightGray, 0, memberOrigin, scale);
                }

                if (playerState != nullptr)
                {
                    // draw member status icons and ship
                    XMFLOAT2 glyphCenterPosition = XMFLOAT2(memberPositions[memberNumber % 4].x, memberPositions[memberNumber % 4].y + spriteFont->GetLineSpacing() * 1.7f * scale);

                    if (playerState->InGame)
                    {
                        // draw inGame texture
                        renderContext->Draw(m_inGameTexture, glyphCenterPosition, 0.0f, inGameTextureScale, Colors::White, TexturePosition::Centered);
                    }
                    else if (playerState->LobbyReady)
                    {
                        // draw ready texture if player is ready
                        renderContext->Draw(m_readyTexture, glyphCenterPosition, 0.0f, readyTextureScale, Colors::White, TexturePosition::Centered);
                    }

                    // draw ship
                    auto ship = playerState->GetShip();
                    if (ship)
                    {
                        XMFLOAT2 oldShipPosition = ship->Position;
                        float oldShipRotation = ship->Rotation;

                        ship->Position = glyphCenterPosition;
                        ship->Position.x += ship->Radius + 10.0f;
                        ship->Rotation = 0.0f;

                        ship->Draw(0.0f, renderContext.get(), true, GetScaleMultiplierForViewport(viewportWidth, viewportHeight));

                        ship->Rotation = oldShipRotation;
                        ship->Position = oldShipPosition;
                    }
                }

                // adjust the column for the next row
                memberPositions[memberNumber % 4].y += spriteFont->GetLineSpacing() * scale * 3.7f;
                memberNumber++;
            }

            if (IsActive())
            {
                // draw instructions centered at bottom of screen

                XMFLOAT2 position = XMFLOAT2(viewportWidth / 2.0f, viewportHeight * 0.95f);

                if (Managers::Get<GameStateManager>()->GetState() == GameState::Lobby)
                {
                    XMVECTOR size = spriteFont->MeasureString(gameSessionInstructions);
                    XMFLOAT2 origin = XMFLOAT2(XMVectorGetX(size) / 2.0f, spriteFont->GetLineSpacing() / 2.0f);
                    renderContext->DrawString(spriteFont, gameSessionInstructions, position, Colors::White, 0, origin, scale);
                }
            }
        }

        renderContext->End();
    }

    MenuScreen::Draw(totalTime, elapsedTime);

    DrawCurrentUser();
}

void GameLobbyScreen::OnCancel()
{
    m_menuEntries.clear();

    if (Managers::Get<OnlineManager>()->IsMatchmaking())
    {
        Managers::Get<GameStateManager>()->SwitchToState(GameState::MPMatchmakingCancel);
        Managers::Get<OnlineManager>()->CancelMatchmaking(false);
    }
    else
    {
        Managers::Get<GameStateManager>()->SwitchToState(GameState::ExitingGame);
        Managers::Get<OnlineManager>()->LeaveMultiplayerGame(false);
    }
}
