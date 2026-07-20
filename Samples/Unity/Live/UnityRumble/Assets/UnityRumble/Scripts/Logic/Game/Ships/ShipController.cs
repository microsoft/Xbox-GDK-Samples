//--------------------------------------------------------------------------------------
// ShipController.cs
//
// The game behavior class for managing a ship's input and game play behavior.
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
using UnityEngine;

public class ShipController : BaseObjectController
{
    public int StartingShipHealth = 100;

    public float Thrust = 50f;
    public float Dampening = 10f;

    public ProjectileBehavior ProjectilePrefab;
    public float FireRate = 0.3f;
    public float FireOffset = 1.2f;
    public float ProjectileSpeed = 400f;

    public event Action OnLocalPlayerRequestQuit;
    public event Action<ShipController, ulong> OnShipDestroyed; // parameters: ship that was destroyed, the destroyer xuid

    public Color ShipColor { get; set; }
    public BaseShipInputProvider InputProvider { get; private set; }
    public ShipBehavior MyShipBehavior { get { return _myShipBehavior; } }
    public Rigidbody2D MyRigidBody { get { return _myRigidBody; } }

    private int _currentShipHealth;

    // move related
    private Rigidbody2D _myRigidBody;
    private ShipBehavior _myShipBehavior;

    // fire related
    private float _timeToNextFire = 0.0f;

    public void Initialize(ulong ownerId, BaseShipInputProvider inputProvider)
    {
        base.Initialize(ownerId);

        InputProvider = inputProvider;

        GetComponent<ShipDamager>().SourceXuid = ownerId;
    }

    public void OnQuit()
    {
        if (InputProvider.IsLocal())
        {
            OnLocalPlayerRequestQuit?.Invoke();
        }
    }

    private void Awake()
    {
        _myRigidBody = gameObject.GetComponent<Rigidbody2D>();
        _myShipBehavior = gameObject.GetComponent<ShipBehavior>();
        _currentShipHealth = StartingShipHealth;
    }

    private void Start()
    {
        MyShipBehavior.SetShipColor(ShipColor);
    }

    private void OnDestroy()
    {
        OnLocalPlayerRequestQuit = null;
    }

    private void Update()
    {
        if(null != InputProvider)
        {
            // Update firing timer
            _timeToNextFire -= Time.deltaTime;
        }
    }

    private void FixedUpdate()
    {
        if (null != InputProvider)
        {
            HandleMoveInput(InputProvider.MoveInput, InputProvider.LookInput);
        }
    }

    private void OnCollisionEnter2D(Collision2D collision)
    {
        // look for a "ShipDamager" behavior on the object we collided with...
        var shipDamager = collision.gameObject.GetComponent<ShipDamager>();
        if (null != shipDamager)
        {
            // ... and apply the appropriate amount of damage
            ApplyDamage(shipDamager.ImpactDamage, shipDamager.SourceXuid);

            // play a shield impact effect at the contact point
            var contact = collision.GetContact(0);
            _myShipBehavior.ShieldImpact(contact.point, contact.normal);
        }
    }

    private void ApplyDamage(int damage, ulong damagerXuid)
    {
        Debug.LogFormat("ShipController.ApplyDamage({0})", damage);
        _currentShipHealth -= damage;
        _currentShipHealth = Math.Max(0, _currentShipHealth);

        _myShipBehavior.SetShieldStrength(
            Convert.ToSingle(_currentShipHealth) / Convert.ToSingle(StartingShipHealth));

        if (InputProvider.IsLocal() && _currentShipHealth <= 0)
        {
            OnShipDestroyed?.Invoke(this, damagerXuid);
        }
    }

    private void HandleMoveInput(Vector2 moveVector, Vector2 lookVector)
    {
        var showEngineFlare = false;

        if (moveVector != Vector2.zero)
        {
            // Face the direction of the joystick
            transform.rotation = Quaternion.LookRotation(Vector3.forward, moveVector);

            // Move in the direction of the joystick
            _myRigidBody.AddForce(moveVector * Thrust);

            // show engine flare
            showEngineFlare = true;
        }

        if(lookVector != Vector2.zero)
        {
            transform.rotation = Quaternion.LookRotation(Vector3.forward, lookVector);
        }

        // dampen our movement
#if UNITY_6000_0_OR_NEWER
        _myRigidBody.AddForce(-_myRigidBody.linearVelocity * Dampening);
#else
        _myRigidBody.AddForce(-_myRigidBody.velocity * Dampening);
#endif

        // Show the engine flare if there is controller input
        _myShipBehavior.FlareEngine(showEngineFlare);
    }

    public void HandleFireWeapon()
    {
        // Shoot the projectile
        if(_timeToNextFire <= 0.0f)
        {
            _timeToNextFire = FireRate;
            Fire();
        }
    }

    private void Fire()
    {
        // Create the projectile
        var bullet = Instantiate(
            ProjectilePrefab,
            transform.position + transform.up * FireOffset,
            transform.rotation
        );

        bullet.SourceXuid = OwningSessionMemberId;
        bullet.GetComponent<ShipDamager>().SourceXuid = OwningSessionMemberId;

        // Match velocity and set firing force
#if UNITY_6000_0_OR_NEWER
        bullet.GetComponent<Rigidbody2D>().linearVelocity = GetComponent<Rigidbody2D>().linearVelocity;
#else
        bullet.GetComponent<Rigidbody2D>().velocity = GetComponent<Rigidbody2D>().velocity;
#endif
        bullet.GetComponent<Rigidbody2D>().AddForce(transform.up * ProjectileSpeed);

        // Bullets should match the color of the shooter
        bullet.GetComponent<SpriteRenderer>().color = ShipColor;

        // Don't let the projectile collide with us
        Physics2D.IgnoreCollision(
            GetComponent<Collider2D>(),
            bullet.GetComponent<Collider2D>());

        // Pew pew
        GetComponent<AudioSource>().Play();
    }
}
