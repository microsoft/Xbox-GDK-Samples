//--------------------------------------------------------------------------------------
// ShipInput.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ShipInput.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

ShipInput::ShipInput(const GamePad::State& gamePadState) : 
    LeftStick(SimpleMath::Vector2(gamePadState.thumbSticks.leftX, gamePadState.thumbSticks.leftY)), 
    RightStick(SimpleMath::Vector2(gamePadState.thumbSticks.rightX, gamePadState.thumbSticks.rightY)), 
    MineFired(gamePadState.triggers.right >= 0.9f)
{
}

ShipInput::ShipInput(const DirectX::Keyboard::State& keyboardState) :
    LeftStick(SimpleMath::Vector2(0, 0)),
    RightStick(SimpleMath::Vector2(0, 0)),
    MineFired(keyboardState.Space)
{
    if (keyboardState.W)
    {
        LeftStick.y += 1.f;
    }
    if (keyboardState.A)
    {
        LeftStick.x -= 1.f;
    }
    if (keyboardState.S)
    {
        LeftStick.y -= 1.f;
    }
    if (keyboardState.D)
    {
        LeftStick.x += 1.f;
    }
    LeftStick.Normalize();

    if (keyboardState.Up)
    {
        RightStick.y += 1.f;
    }
    if (keyboardState.Left)
    {
        RightStick.x -= 1.f;
    }
    if (keyboardState.Down)
    {
        RightStick.y -= 1.f;
    }
    if (keyboardState.Right)
    {
        RightStick.x += 1.f;
    }
}

void ShipInput::Add(const ShipInput & moreInput)
{
    LeftStick += moreInput.LeftStick;
    RightStick += moreInput.RightStick;
    if (!MineFired)
    {
        MineFired = moreInput.MineFired;
    }
}

std::vector<uint8_t> ShipInput::Serialize()
{
    auto bufferLen = sizeof(ShipInput);
    auto buffer = new uint8_t[bufferLen];

    if (!buffer)
    {
        return std::vector<uint8_t>();
    }

    CopyMemory(buffer, this, bufferLen);

    std::vector<uint8_t> serialized(buffer, buffer + bufferLen);

    delete[] buffer;
    return serialized;
}

void ShipInput::Deserialize(const std::vector<uint8_t> &data)
{
    CopyMemory(this, data.data(), sizeof(ShipInput));
}
