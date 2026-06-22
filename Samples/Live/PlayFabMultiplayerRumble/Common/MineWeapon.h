//--------------------------------------------------------------------------------------
// MineWeapon.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Weapon.h"

namespace PlayFabMultiplayerRumble
{

class MineWeapon final : public Weapon
{
public:
    MineWeapon(Ship* owner);
    virtual ~MineWeapon() = default;

    // The distance that the mine spawns behind the ship.
    static constexpr float c_mineSpawnDistance = 8.0f;

    virtual void CreateProjectiles(const DirectX::SimpleMath::Vector2 &direction) override;
    virtual WeaponType GetWeaponType() const override { return WeaponType::Mine; }
};

}
