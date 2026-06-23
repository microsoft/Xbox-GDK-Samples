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

using UnityEngine;

public enum SessionMessageType : byte
{
    HelloNetwork,
    LobbyState,
    PlayerLobbyState,
    GameState,
    PlayerGameState,
    SpawnShipState,
    UpdateShipState,
    UpdateWeaponFireState,
    DestroyShipState,
    SpawnAsteroidState,
    UpdateAsteroidState,
    GoodbyeNetwork,
    InvalidMessage,
    MAX_COUNT
}

public static class SessionMessageHandler
{
    public const int MaxMessageLength = 64;

    public static SessionMessageType ParseMessageType(byte[] message)
    {
        SessionMessageType type = SessionMessageType.InvalidMessage;
        
        if (message.Length > 0)
        {
            if(message.Length <= MaxMessageLength)
            {
                var messageTypeByte = message[0];

                if(messageTypeByte >= 0 && messageTypeByte <= (byte)SessionMessageType.MAX_COUNT)
                {
                    type = (SessionMessageType)messageTypeByte;
                }
                else
                {
                    Debug.Log("SessionMessageHandler.ParseMessageType: A SessionMessageType was invalid");
                }
            }
            else
            {
                Debug.Log("SessionMessageHandler.ParseMessageType: message length was too large");
            }
        }
        else
        {
            Debug.Log("SessionMessageHandler.ParseMessageType: message length was invalid");
        }

        return type;
    }

    public static T ParseMessage<T>(byte[] message) where T : BaseNetStateObject, new()
    {
        var netObject = new T();
        netObject.DeserializeFrom(message);
        return netObject;
    }
}