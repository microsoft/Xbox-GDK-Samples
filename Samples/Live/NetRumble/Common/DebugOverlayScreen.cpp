//--------------------------------------------------------------------------------------
// DebugOverlayScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Managers.h"
#include "ScreenManager.h"
#include "DebugOverlayScreen.h"
#include "Game.h"
#include "PlayerState.h"

using namespace NetRumble;
using namespace DirectX;
using namespace Party;

const float c_UserInfoLeft = 50.0f;
const float c_UserInfoTop = 50.0f;

DebugOverlayScreen::DebugOverlayScreen() : GameScreen()
{
    m_exitWhenHidden = false;
}

void DebugOverlayScreen::Draw(float totalTime, float elapsedTime)
{
    auto renderManager = Managers::Get<RenderManager>();
    auto renderContext = renderManager->GetRenderContext(BlendMode::NonPremultiplied);
    auto spriteFont = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\Consolas_32.spritefont");
    auto viewportWidth = static_cast<float>(g_game->GetWindowWidth());
    float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
    float scale = 0.45f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

    renderContext->Begin();

    // Draw a status line for each player
    auto count = 0;
    auto lineWidth = spriteFont->MeasureString("X");
    auto playerStates = g_game->GetAllPlayerStates();

    if (playerStates.size() > 0)
    {
        for (const auto& playerState : playerStates)
        {
            if (playerState != nullptr)
            {
                auto ship = playerState->GetShip();
                char buffer[512]{};

                sprintf_s(
                    buffer,
                    512,
                    "%-20s[%15llu]%4s%4s%4s%4sPos: %010.4f, %010.4f Vel: %010.4f, %010.4f LS: %010.4f, %010.4f RS %010.4f, %010.4f",
                    playerState->DisplayName.empty() ? "[NONAMEYET]" : playerState->DisplayName.c_str(),
                    playerState->PeerId,
                    playerState->InGame ? "GME" : "",
                    playerState->IsLocalPlayer ? "LCL" : "",
                    playerState->LobbyReady ? "RDY" : "",
                    playerState->IsInactive() ? "IDL" : "",
                    ship ? ship->Position.x : 0.0f,
                    ship ? ship->Position.y : 0.0f,
                    ship ? ship->Velocity.x : 0.0f,
                    ship ? ship->Velocity.y : 0.0f,
                    ship ? ship->Input.LeftStick.x : 0.0f,
                    ship ? ship->Input.LeftStick.y : 0.0f,
                    ship ? ship->Input.RightStick.x : 0.0f,
                    ship ? ship->Input.RightStick.y : 0.0f
                    );

                renderContext->DrawString(
                    spriteFont,
                    buffer,
                    XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
                    Colors::Yellow,
                    0,
                    XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
                    scale
                );

                count++;
            }
        }
    }

    PartyNetworkDescriptor descriptor;
    std::string msgStr;

    if (Managers::Get<PlayFabPartyManager>()->GetPartyNetworkDescriptor(&descriptor))
    {

        count++;
        msgStr = "Party Network Identifier: ";
        msgStr += descriptor.networkIdentifier;

        scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

        renderContext->DrawString(
            spriteFont,
            msgStr.c_str(),
            XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
            Colors::Yellow,
            0,
            XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
            scale
        );

        count++;

        msgStr = "Party Network Region: ";
        msgStr += descriptor.regionName;

        scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

        renderContext->DrawString(
            spriteFont,
            msgStr.c_str(),
            XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
            Colors::Yellow,
            0,
            XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
            scale
        );
    }

    msgStr = "CurrentlyQueuedSendMessages: ";
    msgStr += std::to_string(Managers::Get<PlayFabPartyManager>()->GetNetworkQueuedSendMessagesCount());

    count++;
    scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

    renderContext->DrawString(
        spriteFont,
        msgStr.c_str(),
        XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
        Colors::Yellow,
        0,
        XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
        scale
    );

    count++;

    msgStr = "AverageRelayServerRoundTripLatencyInMilliseconds: ";
    msgStr += std::to_string(Managers::Get<PlayFabPartyManager>()->GetNetworkAverageRelayRoundTripTime());

    scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

    renderContext->DrawString(
        spriteFont,
        msgStr.c_str(),
        XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
        Colors::Yellow,
        0,
        XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
        scale
    );

    count++;

    auto userLatencies = Managers::Get<PlayFabPartyManager>()->GetRemotePlayerLatencies();

    if (userLatencies.size() > 0)
    {
        std::string timingString = "Player to Player Round-Trip Times:\n";

        for (const auto& item : userLatencies)
        {
            std::string name;
            std::shared_ptr<PlayerState> playerInfo = g_game->GetPlayerState(item.first);

            if (playerInfo != nullptr)
            {
                name = playerInfo->DisplayName;
            }
            else
            {
                name = std::to_string(item.first);
            }

            char buffer[128]{};
            sprintf_s(buffer, 128, "%20s:  %llu ms\n", name.c_str(), item.second);
            timingString += buffer;
        }

        count++;
        scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

        renderContext->DrawString(
            spriteFont,
            timingString.c_str(),
            XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
            Colors::Yellow,
            0,
            XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
            scale
        );

        count += static_cast<int>(userLatencies.size()) + 1;
    }

    uint32_t regionCount;
    const Party::PartyRegion* regionList;

    PartyError err = PartyManager::GetSingleton().GetRegions(&regionCount, &regionList);

    if (PARTY_SUCCEEDED(err))
    {
        msgStr.clear();
        msgStr.reserve(1024);
        msgStr = "Party Regions: ";

        if (regionCount > 0)
        {
            msgStr += "\n";

            for (uint32_t x = 0; x < regionCount; x++)
            {
                char buffer[128]{};
                sprintf_s(buffer, 128, "%20hs:  %lu ms\n", regionList[x].regionName, regionList[x].roundTripLatencyInMilliseconds);
                msgStr += buffer;
            }
        }
        else
        {
            msgStr += "Not populated yet";
        }
    }

    count++;
    scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

    renderContext->DrawString(
        spriteFont,
        msgStr.c_str(),
        XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
        Colors::Yellow,
        0,
        XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
        scale
    );

    renderContext->End();

    GameScreen::Draw(totalTime, elapsedTime);
}
