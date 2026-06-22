//--------------------------------------------------------------------------------------
// LaserProjectile.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Projectile.h"

namespace PlayFabMultiplayerRumble
{

class LaserProjectile final : public Projectile
{
public:
    LaserProjectile(Ship* owner, DirectX::SimpleMath::Vector2 direction);

    virtual void Draw(float elapsedTime, RenderContext* renderContext) override;
    virtual void Die(GameplayObject* source, bool cleanupOnly) override;

    // speed of the laser-bolt projectiles
    static constexpr float initialSpeed = 640.0f;
private:
    TextureHandle m_texture;
};

}
