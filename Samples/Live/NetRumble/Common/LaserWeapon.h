//--------------------------------------------------------------------------------------
// LaserWeapon.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Weapon.h"

namespace NetRumble
{

class LaserWeapon : public Weapon
{
public:
    LaserWeapon(Ship* owner);
    virtual ~LaserWeapon() = default;

    virtual void CreateProjectiles(const DirectX::SimpleMath::Vector2 &direction) override;
    virtual WeaponType GetWeaponType() const override { return WeaponType::Laser; }
};

}
