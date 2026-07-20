//--------------------------------------------------------------------------------------
// SessionMessageType.cs
//
// The different types of individual game network messages and object states.
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

using System.Collections.Generic;

public enum SessionMessageType : byte
{
    HelloNetwork = 0xC0,
    LobbyState = 0xC1,
    PlayerLobbyState = 0xC2,
    GameState = 0xC3,
    PlayerGameState = 0xC4,
    SpawnShipState = 0xC5,
    UpdateShipState = 0xC6,
    UpdateWeaponFireState = 0xC7,
    DestroyShipState = 0xC8,
    SpawnAsteroidState = 0xC9,
    UpdateAsteroidState = 0xCA,
    //CB-CE are available for use
    GoodbyeNetwork = 0xCF,
    InvalidMessage = 0xFF,
}

public static class SessionMessageHandler
{
    public const int MaxMessageLength = 64;

    public static SessionMessageType ParseMessageType(byte[] message)
    {
        if (message.Length < 1)
        {
            return SessionMessageType.InvalidMessage;
        }

        else if (message.Length > MaxMessageLength)
        {
            return SessionMessageType.InvalidMessage;
        }

        var messageTypeByte = message[0];

        if (!_sessionMessageTypes.ContainsKey(messageTypeByte))
        {
            return SessionMessageType.InvalidMessage;
        }

        return _sessionMessageTypes[messageTypeByte];
    }

    public static int ExpectedMessageSize(SessionMessageType messageType)
    {
        return _messageSizes[messageType];
    }

    public static T ParseMessage<T>(byte[] message) where T : BaseNetStateObject, new()
    {
        var netObject = new T();
        netObject.DeserializeFrom(message);
        return netObject;
    }

    private static Dictionary<byte, SessionMessageType> _sessionMessageTypes = new Dictionary<byte, SessionMessageType>()
    {
        { (byte)SessionMessageType.HelloNetwork, SessionMessageType.HelloNetwork },
        { (byte)SessionMessageType.LobbyState, SessionMessageType.LobbyState },
        { (byte)SessionMessageType.PlayerLobbyState, SessionMessageType.PlayerLobbyState },
        { (byte)SessionMessageType.GameState, SessionMessageType.GameState },
        { (byte)SessionMessageType.PlayerGameState, SessionMessageType.PlayerGameState },
        { (byte)SessionMessageType.SpawnShipState, SessionMessageType.SpawnShipState },
        { (byte)SessionMessageType.UpdateShipState, SessionMessageType.UpdateShipState },
        { (byte)SessionMessageType.UpdateWeaponFireState, SessionMessageType.UpdateWeaponFireState },
        { (byte)SessionMessageType.DestroyShipState, SessionMessageType.DestroyShipState },
        { (byte)SessionMessageType.GoodbyeNetwork, SessionMessageType.GoodbyeNetwork },
        { (byte)SessionMessageType.SpawnAsteroidState, SessionMessageType.SpawnAsteroidState },
        { (byte)SessionMessageType.UpdateAsteroidState, SessionMessageType.UpdateAsteroidState },
        { (byte)SessionMessageType.InvalidMessage, SessionMessageType.InvalidMessage },
    };

    private static Dictionary<SessionMessageType, int> _messageSizes = new Dictionary<SessionMessageType, int>()
    {
        // ulong
        { SessionMessageType.HelloNetwork, 8 },
        // bool(byte)
        { SessionMessageType.LobbyState, 1 },
        // bool(byte), byte
        { SessionMessageType.PlayerLobbyState, 2 },
        // ulong
        { SessionMessageType.GameState, 8 },
        // int, int
        { SessionMessageType.PlayerGameState, 8 },
        // int, int, float, float, float
        { SessionMessageType.SpawnShipState, 20 },
        // float, float, float, float, sbyte, sbyte, sbyte, sbyte
        { SessionMessageType.UpdateShipState, 20 },
        // bool
        { SessionMessageType.UpdateWeaponFireState, 1 },
        // ulong, ulong
        { SessionMessageType.DestroyShipState, 16 },
        // ulong
        { SessionMessageType.GoodbyeNetwork, 8 },
        // int, float, float
        { SessionMessageType.SpawnAsteroidState, 16 },
        // sbyte, float, float, float, float
        { SessionMessageType.UpdateAsteroidState, 17 },
        // <null>
        { SessionMessageType.InvalidMessage, 0 },
    };
}