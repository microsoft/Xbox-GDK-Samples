using System;

public static class SessionConfiguration
{
    public const UInt32 TITLE_ID = 0x799A2E03;
    public const string SCID = @"00000000-0000-0000-0000-0000799a2e03";
    public const string LOBBY_SESSION_TEMPLATE_NAME = @"LobbySessionCrossGen";
    public const int LOBBY_MAX_MEMBERS_IN_SESSION = 8;
    public const string GAME_SESSION_TEMPLATE_NAME = @"GameSessionCrossGen";
    public const string APPID = @"000000004C5CA94C";
    public const string PUBLISHER = @"CN=A4954634-DF4B-47C7-AB70-D3215D246AF1";
    public const string IDENTITY_NAME = @"41336MicrosoftATG.SimpleCrossGenMPSD";
    public const string PUBLISHER_DISPLAY_NAME = @"Xbox Advanced Technology Group";
    public const string MATCHMAKE_HOPPER_NAME = @"GameHopperCrossGen";
    public const UInt64 MATCHMAKE_TIMEOUT_S = 30;
    public const UInt64 MATCHMAKE_TIMEOUT_MS = MATCHMAKE_TIMEOUT_S * 1000;
}
