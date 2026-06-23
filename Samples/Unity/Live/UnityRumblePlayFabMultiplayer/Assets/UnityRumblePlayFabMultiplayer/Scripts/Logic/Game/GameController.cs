//--------------------------------------------------------------------------------------
// GameController.cs
//
// The high level game controller that handles game events and manages game objects.
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
using System.Linq;
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.InputSystem.UI;

public partial class GameController : MonoBehaviour
{
#pragma warning disable 0649 //these are set via the editor
    [SerializeField]
    private SessionNetwork Networking;

    [SerializeField]
    private GameAssetManager AssetManager;

    [SerializeField]
    private InputSystemUIInputModule InputModule;

    [SerializeField]
    private Camera GameCamera;

    [SerializeField]
    private SpawnController[] ShipSpawners;

    [SerializeField]
    private SpawnController[] AsteroidSpawners;

    [SerializeField]
    private GameObject ShipsContainer;

    [SerializeField]
    private GameObject SatellitesContainer;

    [SerializeField]
    private GameObject AsteroidsContainer;
#pragma warning restore 0649

    public event Action OnLocalPlayerRequestQuit;
    // parameters are: destroyed UserID, destroyer UserID
    public event Action<UserID, UserID> OnShipDestroyed;

    public int LocalShipIndex { get; private set; }
    public int LocalColorIndex { get; private set; }
    public ShipController LocalPlayerShip { get; private set; }

    private System.Random _randomNumberGenerator = new System.Random();
    private readonly List<AsteroidController> _asteroidControllers = new List<AsteroidController>();
    private readonly Dictionary<UserID, ShipController> _shipControllers = new Dictionary<UserID, ShipController>();
    
    public void NowIsHost()
    {
        foreach (var asteroid in _asteroidControllers)
        {
            asteroid.IsHost = true;
        }
    }

    public void Initialize(int localShipIndex, int localColorIndex)
    {
        Debug.LogFormat($"GameController.Initialize({localShipIndex}, {localColorIndex})");

        LocalShipIndex = localShipIndex;
        LocalColorIndex = localColorIndex;

        Networking.OnNetworkMessage_GameState_Received += HandleGameStateReceived;
        Networking.OnNetworkMessage_PlayerGameState_Received += HandlePlayerGameStateReceived;
        Networking.OnNetworkMessage_SpawnAsteroidState_Received += HandleSpawnAsteroidStateReceived;
        Networking.OnNetworkMessage_SpawnShipState_Received += HandleSpawnShipStateReceived;
        Networking.OnNetworkMessage_DestroyShipState_Received += HandleDestroyShipStateReceived;
        Networking.OnNetworkMessage_GoodbyeNetwork_Received += HandleGoodbyeNetworkReceived;
    }

    public void Cleanup()
    {
        Debug.Log("GameController.Cleanup()");

        while (0 < _shipControllers.Count)
        {
            DestroyShip(_shipControllers.First().Key);
        }

        while (0 < _asteroidControllers.Count)
        {
            DestroyAsteroid(0);
        }

        Networking.OnNetworkMessage_GameState_Received -= HandleGameStateReceived;
        Networking.OnNetworkMessage_PlayerGameState_Received -= HandlePlayerGameStateReceived;
        Networking.OnNetworkMessage_SpawnAsteroidState_Received -= HandleSpawnAsteroidStateReceived;
        Networking.OnNetworkMessage_SpawnShipState_Received -= HandleSpawnShipStateReceived;
        Networking.OnNetworkMessage_DestroyShipState_Received -= HandleDestroyShipStateReceived;
        Networking.OnNetworkMessage_GoodbyeNetwork_Received -= HandleGoodbyeNetworkReceived;
    }

    public void SpawnLocalShip(UserID ownerUserID)
    {
        Debug.LogFormat($"GameController.SpawnLocalShip({ownerUserID})");

        var randomShipSpawnerIndex = _randomNumberGenerator.Next(0, ShipSpawners.Length - 1);
        var randomRotation = Convert.ToSingle(_randomNumberGenerator.NextDouble()) * 360F;
        
        var ship = ShipSpawners[randomShipSpawnerIndex].SpawnShip(
            AssetManager, 
            LocalShipIndex, 
            LocalColorIndex, 
            ShipsContainer.transform);
        ship.transform.rotation = Quaternion.Euler(0F, 0F, randomRotation);
        
        ship.Initialize(ownerUserID, true, Networking);
        ship.OnShipDestroyed += HandleShipDestroyed;
        ship.OnLocalPlayerRequestQuit += () => { OnLocalPlayerRequestQuit?.Invoke(); };

        _shipControllers.Add(ownerUserID, ship);

        Assert.IsNull(LocalPlayerShip);
        LocalPlayerShip = ship;

        Networking.SendMessageToAll(
            new SpawnShipState(
                LocalShipIndex, 
                LocalColorIndex, 
                ship.transform.position.x, 
                ship.transform.position.y,
                randomRotation));
    }

    public void SpawnRemoteShip(UserID ownerUserID, int shipIndex, int colorIndex, float posX, float posY, float rotation)
    {
        Debug.LogFormat($"GameController.SpawnRemoteShip({ownerUserID})");

        DestroyShip(ownerUserID);

        var ship = ShipSpawners[0].SpawnShip(AssetManager, shipIndex, colorIndex, ShipsContainer.transform);
        ship.transform.position = new Vector2(posX, posY);
        ship.transform.rotation = Quaternion.Euler(0F, 0F, rotation);
        
        ship.Initialize(ownerUserID, false, Networking);

        _shipControllers.Add(ownerUserID, ship);
    }

    public void SpawnLocalAsteroids(int minAsteroids, int maxAsteroids)
    {
        Debug.LogFormat($"GameController.SpawnLocalAsteroids({minAsteroids}, {maxAsteroids})");

        var randomAsteroidQuantity = _randomNumberGenerator.Next(minAsteroids, maxAsteroids);
        for (var index = 0; index < randomAsteroidQuantity; index++)
        {
            SpawnLocalAsteroid();
        }
    }

    public void SpawnLocalAsteroid()
    {
        Debug.Log("GameController.SpawnLocalAsteroid()");

        var randomAsteroidSpawnerIndex = _randomNumberGenerator.Next(0, AsteroidSpawners.Length - 1);
        var randomRotation = Convert.ToSingle(_randomNumberGenerator.NextDouble()) * 360F;
        var randomAsteroidIndex = _randomNumberGenerator.Next(0, AssetManager.AsteroidPrefabs.Length - 1);

        var asteroid = AsteroidSpawners[randomAsteroidSpawnerIndex].SpawnAsteroid(AssetManager, randomAsteroidIndex, AsteroidsContainer.transform);

        var id = _asteroidControllers.Count();
        asteroid.Initialize(Networking, id, true);

        _asteroidControllers.Add(asteroid);

        Networking.SendMessageToAll(
            new SpawnAsteroidState(
                id,
                randomAsteroidIndex,
                asteroid.transform.position.x,
                asteroid.transform.position.y));
    }

    private void SpawnRemoteAsteroid(int id, int asteroidIndex, float posX, float posY)
    {
        Debug.Log("GameController.SpawnRemoteAsteroid()");

        var asteroid = AsteroidSpawners[0].SpawnAsteroid(AssetManager, asteroidIndex, AsteroidsContainer.transform);
        asteroid.transform.position = new Vector2(posX, posY);

        asteroid.Initialize(Networking, id, false);

        _asteroidControllers.Add(asteroid);
    }

    private void OnValidate()
    {
        Assert.IsNotNull(Networking);
        Assert.IsNotNull(AssetManager);
        Assert.IsNotNull(InputModule);
        Assert.IsNotNull(GameCamera);

        Assert.IsTrue(0 < ShipSpawners.Length);

        Assert.IsNotNull(ShipsContainer);
    }

    private void DestroyShip(UserID owningUserID)
    {
        Debug.LogFormat($"GameController.DestroyShip({owningUserID})");

        if (!_shipControllers.ContainsKey(owningUserID))
        {
            return;
        }

        var ship = _shipControllers[owningUserID];
        _shipControllers.Remove(owningUserID);

        if (ship == LocalPlayerShip)
        {
            ship.OnShipDestroyed -= HandleShipDestroyed;
            LocalPlayerShip = null;
        }

        ship.gameObject.SetActive(false);
        ship.transform.parent = null;

        Destroy(ship.gameObject);
    }

    private void DestroyAsteroid(int asteroidIndex)
    {
        Debug.LogFormat($"GameController.DestroyAsteroid({asteroidIndex})");

        if (_asteroidControllers.Count <= asteroidIndex)
        {
            return;
        }

        var asteroid = _asteroidControllers[asteroidIndex];
        _asteroidControllers.RemoveAt(asteroidIndex);

        asteroid.gameObject.SetActive(false);
        asteroid.transform.parent = null;

        Destroy(asteroid.gameObject);
    }

    private void HandleShipDestroyed(ShipController ship, UserID byWhoUserID)
    {
        Debug.LogFormat($"GameController.HandleShipDestroyed({ship.OwningSessionMemberId})");

        var isLocalShip = ship == LocalPlayerShip;
        var shipUserID = ship.OwningSessionMemberId;

        ship.myShipBehavior.Explode();
        DestroyShip(shipUserID);

        if (isLocalShip)
        {
            if (byWhoUserID == null)
            {
                byWhoUserID = shipUserID;
            }

            Networking.SendMessageToAll(new DestroyShipState(shipUserID, byWhoUserID));
            OnShipDestroyed?.Invoke(shipUserID, byWhoUserID);
        }
    }

    private void HandleGoodbyeNetworkReceived(UserID userID)
    {
        DestroyShip(userID);
    }

    private void HandleGameStateReceived(UserID senderUserID, GameState gameState)
    {
        // nothing to do here for now...
    }

    private void HandlePlayerGameStateReceived(UserID senderUserID, PlayerGameState playerGameState)
    {
        // nothing to do here for now...
    }

    private void HandleSpawnAsteroidStateReceived(UserID senderUserID, SpawnAsteroidState spawnAsteroidState)
    {
        SpawnRemoteAsteroid(
            spawnAsteroidState.Id,
            spawnAsteroidState.AsteroidIndex,
            spawnAsteroidState.PosX,
            spawnAsteroidState.PosY);
    }

    private void HandleSpawnShipStateReceived(UserID senderUserID, SpawnShipState spawnShipState)
    {
        SpawnRemoteShip(
            senderUserID, 
            spawnShipState.ShipIndex, 
            spawnShipState.ColorIndex, 
            spawnShipState.PosX,
            spawnShipState.PosY,
            spawnShipState.Rotation);
    }

    private void HandleDestroyShipStateReceived(UserID senderUserID, DestroyShipState destroyShipState)
    {
        if (_shipControllers.ContainsKey(destroyShipState.DestroyedUserID))
        {
            var ship = _shipControllers[destroyShipState.DestroyedUserID];
            ship.myShipBehavior.Explode();
            DestroyShip(destroyShipState.DestroyedUserID);

            OnShipDestroyed?.Invoke(destroyShipState.DestroyedUserID, destroyShipState.DestroyerUserID);
        }
    }
}
