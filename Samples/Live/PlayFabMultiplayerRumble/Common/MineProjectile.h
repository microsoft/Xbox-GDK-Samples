//--------------------------------------------------------------------------------------
// MineProjectile.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Projectile.h"

namespace PlayFabMultiplayerRumble
{

class MineProjectile final : public Projectile
{
public:
    MineProjectile(Ship* owner, DirectX::SimpleMath::Vector2 direction);

    virtual void Update(float elapsedTime) override;
    virtual void Draw(float elapsedTime, RenderContext* renderContext) override;
    virtual bool TakeDamage(GameplayObject* source, float damageAmount) override;
    virtual void Die(GameplayObject* source, bool cleanupOnly) override;

private:
    bool anchored = false;
    TextureHandle m_texture;
};

}
