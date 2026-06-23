//--------------------------------------------------------------------------------------
// Configuration.cs
//
// Global level configuration settings and constants.
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
// to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

public static class Configuration
{
    public const string PLAYFAB_TITLE_ID = @"9AD5D";
    public const string PLAYFAB_TOKEN_PROVIDER_URL = @"https://playfabapi.com/";

    public const int LOBBY_MIN_MEMBERS_TO_START_GAME = 1;
    public const int LOBBY_MAX_MEMBERS_IN_SESSION = 4;
    public const string MATCHMAKING_HOPPER_NAME = @"SampleQuickMatch";
    public const int MATCHMAKING_TIMEOUT_IN_SECONDS = 30;
    
    public const float COUNTDOWN_SECONDS = 3.0F;
    public const int MAX_KILLS_PER_GAME = 5;
    public const int MIN_ASTEROID_COUNT = 2;
    public const int MAX_ASTEROID_COUNT = 5;
}
