//--------------------------------------------------------------------------------------
// Asteroid.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "GameplayObject.h"

namespace NetRumble
{

class Asteroid final : public GameplayObject
{
public:
    // jameslen: There are 4 variants of asteriod, tell me which one it is at creation time.
    Asteroid(float radius, int variation);

    virtual void Update(float elapsedTime) override;
    void Draw(float elapsedTime, RenderContext* renderContext);
    virtual bool OnTouch(GameplayObject* target) override;

    virtual GameplayObjectType GetType() const override { return GameplayObjectType::Asteroid; }

    int Variation;

    static constexpr int c_Variations = 3;

    // The ratio of the mass of an asteroid to its radius.
    static constexpr float c_massRadiusRatio = 0.5f;

    // The ratio of the mass of an asteroid to its radius.
    static constexpr float c_lifeRadiusRatio = 10.0f;

    // The amount of drag applied to velocity per second, 
    // as a percentage of velocity.
    static constexpr float c_dragPerSecond = 0.15f;

    // Scalar to convert the velocity / mass ratio into a "nice" rotational value.
    static constexpr float c_velocityMassRatioToRotationScalar = 0.0017f;

    // Scalar for calculated damage values that asteroids apply to players.
    static constexpr float c_momentumToDamageScalar = 0.007f;

    // The minimum possible initial speed for asteroids.
    static constexpr float c_initialSpeedMinimum = 32.0f;

    // The minimum possible initial speed for asteroids.
    static constexpr float c_initialSpeedMaximum = 96.0f;

    // The minimum speed for drag to have any effect.
    static constexpr float c_minSpeedFromDrag = 25.0f;

private:
    TextureHandle m_texture;
};

}
