//--------------------------------------------------------------------------------------
// DoubleLaserWeapon.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "LaserWeapon.h"

namespace NetRumble
{

class DoubleLaserWeapon final : public LaserWeapon
{
public:
    DoubleLaserWeapon(Ship* owner);
    virtual ~DoubleLaserWeapon() = default;

    virtual void CreateProjectiles(const DirectX::SimpleMath::Vector2 &direction) override;
    virtual WeaponType GetWeaponType() const override { return WeaponType::DoubleLaser; }

    // The distance that the laser bolts are moved off of the owner's position.
    static constexpr float c_laserSpread = 8.0f;
};

}
