//--------------------------------------------------------------------------------------
// RocketProjectile.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Projectile.h"

namespace NetRumble
{

class ParticleEffect;

class RocketProjectile final : public Projectile
{
public:
    RocketProjectile(Ship* owner, DirectX::SimpleMath::Vector2 direction);

    void Initialize();
    virtual void Draw(float elapsedTime, RenderContext* renderContext) override;
    virtual void Die(GameplayObject* source, bool cleanupOnly) override;

    // speed of the laser-bolt projectiles
    static constexpr float c_initialSpeed = 650.0f;

protected:
    TextureHandle m_rocketTexture;
    std::shared_ptr<ParticleEffect> m_rocketTrailEffect;
};

}
