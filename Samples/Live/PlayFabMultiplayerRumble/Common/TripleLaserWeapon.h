//--------------------------------------------------------------------------------------
// TripleLaserWeapon.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "LaserWeapon.h"

namespace PlayFabMultiplayerRumble
{

class TripleLaserWeapon final : public LaserWeapon
{
public:
    TripleLaserWeapon(Ship* owner);
    virtual ~TripleLaserWeapon() = default;

    virtual void CreateProjectiles(const DirectX::SimpleMath::Vector2 &direction) override;
    virtual WeaponType GetWeaponType() const override { return WeaponType::TripleLaser; }

    // The spread of the second and third laser projectiles' directions, in radians
    static constexpr float c_laserSpreadRadians = DirectX::XMConvertToRadians(2.5f);
};

}
