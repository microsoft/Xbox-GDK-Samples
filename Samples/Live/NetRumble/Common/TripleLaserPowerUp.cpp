//--------------------------------------------------------------------------------------
// TripleLaserPowerUp.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "TripleLaserPowerUp.h"

#include "Managers.h"
#include "TripleLaserWeapon.h"
#include "Ship.h"

using namespace NetRumble;
using namespace DirectX;

TripleLaserPowerUp::TripleLaserPowerUp()
{
    m_powerUpTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\powerupTripleLaser.png");
}

bool TripleLaserPowerUp::OnTouch(GameplayObject* target)
{
    // check the target, if we have one
    if (target != nullptr)
    {
        if (target->GetType() == GameplayObjectType::Ship)
        {
            auto ship = static_cast<Ship*>(target);
            ship->PrimaryWeapon = std::make_shared<TripleLaserWeapon>(ship);
        }
    }

    return PowerUp::OnTouch(target);
}

void TripleLaserPowerUp::Draw(float elapsedTime, RenderContext* renderContext)
{
    PowerUp::Draw(elapsedTime, renderContext, m_powerUpTexture, Colors::White);
}
