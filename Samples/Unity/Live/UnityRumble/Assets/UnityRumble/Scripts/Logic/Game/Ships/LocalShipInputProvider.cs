//--------------------------------------------------------------------------------------
// LocalShipInputProvider.cs
//
// The input provider class that handles input controls for the local player's ship.
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

public class LocalShipInputProvider : BaseShipInputProvider
{
    public float NetworkUpdateTime = 0.1F;
    public float DeadZone = 0.2F;

    private float _timeSinceLastUpdate = 0F;
    private float _timeSinceLastMouseLook = 0F;
    private bool _usingMouse = false;

    public override void Initialize(SessionNetwork network, ShipController shipController)
    {
        base.Initialize(network, shipController);
    }

    public override bool IsLocal()
    {
        return true;
    }

    public void OnFire(InputValue inputValue)
    {
        _shipController.HandleFireWeapon();
        _network.SendMessageToAll(new UpdateWeaponFireState(inputValue.isPressed));

        if(_usingMouse)
        {
            // extend aim time while shooting
            _timeSinceLastMouseLook = 0F;
        }
    }

    public void OnLook(InputValue inputValue)
    {
        LookInput = inputValue.Get<Vector2>();

        if (Math.Abs(LookInput.x) <= DeadZone &&
            Math.Abs(LookInput.y) <= DeadZone)
        {
            LookInput = Vector2.zero;
        }

        _usingMouse = false;
    }

    public void OnMouseLook(InputValue inputValue)
    {
        Vector3 inputVector = inputValue.Get<Vector2>();
        Vector3 worldVector = Camera.main.ScreenToWorldPoint(inputVector) - transform.position;
        LookInput = new(worldVector.x, worldVector.y);
        _usingMouse = true;
        _timeSinceLastMouseLook = 0F;
    }

    public void OnMove(InputValue inputValue)
    {
        MoveInput = inputValue.Get<Vector2>();

        if (Math.Abs(MoveInput.x) <= DeadZone &&
            Math.Abs(MoveInput.y) <= DeadZone)
        {
            MoveInput = Vector2.zero;
        }
    }

    private void Update()
    {
        _timeSinceLastUpdate += Time.deltaTime;

        if (_timeSinceLastUpdate >= NetworkUpdateTime)
        {
            _timeSinceLastUpdate = 0F;
            Vector2 velocity;
#if UNITY_6000_0_OR_NEWER
            velocity = _shipController.MyRigidBody.linearVelocity;
#else
            velocity = _shipController.MyRigidBody.velocity;
#endif
            _network.SendMessageToAll(
                new UpdateShipState(
                    transform.position.x, transform.position.y,
                    velocity.x, velocity.y,
                    MoveInput.x, MoveInput.y,
                    LookInput.x, LookInput.y));
        }

        if (_usingMouse)
        {
            _timeSinceLastMouseLook += Time.deltaTime;
            if(_timeSinceLastMouseLook >= 0.5F)
            {
                LookInput = Vector2.zero;
            }
        }
    }
}
