//--------------------------------------------------------------------------------------
// RocketWeapon.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Weapon.h"

namespace PlayFabMultiplayerRumble
{
class RocketWeapon final : public Weapon
{

public:
    RocketWeapon(Ship* owner);
    virtual ~RocketWeapon() = default;

    virtual void CreateProjectiles(const DirectX::SimpleMath::Vector2 &direction) override;
    virtual WeaponType GetWeaponType() const override { return WeaponType::Rocket; }
};

}
