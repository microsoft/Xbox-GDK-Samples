#pragma once

namespace PlayFabMultiplayerRumble
{
    namespace SampleConfig
    {
        constexpr auto c_pfTitleId = "9AD5D";
        constexpr auto c_ServiceConfigId = "00000000-0000-0000-0000-0000664cd024";

        constexpr auto c_MaxPlayers = 8;
        constexpr auto c_MaxPlayersPerDevice = 1;

        constexpr auto c_MatchQueueName = "SampleQuickMatch";
        constexpr auto c_MatchTimeoutSeconds = 60;

        constexpr auto c_LobbyCountdownSeconds = 3.0f;

        constexpr auto c_AutoLogIn = false;               //For debugging purposes
        constexpr auto c_AllowSinglePlayerMatch = false;  //For debugging purposes
    }
}
