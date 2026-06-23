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
using UnityEngine.InputSystem;

public class ShipController : BaseObjectController
{
    public bool isLocal = false;
    public int startingShipHealth = 100;
    private int currentShipHealth = 100;

    public float thrust = 50f;
    public float dampening = 10f;

    public ProjectileBehavior ProjectilePrefab;
    public float fireRate = 0.3f;
    public float fireOffset = 1.2f;
    public float projectileSpeed = 400f;

    public Color shipColor;

    public event Action OnLocalPlayerRequestQuit;
    public event Action<ShipController, UserID> OnShipDestroyed;

    // move related
    public Rigidbody2D myRigidBody;
    public ShipBehavior myShipBehavior;

    public Vector2 LookInput;
    public Vector2 MoveInput;

    public SessionNetwork network;

    private float timeToNextFire = 0;
    private float timeSinceLastUpdate = 0;
    public float networkUpdateTime = 0.1F;
    public float deadZone = 0.2f;

    private UpdateShipState latestShipState;
    public float CorrectionDistanceThreshold = 1.0f;
    public float CorrectionThrustMultiplier = 2.5f;

    private bool _usingMouse = false;
    private float _timeSinceLastMouseLook = 0F;

    public void Initialize(UserID ownerUserID, bool isLocalShip, SessionNetwork inNetwork)
    {
        GetComponent<ShipDamager>().SourceUserID = ownerUserID;

        OwningSessionMemberId = ownerUserID;
        isLocal = isLocalShip;
        network = inNetwork;

        if(!isLocal)
        {
            network.OnNetworkMessage_UpdateShipState_Received += HandleUpdateShipState;
            network.OnNetworkMessage_UpdateWeaponFireState_Received += HandleUpdateWeaponFireState;
        }
    }

    public void OnQuit()
    {
        if (isLocal)
        {
            OnLocalPlayerRequestQuit?.Invoke();
        }
    }

    private void Awake()
    {
        myRigidBody = gameObject.GetComponent<Rigidbody2D>();
        myShipBehavior = gameObject.GetComponent<ShipBehavior>();
        currentShipHealth = startingShipHealth;
    }

    private void Start()
    {
        myShipBehavior.SetShipColor(shipColor);
    }

    private void OnDestroy()
    {
        OnLocalPlayerRequestQuit = null;

        if(!isLocal)
        {
            network.OnNetworkMessage_UpdateShipState_Received -= HandleUpdateShipState;
            network.OnNetworkMessage_UpdateWeaponFireState_Received -= HandleUpdateWeaponFireState;
        }
    }

    public void OnFire(InputValue inputValue)
    {
        if (isLocal)
        {
            HandleFireWeapon();

            if(_usingMouse)
            {
                // extend aim time while shooting
                _timeSinceLastMouseLook = 0F;
            }
        }
    }

    public void OnLook(InputValue inputValue)
    {
        if (isLocal)
        {
            LookInput = inputValue.Get<Vector2>();

            if (Math.Abs(LookInput.x) <= deadZone &&
                Math.Abs(LookInput.y) <= deadZone)
            {
                LookInput = Vector2.zero;
            }

            _usingMouse = false;
        }
    }

    public void OnMouseLook(InputValue inputValue)
    {
        if (isLocal)
        {
            Vector3 inputVector = inputValue.Get<Vector2>();
            Vector3 worldVector = Camera.main.ScreenToWorldPoint(inputVector) - transform.position;
            LookInput = new(worldVector.x, worldVector.y);
            _usingMouse = true;
            _timeSinceLastMouseLook = 0F;
        }
    }

    public void OnMove(InputValue inputValue)
    {
        if (isLocal)
        {
            MoveInput = inputValue.Get<Vector2>();

            if (Math.Abs(MoveInput.x) <= deadZone && 
                Math.Abs(MoveInput.y) <= deadZone)
            {
                MoveInput = Vector2.zero;
            }
        }
    }
    
    private void Update()
    {
        if (isLocal)
        {
            timeToNextFire -= Time.deltaTime;
            
            timeSinceLastUpdate += Time.deltaTime;
            if (timeSinceLastUpdate >= networkUpdateTime)
            {
                timeSinceLastUpdate = 0F;
                Vector2 velocity;
#if UNITY_6000_0_OR_NEWER
                velocity = myRigidBody.linearVelocity;
#else
                velocity = myRigidBody.velocity;
#endif
                network.SendMessageToAll(
                    new UpdateShipState(
                        transform.position.x, transform.position.y,
                        velocity.x, velocity.y,
                        MoveInput.x, MoveInput.y,
                        LookInput.x, LookInput.y));
            }

            if (_usingMouse)
            {
                _timeSinceLastMouseLook += Time.deltaTime;
                if (_timeSinceLastMouseLook >= 0.5F)
                {
                    LookInput = Vector2.zero;
                }
            }
        }
    }

    private void FixedUpdate()
    {
        HandleMoveInput(MoveInput, LookInput);
    }

    private void OnCollisionEnter2D(Collision2D collision)
    {
        // look for a "ShipDamager" behavior on the object we collided with...
        var shipDamager = collision.gameObject.GetComponent<ShipDamager>();
        if (null != shipDamager)
        {
            // ... and apply the appropriate amount of damage
            ApplyDamage(shipDamager.ImpactDamage, shipDamager.SourceUserID);

            // play a shield impact effect at the contact point
            var contact = collision.GetContact(0);
            myShipBehavior.ShieldImpact(contact.point, contact.normal);
        }
    }

    private void ApplyDamage(int damage, UserID damagerUserID)
    {
        Debug.LogFormat($"ShipController.ApplyDamage({damage})");
        currentShipHealth -= damage;
        currentShipHealth = Math.Max(0, currentShipHealth);

        myShipBehavior.SetShieldStrength(Convert.ToSingle(currentShipHealth) / Convert.ToSingle(startingShipHealth));

        if (isLocal && currentShipHealth <= 0)
        {
            OnShipDestroyed?.Invoke(this, damagerUserID);
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
            myRigidBody.AddForce(moveVector * thrust);

            // show engine flare
            showEngineFlare = true;
        }

        if (lookVector != Vector2.zero)
        {
            transform.rotation = Quaternion.LookRotation(Vector3.forward, lookVector);
        }

        // dampen our movement
#if UNITY_6000_0_OR_NEWER
        myRigidBody.AddForce(-myRigidBody.linearVelocity * dampening);
#else
        myRigidBody.AddForce(-myRigidBody.velocity * dampening);
#endif

        // Show the engine flare if there is controller input
        myShipBehavior.FlareEngine(showEngineFlare);
    }

    private void HandleFireWeapon()
    {
        if (timeToNextFire <= 0.0f)
        {
            timeToNextFire = fireRate;
            Fire();
            network.SendMessageToAll(new UpdateWeaponFireState(true));
        }
    }

    private void Fire()
    {
        // Create the projectile
        var bullet = Instantiate(ProjectilePrefab, transform.position + transform.up * fireOffset, transform.rotation);

        bullet.SourceUserID = OwningSessionMemberId;
        bullet.GetComponent<ShipDamager>().SourceUserID = OwningSessionMemberId;

        // Match velocity and set firing force
#if UNITY_6000_0_OR_NEWER
        bullet.GetComponent<Rigidbody2D>().linearVelocity = GetComponent<Rigidbody2D>().linearVelocity;
#else
        bullet.GetComponent<Rigidbody2D>().velocity = GetComponent<Rigidbody2D>().velocity;
#endif
        bullet.GetComponent<Rigidbody2D>().AddForce(transform.up * projectileSpeed);

        // Bullets should match the color of the shooter
        bullet.GetComponent<SpriteRenderer>().color = shipColor;

        // Don't let the projectile collide with us
        Physics2D.IgnoreCollision(GetComponent<Collider2D>(), bullet.GetComponent<Collider2D>());

        // Pew pew
        GetComponent<AudioSource>().Play();
    }

    private void HandleUpdateShipState(UserID senderUserID, UpdateShipState shipState)
    {
        if (senderUserID == OwningSessionMemberId)
        {
            // we need to set our fire and move input vectors according to
            // unreliable network state messages that we receive...

            latestShipState = shipState;

            MoveInput = new Vector2(latestShipState.MoveX, latestShipState.MoveY);
            LookInput = new Vector2(latestShipState.LookX, latestShipState.LookY);

            // nudge toward position if the ship differs too much?

            var positionalDifference = new Vector2(latestShipState.PosX - transform.position.x, latestShipState.PosY - transform.position.y);

            if (positionalDifference.magnitude >= CorrectionDistanceThreshold)
            {
                positionalDifference.Normalize();
                myRigidBody.AddForce(positionalDifference * thrust * CorrectionThrustMultiplier);
            }
        }
    }

    private void HandleUpdateWeaponFireState(UserID senderUserID, UpdateWeaponFireState fireState)
    {
        // fire weapon for ship according to reliable network state messages that we recieve...
        if (senderUserID == OwningSessionMemberId)
        {
            Fire();
        }
    }
}
