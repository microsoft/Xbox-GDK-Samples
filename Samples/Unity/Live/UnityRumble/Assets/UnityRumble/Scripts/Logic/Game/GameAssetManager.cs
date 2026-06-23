//--------------------------------------------------------------------------------------
// GameAssetManager.cs
//
// The singleton that gives access to all of the game play related assets.
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

using System.Linq;
using UnityEngine;
using UnityEngine.Assertions;

public class GameAssetManager : MonoBehaviour
{
    public const int PlayerShipTypes = 4;
    public const int EnemyShipTypes = 3;
    public const int AsteroidTypes = 3;
    public const int MinimumShipColorChoices = 3;

    public Color[] PlayerShipColorChoices;
    public Sprite[] PlayerShipSprites;
    public ShipController[] PlayerShipPrefabs;
    public SatelliteController[] EnemyShipPrefabs;
    public AsteroidController[] AsteroidPrefabs;

    private void OnValidate()
    {
        Assert.IsNotNull(PlayerShipColorChoices);
        Assert.IsTrue(PlayerShipColorChoices.Length >= MinimumShipColorChoices);

        Assert.IsNotNull(PlayerShipSprites);
        Assert.IsTrue(PlayerShipSprites.Length == PlayerShipTypes);
        Assert.IsTrue(0 == PlayerShipSprites.Count(sprite => null == sprite));

        Assert.IsNotNull(PlayerShipPrefabs);
        Assert.IsTrue(PlayerShipPrefabs.Length == PlayerShipTypes);
        Assert.IsTrue(0 == PlayerShipPrefabs.Count(prefab => null == prefab));

        Assert.IsNotNull(EnemyShipPrefabs);
        Assert.IsTrue(EnemyShipPrefabs.Length == EnemyShipTypes);
        Assert.IsTrue(0 == EnemyShipPrefabs.Count(prefab => null == prefab));

        Assert.IsNotNull(AsteroidPrefabs);
        Assert.IsTrue(AsteroidPrefabs.Length == AsteroidTypes);
        Assert.IsTrue(0 == AsteroidPrefabs.Count(prefab => null == prefab));
    }
}
