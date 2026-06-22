//--------------------------------------------------------------------------------------
// DoubleLaserPowerUp.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "PowerUp.h"

namespace PlayFabMultiplayerRumble
{

class DoubleLaserPowerUp final : public PowerUp
{
public:
    DoubleLaserPowerUp();

    virtual bool OnTouch(GameplayObject* target) override;
    virtual void Draw(float elapsedTime, RenderContext* renderContext) override;
    virtual PowerUpType GetPowerUpType() const override { return PowerUpType::DoubleLaser; }

private:
    TextureHandle _powerUpTexture;
};

}
