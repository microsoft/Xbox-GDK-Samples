//--------------------------------------------------------------------------------------
// Projectile.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Projectile.h"

#include "Managers.h"
#include "Ship.h"

using namespace NetRumble;

Projectile::Projectile(Ship* owner, DirectX::SimpleMath::Vector2 direction) :
    m_owner(owner)
{
    Velocity = direction;
    Position = owner->Position;
    Rotation = std::acos(direction.y); // safe for all angles, but assumes direction is a unit vector
    if (direction.x > 0.0f)
    {
        Rotation *= -1.0f;
    }
}
    
void Projectile::Update(float elapsedTime)
{
    // projectiles can "time out"
    if (m_duration > 0.0f)
    {
        m_duration -= elapsedTime;
        if (m_duration < 0.0f)
        {
            Die(nullptr, false);
        }
    }

    GameplayObject::Update(elapsedTime);
}

/// <summary>
/// Defines the interaction between this projectile and a target GameplayObject
/// when they touch.
/// </summary>
/// <param name="target">The GameplayObject that is touching this one.</param>
/// <returns>True if the objects meaningfully interacted.</returns>
bool Projectile::OnTouch(GameplayObject* target)
{
    // check the target, if we have one
    if (target != nullptr)
    {
        // don't bother hitting any power-ups
        if (target->GetType() == GameplayObjectType::PowerUp)
        {
            return false;
        }

        // don't hit the owner if the damageOwner flag isn't set
        if ((m_damageOwner == false) && (target == m_owner))
        {
            return false;
        }

        // don't hit other projectiles from the same ship
        if (target->GetType() == GameplayObjectType::Projectile)
        {
            auto projectile = static_cast<Projectile*>(target);
            if (projectile->GetOwner() == m_owner)
            {
                return false;
            }
        }

        // asteroids and enemy ships kill all projectiles
        if (target->GetType() == GameplayObjectType::Asteroid || target->GetType() == GameplayObjectType::Ship)
        {
            Life = 0.0f;
        }

        // damage the target
        target->TakeDamage(this, m_damageAmount);
    }

    // either we hit something or the target is null - in either case, check if we die
    if (Life <= 0.0f)
    {
        Die(target, false);
    }

    return GameplayObject::OnTouch(target);
}

void Projectile::Die(GameplayObject* source, bool cleanupOnly)
{
    if (m_active)
    {
        if (!cleanupOnly)
        {
            Managers::Get<CollisionManager>()->Explode(
                this,
                source,
                m_damageAmount,
                Position,
                m_damageRadius,
                m_damageOwner
                );
        }
    }

    GameplayObject::Die(source, cleanupOnly);
}