//--------------------------------------------------------------------------------------
// GamePlayScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GamePlayScreen.h"
#include "Managers.h"
#include "PlayerState.h"
#include "Game.h"
#include "StarfieldScreen.h"
#include "DebugOverlayScreen.h"
#include "SampleConfig.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;
using namespace DirectX::SimpleMath;

GamePlayScreen::GamePlayScreen() :
    GameScreen()
{
    m_playerFont = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");
    m_scoreFont = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\NetRumble.spritefont");
}

void GamePlayScreen::HandleInput()
{
    auto inputManager = Managers::Get<InputManager>();

    // pass input along to the local ship
    auto localPlayer = g_game->GetLocalPlayerState();
    auto localShip = localPlayer->GetShip();
    if (localShip->Active() && !g_game->IsGameWon())
    {
        localShip->Input = ShipInput(inputManager->CurrentGamePadState);
        localShip->Input.Add(ShipInput(inputManager->CurrentKeyboardState()));

        if (localShip->Input.LeftStick != SimpleMath::Vector2(0.0f, 0.0f) ||
            localShip->Input.RightStick != SimpleMath::Vector2(0.0f, 0.0f) ||
            localShip->Input.MineFired != false)
        {
            Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::ShipInput, localShip->Input.Serialize()));
        }
    }
    else
    {
        localShip->Input = ShipInput();
    }

    if (g_game->IsGameWon())
    {
        if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::A) || inputManager->IsNewKeyPress(Keyboard::Keys::Enter))
        {
            ExitScreen();
        }
    }

    if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::View))
    {
        // TODO: add exit confirmation
        ExitScreen();
    }

    GameScreen::HandleInput();
}

void GamePlayScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
    g_game->UpdateWorld(totalTime, elapsedTime);
    GameScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);
}

void GamePlayScreen::Draw(float totalTime, float elapsedTime)
{
    UNREFERENCED_PARAMETER(totalTime);

    g_game->DrawWorld(elapsedTime);
    DrawHud();
}

void GamePlayScreen::ExitScreen(bool immediate)
{
    g_game->ResetGameplayData();

    Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
    Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);

    GameScreen::ExitScreen(immediate);
}

void GamePlayScreen::DrawHud()
{
    auto renderManager = Managers::Get<RenderManager>();
    auto renderContext = renderManager->GetRenderContext();
    auto viewportWidth = static_cast<float>(g_game->GetWindowWidth());
    auto viewportHeight = static_cast<float>(g_game->GetWindowHeight());
    XMFLOAT2 fontOrigin = XMFLOAT2(0, 0);//m_playerFont->GetLineSpacing() / 2.0f);
    float playerNameScale = .4f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
    float messageScale = 2.25f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

    renderContext->Begin();

    auto playerStates = g_game->GetAllPlayerStates();
    auto playerIds = std::vector<uint16_t>();
    auto count = playerStates.size();

    Vector2 memberPositions[] =
    {
        Vector2(viewportWidth * 0.15f, viewportHeight * 0.065f),
        Vector2(viewportWidth * 0.30f, viewportHeight * 0.065f),
        Vector2(viewportWidth * 0.70f, viewportHeight * 0.065f),
        Vector2(viewportWidth * 0.85f, viewportHeight * 0.065f)
    };

    // draw players 0 - 3 at the top of the screen
    for (uint32_t i = 0; i < std::min<size_t>(static_cast<size_t>(4), count); ++i)
    {
        auto playerState = playerStates[i];
        if (playerState)
        {
            auto memberName = playerState->DisplayName;
            auto memberColor = (playerState->InGame) ? Ship::Colors[playerState->ShipColor()] : Colors::LightGray;
            auto memberNameLen = (playerNameScale * Vector2(m_scoreFont->MeasureString(memberName.c_str())).x) / 2;

            auto namePosition = memberPositions[i] - Vector2(memberNameLen / 2, 0);

            renderContext->DrawString(m_playerFont, memberName, namePosition, memberColor, 0, fontOrigin, playerNameScale);
            memberName = playerState->DisplayName;
            // draw score and respawn counter centered underneath each name
            auto ship = playerState->GetShip();
            std::string memberData = std::to_string(ship->Score);
            /*if (!ship->Active() && ship->RespawnTimer > 0.0f)
            {
                memberData += "  (" + std::to_string(1 + static_cast<int>(ship->RespawnTimer)) + ")";
            }*/

            auto scoreLen = (playerNameScale * Vector2(m_scoreFont->MeasureString(memberData.c_str())).x) / 2;
            auto scorePosition = memberPositions[i] + Vector2((-scoreLen) / 2, (playerNameScale * m_scoreFont->GetLineSpacing()));
            renderContext->DrawString(m_scoreFont, memberData.c_str(), scorePosition, memberColor, 0, fontOrigin, 2 * playerNameScale);
        }
    }

    // draw players 4 - 7 at the bottom of the screen
    for (uint32_t i = 4; i < std::min<size_t>(static_cast<size_t>(8), count); ++i)
    {
        memberPositions[i % 4].y = viewportHeight * 0.9f;

        auto playerState = playerStates[i];
        if (playerState)
        {
            auto memberName = playerState->DisplayName;
            auto memberColor = (playerState->InGame) ? Ship::Colors[playerState->ShipColor()] : Colors::LightGray;

            renderContext->DrawString(m_playerFont, memberName, memberPositions[i % 4], memberColor, 0, fontOrigin, playerNameScale);
            memberName = playerState->DisplayName;
            // draw score and respawn counter centered underneath each name
            auto ship = playerState->GetShip();
            std::string memberData = std::to_string(ship->Score);
            if (!ship->Active() && ship->RespawnTimer > 0.0f)
            {
                memberData += "  (" + std::to_string(1 + static_cast<int>(ship->RespawnTimer)) + ")";
            }
            auto memberNameLen = playerNameScale * Vector2(m_playerFont->MeasureString(memberName.c_str()));
            XMFLOAT2 scorePosition = XMFLOAT2(memberPositions[i % 4].x + (XMVectorGetX(memberNameLen) / 2.0f), memberPositions[i % 4].y + (playerNameScale * m_playerFont->GetLineSpacing()));
            renderContext->DrawString(m_scoreFont, memberData.c_str(), scorePosition, memberColor, 0, fontOrigin, 2 * playerNameScale);
        }
    }

    // draw a spawn countdown text message for the local user when appropriate
    auto localShip = g_game->GetLocalPlayerState()->GetShip();
    if (!g_game->IsGameWon() && !localShip->Active() && localShip->RespawnTimer > 0.0f)
    {
        std::string respawnMessage = "Spawning in " + std::to_string(1 + static_cast<int>(localShip->RespawnTimer));
        auto respawnMessageLen = m_playerFont->MeasureString(respawnMessage.c_str());
        XMFLOAT2 respawnMessagePosition = XMFLOAT2(viewportWidth / 2.0f, viewportHeight / 2.0f);
        XMFLOAT2 respawnMessageOrigin = XMFLOAT2(XMVectorGetX(respawnMessageLen) / 2.0f, 0.0f);
        renderContext->DrawString(m_playerFont, respawnMessage.c_str(), respawnMessagePosition, Colors::White, 0.0f, respawnMessageOrigin, messageScale);
    }

    // if round has been won, draw win message
    if (g_game->IsGameWon())
    {
        std::string winMessageLine1, winMessageLine2;

        if (g_game->GetWinnerName() == "Insufficient Players" && SampleConfig::c_AllowSinglePlayerMatch == false)
        {
            winMessageLine1 = "Insufficient Players to continue the game.";
        }
        else
        {
            winMessageLine1 = g_game->GetWinnerName().data();
            winMessageLine1 += " has won the game!";
        }

        winMessageLine2 = "Press (A) to return to the main menu";

        XMFLOAT2 winMessagePosition = XMFLOAT2(viewportWidth / 2.0f, viewportHeight * 0.4f);

        auto winMessageLen = m_playerFont->MeasureString(winMessageLine1.c_str());
        auto winMessageOrigin = XMFLOAT2(XMVectorGetX(winMessageLen) / 2.0f, m_playerFont->GetLineSpacing());
        renderContext->DrawString(m_playerFont, winMessageLine1, winMessagePosition, g_game->GetWinningColor(), 0.0f, winMessageOrigin, playerNameScale);

        winMessageLen = m_playerFont->MeasureString(winMessageLine2.data());
        winMessageOrigin = XMFLOAT2(XMVectorGetX(winMessageLen) / 2.0f, 0.0f);
        renderContext->DrawString(m_playerFont, winMessageLine2.data(), winMessagePosition, g_game->GetWinningColor(), 0.0f, winMessageOrigin, playerNameScale);
    }

    renderContext->End();
}
