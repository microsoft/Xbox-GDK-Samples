//--------------------------------------------------------------------------------------
// ParticleManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "BatchRemovalCollection.h"
#include "GameplayObject.h"
#include "IManager.h"

namespace PlayFabMultiplayerRumble
{

enum class ParticleEffectType
{
    ShipSpawn,
    ShipExplosion,
    RocketTrail,
    RocketExplosion,
    MineExplosion,
    LaserExplosion,
};

enum class SpriteBlendMode
{
    Alpha,
    Additive
};

class Particle
{
public:
    void Update(float elapsedTime, float angularVelocity, float scaleDeltaPerSecond, float opacityDeltaPerSecond);

    float TimeRemaining = 0;
    DirectX::SimpleMath::Vector2 Position;
    DirectX::SimpleMath::Vector2 Velocity;
    DirectX::SimpleMath::Vector2 Acceleration;
    float Scale = 0;
    float Rotation = 0;
    float Opacity = 0;
};

class ParticleCache
{
public:
    ParticleCache(size_t count);
    ~ParticleCache();

    void Reset();
    std::shared_ptr<Particle> GetNextParticle();
    void ReleaseParticle(std::shared_ptr<Particle> p);

    inline int TotalCount() const { return static_cast<int>(Particles.size()); }
    inline int FreeCount() const { return static_cast<int>(freeParticles.size()); }
    inline int UsedCount() const { return static_cast<int>(Particles.size() - (int)freeParticles.size()); }

    std::vector<std::shared_ptr<Particle>> Particles;

private:
    std::queue<std::shared_ptr<Particle>> freeParticles;
};

class ParticleSystem
{
public:
    ParticleSystem() noexcept;

    std::shared_ptr<ParticleSystem> Clone();

    void Initialize();
    void Reset();
    void Update(float elapsedTime);
    void Draw(RenderContext* renderContext);
    void Stop(bool immediately);

    inline bool IsActive() const { return active || TimeRemaining > 0.0f; }

    std::wstring Name;
    int ParticleCount;

    DirectX::SimpleMath::Vector2 Position;
    DirectX::XMVECTORF32 Color;
    std::wstring TextureName;
    DirectX::SimpleMath::Vector2 TextureOrigin;
    SpriteBlendMode BlendMode;

    float Duration;
    float TimeRemaining;
    float InitialDelay;
    float InitialDelayRemaining;

    int ParticlesPerSecond;
    float ReleaseRate;
    float ReleaseTimer;
    float DurationMinimum;
    float DurationMaximum;
    float VelocityMinimum;
    float VelocityMaximum;
    float AccelerationMinimum;
    float AccelerationMaximum;
    float ScaleMinimum;
    float ScaleMaximum;
    float OpacityMinimum;
    float OpacityMaximum;
    float ReleaseAngleMinimum;
    float ReleaseAngleMaximum;
    float ReleaseDistanceMinimum;
    float ReleaseDistanceMaximum;

    float AngularVelocity;
    float ScaleDeltaPerSecond;
    float OpacityDeltaPerSecond;

private:
    void GenerateParticles(float elapsedTime);
    void UpdateParticles(float elapsedTime);
    void InitializeParticle(std::shared_ptr<Particle> particle);

    bool active;
    TextureHandle texture;
    std::unique_ptr<ParticleCache> particles;
};

class ParticleEffect
{
public:
    ParticleEffect();

    std::shared_ptr<ParticleEffect> Clone();

    void Initialize();
    void Reset();
    void Update(float elapsedTime);
    void Draw(RenderContext* renderContext, SpriteBlendMode blendMode);
    void Stop(bool immediately);

    inline DirectX::SimpleMath::Vector2 GetPosition() const { return position; }
    void SetPosition(DirectX::SimpleMath::Vector2 newPosition);
    void SetFollowOffset(DirectX::SimpleMath::Vector2 offset);

    inline bool IsActive() const { return active; }

    std::wstring Name;
    std::shared_ptr<GameplayObject> FollowObject;

    std::vector<std::shared_ptr<ParticleSystem>> ParticleSystems;

private:
    DirectX::SimpleMath::Vector2 position;
    DirectX::SimpleMath::Vector2 followOffset;
    bool active;
    bool hasFollowOffset;
};

class ParticleEffectManager : public IManager
{
public:
    ~ParticleEffectManager() = default;
    void Update(float elapsedTime);
    void Draw(RenderContext* renderContext, SpriteBlendMode blendMode);
    void Initialize();

    std::shared_ptr<ParticleEffect> SpawnEffect(ParticleEffectType effectType, DirectX::SimpleMath::Vector2 position);
    std::shared_ptr<ParticleEffect> SpawnEffect(ParticleEffectType effectType, std::shared_ptr<GameplayObject> gameplayObject);
    std::shared_ptr<ParticleEffect> SpawnEffect(ParticleEffectType effectType, DirectX::SimpleMath::Vector2 position, std::shared_ptr<GameplayObject> gameplayObject);

    void RegisterParticleEffect(ParticleEffectType effectType, std::function<std::shared_ptr<ParticleEffect>(void) > effectFactory, int initialCount);
    void UnregisterParticleEffect(ParticleEffectType effectType);

    // Replacement functions for the XML content the XNA version used
    static std::shared_ptr<ParticleEffect> CreateLaserExplosionEffect();
    static std::shared_ptr<ParticleEffect> CreateMineExplosionEffect();
    static std::shared_ptr<ParticleEffect> CreateRocketExplosionEffect();
    static std::shared_ptr<ParticleEffect> CreateRocketTrailEffect();
    static std::shared_ptr<ParticleEffect> CreateShipExplosionEffect();
    static std::shared_ptr<ParticleEffect> CreateShipSpawnEffect();

private:
    std::map<ParticleEffectType, std::vector<std::shared_ptr<ParticleEffect>>> particleEffectCache;
    BatchRemovalCollection<std::shared_ptr<ParticleEffect>> activeParticleEffects;
};

}
