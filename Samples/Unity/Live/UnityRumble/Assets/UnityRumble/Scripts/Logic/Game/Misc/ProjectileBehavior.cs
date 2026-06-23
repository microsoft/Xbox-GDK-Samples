//--------------------------------------------------------------------------------------
// ProjectileBehavior.cs
//
// A game behavior for a bullet that can be fired by a player ship.
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

public class ProjectileBehavior : MonoBehaviour
{
    public GameObject ExplosionPrefab;
    public float MaxLifetimeInS = 3.0f;

    public ulong SourceXuid { get; set; }

    private void Start()
    {
        Destroy(gameObject, MaxLifetimeInS);
    }

    private void OnCollisionEnter2D(Collision2D collision)
    {
        // Spawn a small explosion where the laser hits an object
        var explosionInstance = Instantiate(ExplosionPrefab, gameObject.transform.position, Quaternion.identity);
        var explosion = explosionInstance.GetComponent<ParticleSystem>();

        explosion.Play();
        Destroy(explosionInstance, explosion.main.duration);

        // After colliding, destroy the projectile
        Destroy(gameObject);
    }
}
