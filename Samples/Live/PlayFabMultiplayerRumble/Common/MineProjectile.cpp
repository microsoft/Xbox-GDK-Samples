//--------------------------------------------------------------------------------------
// MineProjectile.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MineProjectile.h"

#include "Managers.h"
#include "Ship.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

namespace
{
    /// The initial life of this projectile.
    constexpr float c_initialLife = 150.0f;

    /// The initial speed of this projectile.
    constexpr float c_initialSpeed = 80.0f;

    /// The amount of drag applied to velocity per second, 
    /// as a percentage of velocity.
    constexpr float c_dragPerSecond = 0.9f;

    /// The radians-per-second that this object rotates at.
    constexpr float c_rotationRadiansPerSecond = 1.0f;
}

MineProjectile::MineProjectile(Ship* owner, DirectX::SimpleMath::Vector2 direction) : 
    Projectile(owner, direction)
{
    // set the gameplay data
    Velocity *= c_initialSpeed;

    // set the collision data
    Radius = 10.0f;
    Mass = 5.0f;

    // set the projectile data
    m_duration = 20.0f;
    m_damageAmount = 200.0f;
    m_damageRadius = 300.0f;
    m_damageOwner = false;
    Life = c_initialLife;

    m_texture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\mine.png");
}

void MineProjectile::Update(float elapsedTime)
{
    Projectile::Update(elapsedTime);

    Velocity.x -= Velocity.x * elapsedTime * c_dragPerSecond;
    Velocity.y -= Velocity.y * elapsedTime * c_dragPerSecond;

    // once mines near stop after their initial release, they become anchored and unmovable
    if (!anchored)
    {
        float speedSquared = 0.0f;
        XMStoreFloat(&speedSquared, XMVector2LengthSq(XMLoadFloat2(&Velocity)));

        if (speedSquared < 100.0f)
        {
            anchored = true;
        }
    }

    if (anchored)
    {
        Velocity = SimpleMath::Vector2::Zero;
    }

    Rotation += elapsedTime * c_rotationRadiansPerSecond;
}

void MineProjectile::Draw(float elapsedTime, RenderContext* renderContext)
{
    // ignore the parameter color if we have an owner
    GameplayObject::Draw(elapsedTime, renderContext, m_texture, (anchored && m_owner != nullptr) ? m_owner->Color : Colors::White);
}

bool MineProjectile::TakeDamage(GameplayObject* source, float damageAmount)
{
    if (source->GetType() == GameplayObjectType::Projectile)
    {
        Life -= damageAmount;
    }
    else
    {
        Life = 0.0f;
    }

    if (Life <= 0.0f)
    {
        Die(source, false);
    }

    return true;
}

void MineProjectile::Die(GameplayObject* source, bool cleanupOnly)
{
    if (m_active)
    {
        if (!cleanupOnly)
        {
            // play the mine explosion sound
            Managers::Get<AudioManager>()->PlaySound(L"explosion_large");

            // display the mine explosion
            Managers::Get<ParticleEffectManager>()->SpawnEffect(ParticleEffectType::MineExplosion, Position);
        }
    }

    Projectile::Die(source, cleanupOnly);
}
