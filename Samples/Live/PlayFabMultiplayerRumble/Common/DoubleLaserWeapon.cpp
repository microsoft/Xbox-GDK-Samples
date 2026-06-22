//--------------------------------------------------------------------------------------
// DoubleLaserWeapon.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DoubleLaserWeapon.h"

#include "LaserProjectile.h"
#include "Ship.h"

#include <SimpleMath.h>

using namespace PlayFabMultiplayerRumble;
using namespace DirectX::SimpleMath;

DoubleLaserWeapon::DoubleLaserWeapon(Ship* owner) : LaserWeapon(owner)
{
}

void DoubleLaserWeapon::CreateProjectiles(const Vector2 &direction)
{
    // calculate the spread of the laser bolts
    Vector2 cross = Vector2(-direction.y, direction.x) * c_laserSpread;

    // create the first new projectile
    auto projectile1 = std::make_shared<LaserProjectile>(m_owner, direction);
    projectile1->Initialize();
    projectile1->Position += cross;
    m_owner->Projectiles.push_back(projectile1);

    // create the second projectile
    auto projectile2 = std::make_shared<LaserProjectile>(m_owner, direction);
    projectile2->Initialize();
    projectile2->Position -= cross;
    m_owner->Projectiles.push_back(projectile2);
}
