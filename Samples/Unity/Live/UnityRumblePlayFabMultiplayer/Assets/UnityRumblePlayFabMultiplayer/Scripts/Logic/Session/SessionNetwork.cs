//--------------------------------------------------------------------------------------
// SessionNetwork.cs
//
// The game network which wraps the underlying networking APIs and adds messaging.
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

using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Assertions;
using PlayFab.Party;

public class SessionNetwork : MonoBehaviour
{
    public enum NetworkState : int
    {
        NoNetwork = 0,
        StartingNetwork = 1,
        InNetwork = 2,
        StoppingNetwork = 3,
    }

    public PlayFabMultiplayerManager PFMultiplayerManagerPrefab;

    // parameters are: sender UserID, message 
    public event Action<UserID> OnNetworkMessage_HelloNetwork_Received;
    public event Action<UserID, LobbyState> OnNetworkMessage_LobbyState_Received;
    public event Action<UserID, PlayerLobbyState> OnNetworkMessage_PlayerLobbyState_Received;
    public event Action<UserID, GameState> OnNetworkMessage_GameState_Received;
    public event Action<UserID, PlayerGameState> OnNetworkMessage_PlayerGameState_Received;
    public event Action<UserID, SpawnAsteroidState> OnNetworkMessage_SpawnAsteroidState_Received;
    public event Action<UserID, UpdateAsteroidState> OnNetworkMessage_UpdateAsteroidState_Received;
    public event Action<UserID, SpawnShipState> OnNetworkMessage_SpawnShipState_Received;
    public event Action<UserID, UpdateWeaponFireState> OnNetworkMessage_UpdateWeaponFireState_Received;
    public event Action<UserID, UpdateShipState> OnNetworkMessage_UpdateShipState_Received;
    public event Action<UserID, DestroyShipState> OnNetworkMessage_DestroyShipState_Received;
    public event Action<UserID> OnNetworkMessage_GoodbyeNetwork_Received;
    // parameters are: the network ID that was started
    public event Action<string> OnNetworkStarted;
    // parameters are: the network ID that was stopped
    public event Action<string> OnNetworkStopped;

    public NetworkState CurrentNetworkState { get; private set; }
    public PlayFabMultiplayerManager PFMultiplayerManager { get; private set; }

    private UserID _myUserID;
    private readonly Dictionary<UserID, PlayFabPlayer> _userIDToPlayerMap = new Dictionary<UserID, PlayFabPlayer>();

    public void InitializePlayFabParty()
    {
        if (null == PFMultiplayerManager)
        {
            PFMultiplayerManager = Instantiate<PlayFabMultiplayerManager>(PFMultiplayerManagerPrefab);
        }
    }

    public void StartNetworking(UserID myUserID, string networkID)
    {
        Debug.Log("SessionNetwork.StartNetworking()");

        if (CurrentNetworkState == NetworkState.NoNetwork)
        {
            _myUserID = myUserID;

            _userIDToPlayerMap.Clear();

            CurrentNetworkState = NetworkState.StartingNetwork;

            PFMultiplayerManager.OnNetworkJoined += HandleNetworkJoined;
            PFMultiplayerManager.OnNetworkChanged += HandleNetworkChanged;

            PFMultiplayerManager.OnRemotePlayerJoined += HandleRemotePlayerJoined;
            PFMultiplayerManager.OnRemotePlayerLeft += HandleRemotePlayerLeft;
            PFMultiplayerManager.OnDataMessageReceived += HandleDataMessageReceived;
        }

        if (string.IsNullOrEmpty(networkID))
        {
            PFMultiplayerManager.CreateAndJoinNetwork();
        }
        else
        {
            PFMultiplayerManager.JoinNetwork(networkID);
        }
    }

    public void StopNetworking()
    {
        Debug.Log("SessionNetwork.StopNetworking()");

        if (null != PFMultiplayerManager)
        {
            CurrentNetworkState = NetworkState.StoppingNetwork;

            PFMultiplayerManager.OnDataMessageReceived -= HandleDataMessageReceived;
            PFMultiplayerManager.OnRemotePlayerLeft -= HandleRemotePlayerLeft;
            PFMultiplayerManager.OnRemotePlayerJoined -= HandleRemotePlayerJoined;

            PFMultiplayerManager.OnNetworkLeft += HandleNeworkLeft;

            PFMultiplayerManager.LeaveNetwork();
        }
    }

    public void SendMessageToMember(UserID memberUserID, BaseNetStateObject netState)
    {
        if (netState.StateType == StateType.Reliable)
        {
            Debug.LogFormat($"SessionNetwork.SendMessageToMember({memberUserID}, {netState.MessageType})");
        }

        SendMessageToPlayer(GetPlayerEntityFromUserID(memberUserID), netState);
    }

    public void SendMessageToAll(BaseNetStateObject netState)
    {
        if (netState.StateType == StateType.Reliable)
        {
            Debug.LogFormat($"SessionNetwork.SendMessageToAll({netState.MessageType})");
        }

        SendMessageToAllRemotePlayers(netState);
    }

    private void Start()
    {
        CurrentNetworkState = NetworkState.NoNetwork;
    }

    private void OnValidate()
    {
        Assert.IsNotNull(PFMultiplayerManagerPrefab);
    }

    private void SendMessageToPlayer(PlayFabPlayer remotePlayer, BaseNetStateObject netState)
    {
        if (CurrentNetworkState != NetworkState.InNetwork)
        {
            return;
        }

        if (netState.StateType == StateType.Reliable)
        {
            Debug.LogFormat($"SessionNetwork.SendMessageToPlayer({remotePlayer.EntityKey.Id}, {netState.MessageType})");
        }

        byte[] messageBytes;
        netState.SerializeTo(out messageBytes);

        PFMultiplayerManager.SendDataMessage(messageBytes, new PlayFabPlayer[]{ remotePlayer }, GetDeliveryOptionForNetObject(netState));
    }

    private void SendMessageToAllRemotePlayers(BaseNetStateObject netState)
    {
        if (CurrentNetworkState != NetworkState.InNetwork)
        {
            return;
        }

        if (netState.StateType == StateType.Reliable)
        {
            Debug.LogFormat($"SessionNetwork.SendMessageToAllRemotePlayers({netState.MessageType})");
        }

        byte[] messageBytes;
        netState.SerializeTo(out messageBytes);

        PFMultiplayerManager.SendDataMessage(messageBytes, PFMultiplayerManager.RemotePlayers, GetDeliveryOptionForNetObject(netState));
    }

    private DeliveryOption GetDeliveryOptionForNetObject(BaseNetStateObject netObject)
    {
        switch (netObject.StateType)
        {
            case StateType.Reliable:
                return DeliveryOption.Guaranteed;

            case StateType.Unreliable:
                return DeliveryOption.BestEffort;

            default:
                throw new NotImplementedException($"Unrecognized net state type of {netObject.StateType}");
        }
    }

    private void HandleDataMessageReceived(object sender, PlayFabPlayer fromPlayer, byte[] buffer)
    {
        if (CurrentNetworkState != NetworkState.InNetwork)
        {
            return;
        }

        var fromUserID = new UserID(fromPlayer.EntityKey.Id);
        var messageType = SessionMessageHandler.ParseMessageType(buffer);

        //to reduce log spam
        if (messageType != SessionMessageType.UpdateShipState && messageType != SessionMessageType.UpdateAsteroidState && messageType != SessionMessageType.UpdateWeaponFireState)
        {
            Debug.LogFormat($"SessionNetwork.HandleDataMessageReceived() Bytes received:{buffer.Length} FromUser:{fromUserID.ID} MessageType:{messageType}");
        }

        switch (messageType)
        {
            case SessionMessageType.HelloNetwork:
                {
                    var helloNetwork = SessionMessageHandler.ParseMessage<HelloNetwork>(buffer);
                    Assert.AreEqual(fromUserID, helloNetwork.MyUserID);
                    SetPlayFabPlayerForUserID(fromPlayer, fromUserID);
                    OnNetworkMessage_HelloNetwork_Received?.Invoke(fromUserID);
                }
                break;

            case SessionMessageType.LobbyState:
                {
                    var lobbyState = SessionMessageHandler.ParseMessage<LobbyState>(buffer);
                    OnNetworkMessage_LobbyState_Received?.Invoke(fromUserID, lobbyState);
                }
                break;

            case SessionMessageType.PlayerLobbyState:
                {
                    var playerLobbyState = SessionMessageHandler.ParseMessage<PlayerLobbyState>(buffer);
                    OnNetworkMessage_PlayerLobbyState_Received?.Invoke(fromUserID, playerLobbyState);
                }
                break;

            case SessionMessageType.GameState:
                {
                    var gameState = SessionMessageHandler.ParseMessage<GameState>(buffer);
                    OnNetworkMessage_GameState_Received?.Invoke(fromUserID, gameState);
                }
                break;

            case SessionMessageType.PlayerGameState:
                {
                    var playerGameState = SessionMessageHandler.ParseMessage<PlayerGameState>(buffer);
                    OnNetworkMessage_PlayerGameState_Received?.Invoke(fromUserID, playerGameState);
                }
                break;

            case SessionMessageType.SpawnAsteroidState:
                {
                    var spawnAsteroidState = SessionMessageHandler.ParseMessage<SpawnAsteroidState>(buffer);
                    OnNetworkMessage_SpawnAsteroidState_Received?.Invoke(fromUserID, spawnAsteroidState);
                }
                break;

            case SessionMessageType.UpdateAsteroidState:
                {
                    var updateAsteroidState = SessionMessageHandler.ParseMessage<UpdateAsteroidState>(buffer);
                    OnNetworkMessage_UpdateAsteroidState_Received?.Invoke(fromUserID, updateAsteroidState);
                }
                break;

            case SessionMessageType.SpawnShipState:
                {
                    var spawnShipState = SessionMessageHandler.ParseMessage<SpawnShipState>(buffer);
                    OnNetworkMessage_SpawnShipState_Received?.Invoke(fromUserID, spawnShipState);
                }
                break;

            case SessionMessageType.UpdateShipState:
                {
                    var updateShipState = SessionMessageHandler.ParseMessage<UpdateShipState>(buffer);
                    OnNetworkMessage_UpdateShipState_Received?.Invoke(fromUserID, updateShipState);
                }
                break;

            case SessionMessageType.UpdateWeaponFireState:
                {
                    var updateWeaponFireState = SessionMessageHandler.ParseMessage<UpdateWeaponFireState>(buffer);
                    OnNetworkMessage_UpdateWeaponFireState_Received?.Invoke(fromUserID, updateWeaponFireState);
                }
                break;

            case SessionMessageType.DestroyShipState:
                {
                    var destroyShipState = SessionMessageHandler.ParseMessage<DestroyShipState>(buffer);
                    OnNetworkMessage_DestroyShipState_Received?.Invoke(fromUserID, destroyShipState);
                }
                break;

            case SessionMessageType.GoodbyeNetwork:
                {
                    SetPlayFabPlayerForUserID(fromPlayer, fromUserID);
                    OnNetworkMessage_GoodbyeNetwork_Received?.Invoke(fromUserID);
                }
                break;

                // NOTE: the Party plugin may issue this callback with messages that did
                // not originate from the app code, so we ignore those here.
            case SessionMessageType.InvalidMessage:
                // for now, do nothing with invalid messages since they could be noisy?
                break;

            default:
                throw new NotImplementedException($"Message type {messageType} not handled.");
        }
    }

    private void HandleRemotePlayerJoined(object sender, PlayFabPlayer player)
    {
        Debug.LogFormat($"SessionNetwork.HandleRemotePlayerJoined({player.EntityKey.Id})");

        SendMessageToPlayer(player, new HelloNetwork(_myUserID));
    }

    private void HandleRemotePlayerLeft(object sender, PlayFabPlayer player)
    {
        Debug.Log("SessionNetwork.HandleRemotePlayerLeft()");

        var userID = new UserID(player.EntityKey.Id);
        
        if(_userIDToPlayerMap.ContainsKey(userID))
        {
            _userIDToPlayerMap.Remove(userID);
            OnNetworkMessage_GoodbyeNetwork_Received?.Invoke(userID);
        }
    }

    private void HandleNetworkChanged(object sender, string newNetworkId)
    {
        Debug.LogFormat($"SessionNetwork.HandleNetworkChanged({newNetworkId})");

        // TODO: take action to migrate our local player
    }

    private void HandleNetworkJoined(object sender, string networkId)
    {
        Debug.LogFormat($"SessionNetwork.HandleNetworkJoined({networkId})");

        PFMultiplayerManager.OnNetworkJoined -= HandleNetworkJoined;

        CurrentNetworkState = NetworkState.InNetwork;
        OnNetworkStarted?.Invoke(networkId);

        // broadcast my XUID to all existing remote players

        SendMessageToAll(new HelloNetwork(_myUserID));
    }

    private void HandleNeworkLeft(object sender, string networkId)
    {
        Debug.Log("SessionNetwork.HandleNetworkLeft()");

        PFMultiplayerManager.OnNetworkLeft -= HandleNeworkLeft;

        _userIDToPlayerMap.Clear();

        CurrentNetworkState = NetworkState.NoNetwork;
        OnNetworkStopped?.Invoke(networkId);
    }

    private void SetPlayFabPlayerForUserID(PlayFabPlayer player, UserID userID)
    {
        Debug.LogFormat($"SessionNetwork.SetPlayFabPlayerForUserID() player:{player.ToString()} userID:{userID}");

        _userIDToPlayerMap[userID] = player;
    }

    private PlayFabPlayer GetPlayerEntityFromUserID(UserID userID)
    {
        PlayFabPlayer player = null;
        _userIDToPlayerMap.TryGetValue(userID, out player);
        return player;
    }   
}
