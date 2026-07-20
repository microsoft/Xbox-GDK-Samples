//--------------------------------------------------------------------------------------
// RemoteShipInputProvider.cs
//
// The input provider that handles input received for a remote player's ship.
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

public class RemoteShipInputProvider : BaseShipInputProvider
{
    public float CorrectionDistanceThreshold = 1.0f;
    public float CorrectionThrustMultiplier = 2.5f;

    private UpdateShipState _latestShipState;

    public override void Initialize(SessionNetwork network, ShipController shipController)
    {
        base.Initialize(network, shipController);

        _network.OnNetworkMessage_UpdateShipState_Received += HandleUpdateShipState;
        _network.OnNetworkMessage_UpdateWeaponFireState_Recieved += HandleFireWeaponState;
    }

    public override bool IsLocal()
    {
        return false;
    }

    private void OnDestroy()
    {
        _network.OnNetworkMessage_UpdateShipState_Received -= HandleUpdateShipState;
        _network.OnNetworkMessage_UpdateWeaponFireState_Recieved -= HandleFireWeaponState;
    }

    private void HandleUpdateShipState(ulong senderXuid, UpdateShipState shipState)
    {
        if (_shipController != null && (senderXuid == _shipController.OwningSessionMemberId))
        {
            UpdateShip(shipState);
        }
    }

    // set fire state according to reliable network state messages that we recieve...
    private void HandleFireWeaponState(ulong senderXuid, UpdateWeaponFireState fireState)
    {
        if(fireState == null)
        { return; }

        if (_shipController != null && (senderXuid == _shipController.OwningSessionMemberId) && fireState.FireWeapon)
        {
            _shipController.HandleFireWeapon();
        }
    }

    // move input vectors according to unreliable network state messages that we receive...
    private void UpdateShip(UpdateShipState updateShipState)
    {
        if (updateShipState == null)
        { return; }

        _latestShipState = updateShipState;

        MoveInput = new Vector2(_latestShipState.MoveX, _latestShipState.MoveY);
        LookInput = new Vector2(_latestShipState.LookX, _latestShipState.LookY);

        // nudge toward position if the ship differs too much?

        var positionalDifference = new Vector2(
            _latestShipState.PosX - transform.position.x,
            _latestShipState.PosY - transform.position.y);

        if (positionalDifference.magnitude >= CorrectionDistanceThreshold)
        {
            positionalDifference.Normalize();
            if (_shipController != null)
            {
                _shipController.MyRigidBody.AddForce(positionalDifference * _shipController.Thrust * CorrectionThrustMultiplier);
            }
            else
            {
                _network.OnNetworkMessage_UpdateShipState_Received -= HandleUpdateShipState;
            }
        }
    }
}
