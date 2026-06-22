//--------------------------------------------------------------------------------------
// Weapon.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Weapon.h"

#include "Managers.h"
#include "Ship.h"

using namespace NetRumble;
using namespace DirectX;

Weapon::Weapon(Ship* owner) :
    m_owner(owner)
{
}

void Weapon::Update(float elapsedTime)
{
    // count down to when the weapon can fire again
    m_timeToNextFire = std::max<float>(m_timeToNextFire - elapsedTime, 0.0f);
}

void Weapon::Fire(const SimpleMath::Vector2 &direction)
{
    // if we can't fire yet, then we're done
    if (m_timeToNextFire > 0.0f)
    {
        return;
    }

    // set the timer
    m_timeToNextFire = m_fireDelay;

    SimpleMath::Vector2 fireDirection = direction;
    fireDirection.Normalize();

    // create and spawn the projectile
    CreateProjectiles(fireDirection);

    // play the sound effect for firing
    if (!m_fireSoundEffect.empty())
    {
        Managers::Get<AudioManager>()->PlaySound(m_fireSoundEffect);
    }
}