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
using PartyCSharpSDK;

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

    // parameters are: sender XUID, message 
    public event Action<ulong> OnNetworkMessage_HelloNetwork_Received;
    public event Action<ulong, LobbyState> OnNetworkMessage_LobbyState_Received;
    public event Action<ulong, PlayerLobbyState> OnNetworkMessage_PlayerLobbyState_Received;
    public event Action<ulong, GameState> OnNetworkMessage_GameState_Received;
    public event Action<ulong, PlayerGameState> OnNetworkMessage_PlayerGameState_Received;
    public event Action<ulong, SpawnAsteroidState> OnNetworkMessage_SpawnAsteroidState_Received;
    public event Action<ulong, UpdateAsteroidState> OnNetworkMessage_UpdateAsteroidState_Received;
    public event Action<ulong, SpawnShipState> OnNetworkMessage_SpawnShipState_Received;
    public event Action<ulong, UpdateShipState> OnNetworkMessage_UpdateShipState_Received;
    public event Action<ulong, UpdateWeaponFireState> OnNetworkMessage_UpdateWeaponFireState_Recieved;
    public event Action<ulong, DestroyShipState> OnNetworkMessage_DestroyShipState_Received;
    public event Action<ulong> OnNetworkMessage_GoodbyeNetwork_Received;
    // parameters are: the network ID that was started
    public event Action<string> OnNetworkStarted;
    // parameters are: the network ID that was stopped
    public event Action<string> OnNetworkStopped;

    public NetworkState CurrentNetworkState { get; private set; }
    public PlayFabMultiplayerManager PFMultiplayerManager { get; private set; }

    private ulong _myXuid;
    private readonly Dictionary<string, ulong> _entityToXuidMap = new();
    private readonly Dictionary<ulong, PlayFabPlayer> _xuidToPlayerMap = new();

    public void StartNetworking(ulong myXuid, string networkID)
    {
        Debug.Log("SessionNetwork.StartNetworking()");

        if(CurrentNetworkState != NetworkState.NoNetwork)
        {
            return;
        }

        if (null == PFMultiplayerManager)
        {
            PFMultiplayerManager = Instantiate<PlayFabMultiplayerManager>(PFMultiplayerManagerPrefab);
        }

        _myXuid = myXuid;
        _entityToXuidMap.Clear();
        _xuidToPlayerMap.Clear();

        CurrentNetworkState = NetworkState.StartingNetwork;

        PFMultiplayerManager.OnNetworkJoined += HandleNetworkJoined;
        PFMultiplayerManager.OnNetworkChanged += HandleNetworkChanged;

        PFMultiplayerManager.OnRemotePlayerJoined += HandleRemotePlayerJoined;
        PFMultiplayerManager.OnRemotePlayerLeft += HandleRemotePlayerLeft;
        PFMultiplayerManager.OnDataMessageReceived += HandleDataMessageReceived;

        // If the network ID is null or empty, then we create one
        // otherwise we join the one provided
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

        if (null != PFMultiplayerManager && CurrentNetworkState != NetworkState.NoNetwork)
        {
            CurrentNetworkState = NetworkState.StoppingNetwork;

            PFMultiplayerManager.OnDataMessageReceived -= HandleDataMessageReceived;
            PFMultiplayerManager.OnRemotePlayerLeft -= HandleRemotePlayerLeft;
            PFMultiplayerManager.OnRemotePlayerJoined -= HandleRemotePlayerJoined;

            PFMultiplayerManager.OnNetworkLeft += HandleNeworkLeft;

            PFMultiplayerManager.LeaveNetwork();
        }
    }

    public void SendMessageToMember(ulong memberXuid, BaseNetStateObject netState)
    {
        if (netState.StateType == StateType.Reliable)
        {
            Debug.LogFormat("SessionNetwork.SendMessageToMember({0}, {1})", memberXuid, netState.MessageType);
        }

        SendMessageToPlayer(GetPlayerEntityFromXuid(memberXuid), netState);
    }

    public void SendMessageToAll(BaseNetStateObject netState)
    {
        if (netState.StateType == StateType.Reliable)
        {
            Debug.LogFormat("SessionNetwork.SendMessageToAll({0})", netState.MessageType);
        }

        SendMessageToAllRemotePlayers(netState);
    }

    public ulong GetXuidForPlayer(PlayFabPlayer player)
    {
        return GetPlayerEntityXuid(player);
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
            Debug.LogFormat("SessionNetwork.__SendMessageToPlayer({0}, {1})", remotePlayer.EntityKey.Id, netState.MessageType);
        }

        netState.SerializeTo(out byte[] messageBytes);

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
            Debug.LogFormat("SessionNetwork.__SendMessageToAllRemotePlayers({0})", netState.MessageType);
        }

        netState.SerializeTo(out byte[] messageBytes);

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

    private void HandleDataMessageReceived(object sender, PlayFabPlayer from, byte[] buffer)
    {
        if (CurrentNetworkState != NetworkState.InNetwork)
        {
            return;
        }

        var messageType = SessionMessageHandler.ParseMessageType(buffer);

        if (messageType != SessionMessageType.UpdateShipState)
        {
            Debug.LogFormat("SessionNetwork.__HandleDataMessageReceived() total bytes of {0} from {1}", buffer.Length, from.EntityKey.Id);
            Debug.LogFormat(">>> message type is: {0}", messageType);
        }

        var expectedMessageSize = SessionMessageHandler.ExpectedMessageSize(messageType);

        if (messageType != SessionMessageType.InvalidMessage &&
            expectedMessageSize != buffer.Length - 1)
        {
            Debug.LogFormat(">>> UNEXPECTED MESSAGE SIZE {0}, SHOULD BE {1}", buffer.Length - 1, expectedMessageSize);
            return;
        }

        switch (messageType)
        {
            case SessionMessageType.HelloNetwork:
                {
                    var helloNetwork = SessionMessageHandler.ParseMessage<HelloNetwork>(buffer);
                    SetPlayerEntityXuid(from, helloNetwork.MyXuid);
                    OnNetworkMessage_HelloNetwork_Received?.Invoke(helloNetwork.MyXuid);
                }
                break;

            case SessionMessageType.LobbyState:
                {
                    var lobbyState = SessionMessageHandler.ParseMessage<LobbyState>(buffer);
                    OnNetworkMessage_LobbyState_Received?.Invoke(GetPlayerEntityXuid(from), lobbyState);
                }
                break;

            case SessionMessageType.PlayerLobbyState:
                {
                    var playerLobbyState = SessionMessageHandler.ParseMessage<PlayerLobbyState>(buffer);
                    OnNetworkMessage_PlayerLobbyState_Received?.Invoke(GetPlayerEntityXuid(from), playerLobbyState);
                }
                break;

            case SessionMessageType.GameState:
                {
                    var gameState = SessionMessageHandler.ParseMessage<GameState>(buffer);
                    OnNetworkMessage_GameState_Received?.Invoke(GetPlayerEntityXuid(from), gameState);
                }
                break;

            case SessionMessageType.PlayerGameState:
                {
                    var playerGameState = SessionMessageHandler.ParseMessage<PlayerGameState>(buffer);
                    OnNetworkMessage_PlayerGameState_Received?.Invoke(GetPlayerEntityXuid(from), playerGameState);
                }
                break;

            case SessionMessageType.SpawnAsteroidState:
                {
                    var spawnAsteroidState = SessionMessageHandler.ParseMessage<SpawnAsteroidState>(buffer);
                    OnNetworkMessage_SpawnAsteroidState_Received?.Invoke(GetPlayerEntityXuid(from), spawnAsteroidState);
                }
                break;

            case SessionMessageType.UpdateAsteroidState:
                {
                    var updateAsteroidState = SessionMessageHandler.ParseMessage<UpdateAsteroidState>(buffer);
                    OnNetworkMessage_UpdateAsteroidState_Received?.Invoke(GetPlayerEntityXuid(from), updateAsteroidState);
                }
                break;

            case SessionMessageType.SpawnShipState:
                {
                    var spawnShipState = SessionMessageHandler.ParseMessage<SpawnShipState>(buffer);
                    OnNetworkMessage_SpawnShipState_Received?.Invoke(GetPlayerEntityXuid(from), spawnShipState);
                }
                break;

            case SessionMessageType.UpdateShipState:
                {
                    var updateShipState = SessionMessageHandler.ParseMessage<UpdateShipState>(buffer);
                    OnNetworkMessage_UpdateShipState_Received?.Invoke(GetPlayerEntityXuid(from), updateShipState);
                }
                break;
            case SessionMessageType.UpdateWeaponFireState:
                {
                    var updateWeaponFireState = SessionMessageHandler.ParseMessage<UpdateWeaponFireState>(buffer);
                    OnNetworkMessage_UpdateWeaponFireState_Recieved?.Invoke(GetPlayerEntityXuid(from), updateWeaponFireState);
                }
                break;
            case SessionMessageType.DestroyShipState:
                {
                    var destroyShipState = SessionMessageHandler.ParseMessage<DestroyShipState>(buffer);
                    OnNetworkMessage_DestroyShipState_Received?.Invoke(GetPlayerEntityXuid(from), destroyShipState);
                }
                break;
            case SessionMessageType.GoodbyeNetwork:
                {
                    var goodbyeNetwork = SessionMessageHandler.ParseMessage<GoodbyeNetwork>(buffer);
                    SetPlayerEntityXuid(from, goodbyeNetwork.MyXuid);
                    OnNetworkMessage_GoodbyeNetwork_Received?.Invoke(goodbyeNetwork.MyXuid);
                }
                break;
            case SessionMessageType.InvalidMessage:
                // for now, do nothing with invalid messages since they could be noisy?
                break;

            default:
                throw new NotImplementedException($"Message type {messageType} not handled.");
        }
    }

    private void HandleRemotePlayerJoined(object sender, PlayFabPlayer player)
    {
        Debug.LogFormat("SessionNetwork.__HandleRemotePlayerJoined({0})", player.EntityKey.Id);

        // tell the remote player what my XUID is

        SendMessageToPlayer(player, new HelloNetwork(_myXuid));
    }

    private void HandleRemotePlayerLeft(object sender, PlayFabPlayer player)
    {
        Debug.Log("SessionNetwork.__HandleRemotePlayerLeft()");

        var entityId = player.EntityKey.Id;

        if (_entityToXuidMap.ContainsKey(entityId))
        {
            var xuid = _entityToXuidMap[entityId];
            _xuidToPlayerMap.Remove(xuid);
            _entityToXuidMap.Remove(entityId);
            OnNetworkMessage_GoodbyeNetwork_Received?.Invoke(xuid);
        }

    }

    private void HandleNetworkChanged(object sender, string newNetworkId)
    {
        Debug.LogFormat("SessionNetwork.__HandleNetworkChanged({0})", newNetworkId);

        // note: beyond the scope of this sample, but in the event this should happen,
        // the code needs to take action to migrate our local player to the new network
        // by leaving the old one, and joining the new one.
    }

    private void HandleNetworkJoined(object sender, string networkId)
    {
        Debug.LogFormat("SessionNetwork.__HandleNetworkJoined({0})", networkId);

        PFMultiplayerManager.OnNetworkJoined -= HandleNetworkJoined;

        CurrentNetworkState = NetworkState.InNetwork;
        OnNetworkStarted?.Invoke(networkId);

        // broadcast my XUID to all existing remote players

        SendMessageToAll(new HelloNetwork(_myXuid));
    }

    private void HandleNeworkLeft(object sender, string networkId)
    {
        Debug.Log("SessionNetwork.__HandleNetworkLeft()");

        PFMultiplayerManager.OnNetworkLeft -= HandleNeworkLeft;

        _entityToXuidMap.Clear();
        _xuidToPlayerMap.Clear();

        // note: we cannot destroy the object per se because there are internal singletons
        // managed by the plugin that will not be properly handled with a new instance of
        // a PartyMultiplayerManager
        //Destroy(PFMultiplayerManager);
        //PFMultiplayerManager = null;

        CurrentNetworkState = NetworkState.NoNetwork;
        OnNetworkStopped?.Invoke(networkId);
    }

    private void SetPlayerEntityXuid(PlayFabPlayer player, ulong xuid)
    {
        Debug.Log("SessionNetwork.__SetPlayerEntityXuid()");
        Debug.LogFormat(">>> entity id {0} has xuid of {1}", player.EntityKey.Id, xuid);

        _entityToXuidMap[player.EntityKey.Id] = xuid;
        _xuidToPlayerMap[xuid] = player;
    }

    private PlayFabPlayer GetPlayerEntityFromXuid(ulong xuid)
    {
        _xuidToPlayerMap.TryGetValue(xuid, out PlayFabPlayer player);
        return player;
    }

    private ulong GetPlayerEntityXuid(PlayFabPlayer player)
    {
        _entityToXuidMap.TryGetValue(player.EntityKey.Id, out ulong xuid);
        return xuid;
    }
}
