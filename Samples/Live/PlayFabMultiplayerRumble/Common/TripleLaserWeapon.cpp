//--------------------------------------------------------------------------------------
// TripleLaserWeapon.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "TripleLaserWeapon.h"

#include "LaserProjectile.h"
#include "Ship.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX::SimpleMath;

TripleLaserWeapon::TripleLaserWeapon(Ship* owner) :
    LaserWeapon(owner)
{
    m_fireDelay = 0.3f;
}

void TripleLaserWeapon::CreateProjectiles(const DirectX::SimpleMath::Vector2 &direction)
{
    // calculate the direction vectors for the second and third projectiles
    Vector2 directionV2{ direction.x, direction.y };
    float rotation = std::acosf(directionV2.Dot(Vector2(0.0f, -1.0f)));
    rotation *= ((-Vector2::UnitY).Dot(Vector2(direction.y, -direction.x)) > 0.0f) ? 1.0f : -1.0f;

    Vector2 direction2{ std::sinf(rotation - c_laserSpreadRadians), -std::cosf(rotation - c_laserSpreadRadians) };
    Vector2 direction3{ std::sinf(rotation + c_laserSpreadRadians), -std::cosf(rotation + c_laserSpreadRadians) };

    // create the first projectile
    auto projectile1 = std::make_shared<LaserProjectile>(m_owner, direction);
    projectile1->Initialize();
    m_owner->Projectiles.push_back(projectile1);

    // create the second projectile
    auto projectile2 = std::make_shared<LaserProjectile>(m_owner, direction2);
    projectile2->Initialize();
    m_owner->Projectiles.push_back(projectile2);

    // create the second projectile
    auto projectile3 = std::make_shared<LaserProjectile>(m_owner, direction3);
    projectile3->Initialize();
    m_owner->Projectiles.push_back(projectile3);
}
