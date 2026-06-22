//--------------------------------------------------------------------------------------
// RocketPowerUp.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "RocketPowerUp.h"

#include "Managers.h"
#include "RocketWeapon.h"
#include "Ship.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

RocketPowerUp::RocketPowerUp()
{
    m_powerUpTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\powerupRocket.png");
}

bool RocketPowerUp::OnTouch(GameplayObject* target)
{
    // check the target, if we have one
    if (target != nullptr)
    {
        if (target->GetType() == GameplayObjectType::Ship)
        {
            auto ship = static_cast<Ship*>(target);
            ship->PrimaryWeapon = std::make_shared<RocketWeapon>(ship);
        }
    }

    return PowerUp::OnTouch(target);
}

void RocketPowerUp::Draw(float elapsedTime, RenderContext* renderContext)
{
    PowerUp::Draw(elapsedTime, renderContext, m_powerUpTexture, Colors::White);
}
