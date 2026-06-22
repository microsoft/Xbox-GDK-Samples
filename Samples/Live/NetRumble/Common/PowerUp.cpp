//--------------------------------------------------------------------------------------
// PowerUp.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PowerUp.h"
#include "RandomMath.h"
#include "Managers.h"
#include "Ship.h"

using namespace NetRumble;

namespace
{
    const std::map<PowerUpType, std::wstring> c_PowerUpStrings = {
        { PowerUpType::Unknown, L"Unknown" },
        { PowerUpType::DoubleLaser, L"DoubleLaser" },
        { PowerUpType::TripleLaser, L"TripleLaser" },
        { PowerUpType::Rocket, L"Rocket" }
    };
}

PowerUp::PowerUp()
{
    Mass = static_cast<float>(INT_MAX);
    Radius = 20.0f;
}

void PowerUp::Initialize()
{
    if (!Active())
    {
        // play the spawn sound effect
        Managers::Get<AudioManager>()->PlaySound(L"powerup_spawn");
    }

    GameplayObject::Initialize();
}

bool PowerUp::OnTouch(GameplayObject* target)
{
    // check the target, if we have one
    if (target != nullptr)
    {
        if (target->GetType() == GameplayObjectType::Ship)
        {
            // play the "power-up picked up" sound effect
            Managers::Get<AudioManager>()->PlaySound(L"powerup_touch");

            // kill the power-up
            Die(target, false);

            if (static_cast<Ship*>(target)->IsLocal)
            {
                //auto typeName = c_PowerUpStrings.find(GetPowerUpType());

                //web::json::value metadata;
                //metadata[L"PowerUpType"] = web::json::value(typeName->second); // TODO: Telemetry Update

                //Managers::Get<IOnlineManager>()->UpdateStatistic("powerups_collected", 1, metadata);
            }

            // the ship keeps going as if it didn't hit anything
            return false;
        }
    }

    return GameplayObject::OnTouch(target);
}

void PowerUp::Draw(float elapsedTime, RenderContext* renderContext, const TextureHandle &sprite, DirectX::XMVECTOR color)
{
    // update the rotation
    Rotation = rotationSpeed * elapsedTime;

    // adjust the radius to affect the scale
    float oldRadius = Radius;
    pulseTimer += elapsedTime;
    Radius *= 1.0f + pulseAmplitude * std::sin(pulseTimer / pulseRate);
    GameplayObject::Draw(elapsedTime, renderContext, sprite, color);
    Radius = oldRadius;
}

PowerUpType PowerUp::ChooseNextPowerUpType()
{
    int randValue = RandomMath::RandomBetween(1, 5);

    if (randValue == 1 || randValue == 2)
    {
        return PowerUpType::DoubleLaser;
    }
    else if (randValue == 3 || randValue == 4)
    {
        return PowerUpType::TripleLaser;
    }
    else
    {
        return PowerUpType::Rocket;
    }
}
