//--------------------------------------------------------------------------------------
// SpawnController.cs
//
// A game object behavior for spawning various game objects like ships and asteroids.
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

public class SpawnController : MonoBehaviour
{
    public float RingRadius = 1F;

    public ShipController SpawnShip(GameAssetManager assetManager, int shipIndex, int colorIndex, Transform parent)
    {
        if (shipIndex < assetManager.PlayerShipPrefabs.Length)
        {
            var shipColor = assetManager.PlayerShipColorChoices[colorIndex];

            var newShip = Instantiate<ShipController>(assetManager.PlayerShipPrefabs[shipIndex], parent);
            newShip.transform.position = transform.position + GetRandomOffset();

            // automatically take care of the ship color
            newShip.shipColor = shipColor;
            
            // NOTE: we leave the rotation up to the caller since the spawner only dictates position
            return newShip;
        }
        else
        {
            return null;
        }
    }

    public AsteroidController SpawnAsteroid(GameAssetManager assetManager, int asteroidIndex, Transform parent)
    {
        if (asteroidIndex < assetManager.AsteroidPrefabs.Length)
        {
            var newAsteroid = Instantiate<AsteroidController>(assetManager.AsteroidPrefabs[asteroidIndex], parent);
            newAsteroid.transform.position = transform.position + GetRandomOffset();

            // NOTE: in this case we can come up with a random rotation since rotation does not
            // matter for asteroids
            var randomRotation = Random.value * 360F;
            newAsteroid.transform.rotation = Quaternion.Euler(0F, 0F, randomRotation);

            return newAsteroid;
        }
        else
        {
            return null;
        }
    }

    private Vector3 GetRandomOffset()
    {
        var randomAngle = Random.value * 360F;
        var x = RingRadius * Mathf.Cos(randomAngle);
        var y = RingRadius * Mathf.Sin(randomAngle);
        return new Vector3(x, y);
    }
}
