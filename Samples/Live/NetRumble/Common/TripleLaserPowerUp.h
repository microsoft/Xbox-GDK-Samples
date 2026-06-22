//--------------------------------------------------------------------------------------
// TripleLaserPowerUp.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "PowerUp.h"

namespace NetRumble
{

class TripleLaserPowerUp final : public PowerUp
{
public:
    TripleLaserPowerUp();

    virtual bool OnTouch(GameplayObject* target) override;
    virtual void Draw(float elapsedTime, RenderContext* renderContext) override;
    virtual PowerUpType GetPowerUpType() const override { return PowerUpType::TripleLaser; }

private:
    TextureHandle m_powerUpTexture;
};

}
