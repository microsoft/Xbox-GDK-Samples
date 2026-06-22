//--------------------------------------------------------------------------------------
// ParticleManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ParticleManager.h"

#include "Managers.h"
#include "GameplayObject.h"
#include "RandomMath.h"

using namespace NetRumble;
using namespace DirectX;
using namespace DirectX::SimpleMath;

void Particle::Update(float elapsedTime, float angularVelocity, float scaleDeltaPerSecond, float opacityDeltaPerSecond)
{
    Velocity.x += Acceleration.x * elapsedTime;
    Velocity.y += Acceleration.y * elapsedTime;

    Position.x += Velocity.x * elapsedTime;
    Position.y += Velocity.y * elapsedTime;

    Rotation += angularVelocity * elapsedTime;
    Scale += scaleDeltaPerSecond * elapsedTime;

    if (Scale < 0.0f)
    {
        Scale = 0.0f;
    }

    Opacity = std::min(std::max(Opacity + opacityDeltaPerSecond * elapsedTime, 0.0f), 1.0f);
}

ParticleCache::ParticleCache(size_t count) :
    Particles(count)
{
    for (size_t i = 0; i < count; i++)
    {
        Particles[i] = std::make_shared<Particle>();
        freeParticles.push(Particles[i]);
    }
}

ParticleCache::~ParticleCache()
{
    for (size_t i = 0; i < Particles.size(); i++)
    {
        Particles[i] = nullptr;
    }
}

void ParticleCache::Reset()
{
    freeParticles = std::queue<std::shared_ptr<Particle>>();

    for (size_t i = 0; i < Particles.size(); i++)
    {
        Particles[i]->TimeRemaining = 0.0f;
        freeParticles.push(Particles[i]);
    }
}

std::shared_ptr<Particle> ParticleCache::GetNextParticle()
{
    std::shared_ptr<Particle> p;

    if (freeParticles.size() > 0)
    {
        p = freeParticles.front();
        freeParticles.pop();
    }

    return p;
}

void ParticleCache::ReleaseParticle(std::shared_ptr<Particle> p)
{
    if (p)
    {
        freeParticles.push(p);
    }
}


ParticleSystem::ParticleSystem() noexcept :
    Name(L"DefaultParticleSystem"),
    ParticleCount(256),
    Position(SimpleMath::Vector2(0, 0)),
    Color(Colors::White),
    TextureName(L"default_particle"),
    TextureOrigin(SimpleMath::Vector2(0, 0)),
    BlendMode(SpriteBlendMode::Alpha),
    Duration((std::numeric_limits<float>::max)()),
    TimeRemaining((std::numeric_limits<float>::max)()),
    InitialDelay(0.0f),
    InitialDelayRemaining(0.0f),
    ParticlesPerSecond(128),
    ReleaseRate(0.25f),
    ReleaseTimer(0.0f),
    DurationMinimum(1.0f),
    DurationMaximum(1.0f),
    VelocityMinimum(16.0f),
    VelocityMaximum(32.0f),
    AccelerationMinimum(0.0f),
    AccelerationMaximum(0.0f),
    ScaleMinimum(1.0f),
    ScaleMaximum(1.0f),
    OpacityMinimum(1.0f),
    OpacityMaximum(1.0f),
    ReleaseAngleMinimum(0.0f),
    ReleaseAngleMaximum(360.0f),
    ReleaseDistanceMinimum(0.0f),
    ReleaseDistanceMaximum(0.0f),
    AngularVelocity(0.0f),
    ScaleDeltaPerSecond(0.0f),
    OpacityDeltaPerSecond(0.0f),
    active(false),
    texture{},
    particles(nullptr)
{
}

std::shared_ptr<ParticleSystem> ParticleSystem::Clone()
{
    auto clone = std::make_shared<ParticleSystem>();

    clone->Name = this->Name;
    clone->ParticleCount = this->ParticleCount;

    clone->Position = this->Position;
    clone->Color = this->Color;
    clone->TextureName = this->TextureName;
    clone->BlendMode = this->BlendMode;

    clone->Duration = this->Duration;
    clone->InitialDelay = this->InitialDelay;

    clone->ParticlesPerSecond = this->ParticlesPerSecond;
    clone->ReleaseRate = this->ReleaseRate;
    clone->DurationMinimum = this->DurationMinimum;
    clone->DurationMaximum = this->DurationMaximum;
    clone->VelocityMinimum = this->VelocityMinimum;
    clone->VelocityMaximum = this->VelocityMaximum;
    clone->AccelerationMinimum = this->AccelerationMinimum;
    clone->AccelerationMaximum = this->AccelerationMaximum;
    clone->ScaleMinimum = this->ScaleMinimum;
    clone->ScaleMaximum = this->ScaleMaximum;
    clone->OpacityMinimum = this->OpacityMinimum;
    clone->OpacityMaximum = this->OpacityMaximum;
    clone->ReleaseAngleMinimum = this->ReleaseAngleMinimum;
    clone->ReleaseAngleMaximum = this->ReleaseAngleMaximum;
    clone->ReleaseDistanceMinimum = this->ReleaseDistanceMinimum;
    clone->ReleaseDistanceMaximum = this->ReleaseDistanceMaximum;

    clone->AngularVelocity = this->AngularVelocity;
    clone->ScaleDeltaPerSecond = this->ScaleDeltaPerSecond;
    clone->OpacityDeltaPerSecond = this->OpacityDeltaPerSecond;

    return clone;
}

void ParticleSystem::Initialize()
{
    // calculate the release rate
    ReleaseRate = 1.0f / (float)ParticlesPerSecond;

    // create the cache
    particles = std::make_unique<ParticleCache>(ParticleCount);

    // load the texture
    try
    {
        texture = Managers::Get<ContentManager>()->LoadTexture(TextureName.c_str());
    }
    catch (std::exception)
    {
        texture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\Particles\\defaultParticle.png");
    }

    // calculate the origin on the texture
    TextureOrigin = SimpleMath::Vector2(static_cast<float>(texture.Texture->Width()) / 2.0f, static_cast<float>(texture.Texture->Height()) / 2.0f);

    // allow us to start updating and drawing
    active = true;
}

void ParticleSystem::Reset()
{
    // reset the cache
    particles->Reset();

    // reset the timers
    TimeRemaining = Duration;
    InitialDelayRemaining = InitialDelay;

    // allow us to start updating and drawing
    active = true;
}

void ParticleSystem::Update(float elapsedTime)
{
    // if the system isn't active, dont' update at all
    if (!IsActive())
    {
        return;
    }

    // update the initial delay
    if (InitialDelayRemaining > 0.0f)
    {
        InitialDelayRemaining -= elapsedTime;
        return;
    }

    // generate new particles
    GenerateParticles(elapsedTime);

    // update the existing particles
    UpdateParticles(elapsedTime);

    // update the active flag, based on if there are any used particles
    active = particles->UsedCount() > 0;
}

void ParticleSystem::Draw(RenderContext* renderContext)
{
    // only draw if we're active
    if (!IsActive())
    {
        return;
    }

    // draw each particle
    for (auto &particle : particles->Particles)
    {
        if (particle->TimeRemaining > 0.0f)
        {
            Color.f[3] = particle->Opacity;

            renderContext->Draw(
                texture,
                particle->Position,
                particle->Rotation,
                particle->Scale,
                Color);
        }
    }
}

void ParticleSystem::Stop(bool immediately)
{
    // halt generation
    TimeRemaining = 0.0f;

    // halt updating/drawing of the particles if requested
    if (immediately)
    {
        active = false;
    }
}

void ParticleSystem::GenerateParticles(float elapsedTime)
{
    if (TimeRemaining <= 0.0f)
    {
        return;
    }

    // update the timer
    TimeRemaining -= elapsedTime;

    // release some particles if it's time
    ReleaseTimer += elapsedTime;
    while (ReleaseTimer >= ReleaseRate)
    {
        // only get new particles if you can
        auto particle = particles->GetNextParticle();
        if (particle == nullptr)
        {
            break;
        }
        else
        {
            // initialize the new particle 
            InitializeParticle(particle);
            // reduce the release timer for the release rate of a particle
            ReleaseTimer -= ReleaseRate;
        }
    }
}

void ParticleSystem::UpdateParticles(float elapsedTime)
{
    for (size_t i = 0; i < particles->Particles.size(); ++i)
    {
        if (particles->Particles[i]->TimeRemaining > 0.0f)
        {
            // update the timer on the particle
            particles->Particles[i]->TimeRemaining -= elapsedTime;

            // if the particle just timed out on this update, 
            // add it to the freed-list.
            if (particles->Particles[i]->TimeRemaining <= 0.0f)
            {
                particles->ReleaseParticle(particles->Particles[i]);
                continue;
            }

            // update the particle
            particles->Particles[i]->Update(
                elapsedTime,
                AngularVelocity,
                ScaleDeltaPerSecond,
                OpacityDeltaPerSecond
                );
        }
    }
}

void ParticleSystem::InitializeParticle(std::shared_ptr<Particle> particle)
{
    float t = 0.0f;

    // safety-check the parameter
    if (particle == nullptr)
    {
        throw std::exception("particle cannot be null");
    }

    // set the time remaining on the new particle
    particle->TimeRemaining = RandomMath::RandomBetween(DurationMinimum, DurationMaximum);

    // generate a random direction
    SimpleMath::Vector2 direction = RandomMath::RandomDirection(ReleaseAngleMinimum, ReleaseAngleMaximum);

    // set the graphics data on the new particle
    t = RandomMath::RandomBetween(ReleaseDistanceMinimum, ReleaseDistanceMaximum);
    particle->Position = Position + direction * t;

    t = RandomMath::RandomBetween(VelocityMinimum, VelocityMaximum);
    particle->Velocity = direction * t;

    if (particle->Velocity.LengthSquared() > 0.0f)
    {
        t = RandomMath::RandomBetween(AccelerationMinimum, AccelerationMaximum);
        particle->Acceleration = direction * t;
    }
    else
    {
        particle->Acceleration = SimpleMath::Vector2(0, 0);
    }
    particle->Rotation = RandomMath::RandomBetween(0.0f, DirectX::XM_2PI);
    particle->Scale = RandomMath::RandomBetween(ScaleMinimum, ScaleMaximum);
    particle->Opacity = RandomMath::RandomBetween(OpacityMinimum, OpacityMaximum);
}

ParticleEffect::ParticleEffect() :
    Name(L"DefaultParticleEffect"),
    FollowObject(nullptr),
    position(SimpleMath::Vector2(0, 0)),
    followOffset(SimpleMath::Vector2(0, 0)),
    active(false),
    hasFollowOffset(false)
{
}

std::shared_ptr<ParticleEffect> ParticleEffect::Clone()
{
    auto clone = std::make_shared<ParticleEffect>();

    // copy the data
    clone->Name = this->Name;
    clone->SetPosition(this->position);
    clone->followOffset = this->followOffset;
    clone->hasFollowOffset = this->hasFollowOffset;

    // copy each system
    for (auto &system : ParticleSystems)
    {
        clone->ParticleSystems.push_back(system->Clone());
    }

    return clone;
}

void ParticleEffect::Initialize()
{
    // initialize each system
    for (auto &system : ParticleSystems)
    {
        system->Initialize();
    }

    // allow us to start updating and drawing
    active = true;
}

void ParticleEffect::Reset()
{
    // reset each system
    for (auto &system : ParticleSystems)
    {
        system->Reset();
    }

    // allow us to start updating and drawing
    active = true;
}

void ParticleEffect::Update(float elapsedTime)
{
    // update the position based on the follow-object, if any
    if (FollowObject != nullptr)
    {
        if (FollowObject->Active())
        {
            SetPosition(FollowObject->Position);
        }
        else
        {
            FollowObject = nullptr;
            Stop(false);
        }
    }

    // update each system
    active = false;
    for (auto &system : ParticleSystems)
    {
        if (system->IsActive())
        {
            system->Update(elapsedTime);
            active = true;
        }
    }
}

void ParticleEffect::Draw(RenderContext* renderContext, SpriteBlendMode blendMode)
{
    if (!active)
    {
        return;
    }

    // draw each system
    for (auto &system : ParticleSystems)
    {
        if (system->BlendMode == blendMode)
        {
            system->Draw(renderContext);
        }
    }
}

void ParticleEffect::Stop(bool immediately)
{
    // stop all of the systems
    for (auto &system : ParticleSystems)
    {
        system->Stop(immediately);
    }

    // halt updating/drawing of the particles if requested
    if (immediately)
    {
        active = false;
    }
}
    
void ParticleEffect::SetPosition(DirectX::SimpleMath::Vector2 newPosition)
{
    position = newPosition;
    SimpleMath::Vector2 adjustedPosition = position;

    if (hasFollowOffset && FollowObject != nullptr)
    {
        SimpleMath::Vector2 velocity = FollowObject->Velocity;
        velocity.Normalize();
        // rotate the offset by the direction angle of the object
        SimpleMath::Vector2 offset = SimpleMath::Vector2(followOffset.x * velocity.x - followOffset.y * velocity.y, followOffset.x * velocity.y + followOffset.y * velocity.x);
        adjustedPosition += offset;
    }

    for (auto &system : ParticleSystems)
    {
        system->Position = adjustedPosition;
    }
}

void ParticleEffect::SetFollowOffset(DirectX::SimpleMath::Vector2 offset)
{
    followOffset = offset;
    hasFollowOffset = !(followOffset.x == 0.0f && followOffset.y == 0.0f);
}

void ParticleEffectManager::Initialize()
{
    RegisterParticleEffect(ParticleEffectType::LaserExplosion, ParticleEffectManager::CreateLaserExplosionEffect, 40);
    RegisterParticleEffect(ParticleEffectType::MineExplosion, ParticleEffectManager::CreateMineExplosionEffect, 8);
    RegisterParticleEffect(ParticleEffectType::RocketTrail, ParticleEffectManager::CreateRocketTrailEffect, 16);
    RegisterParticleEffect(ParticleEffectType::RocketExplosion, ParticleEffectManager::CreateRocketExplosionEffect, 24);
    RegisterParticleEffect(ParticleEffectType::ShipSpawn, ParticleEffectManager::CreateShipSpawnEffect, 4);
    RegisterParticleEffect(ParticleEffectType::ShipExplosion, ParticleEffectManager::CreateShipExplosionEffect, 4);
}

void ParticleEffectManager::Update(float elapsedTime)
{
    for (auto &effect : activeParticleEffects)
    {
        if (effect->IsActive())
        {
            effect->Update(elapsedTime);

            if (!effect->IsActive())
            {
                activeParticleEffects.QueuePendingRemoval(effect);
            }
        }
    }

    activeParticleEffects.ApplyPendingRemovals();
}

void ParticleEffectManager::Draw(RenderContext* renderContext, SpriteBlendMode blendMode)
{
    for (auto &effect : activeParticleEffects)
    {
        if (effect->IsActive())
        {
            effect->Draw(renderContext, blendMode);
        }
    }
}

std::shared_ptr<ParticleEffect> ParticleEffectManager::SpawnEffect(ParticleEffectType effectType, DirectX::SimpleMath::Vector2 position)
{
    return SpawnEffect(effectType, position, nullptr);
}

std::shared_ptr<ParticleEffect> ParticleEffectManager::SpawnEffect(ParticleEffectType effectType, std::shared_ptr<GameplayObject> gameplayObject)
{
    return SpawnEffect(effectType, gameplayObject->Position, gameplayObject);
}

std::shared_ptr<ParticleEffect> ParticleEffectManager::SpawnEffect(ParticleEffectType effectType, DirectX::SimpleMath::Vector2 position, std::shared_ptr<GameplayObject> gameplayObject)
{
    std::shared_ptr<ParticleEffect> particleEffect;

    auto itr = particleEffectCache.find(effectType);
    if (itr != particleEffectCache.end())
    {
        auto availableSystems = itr->second;

        for (auto &system : availableSystems)
        {
            if (system->IsActive() == false)
            {
                particleEffect = system;
                break;
            }
        }

        if (particleEffect == nullptr)
        {
            particleEffect = availableSystems[0]->Clone();
            particleEffect->Initialize();
            availableSystems.push_back(particleEffect);
        }
    }

    if (particleEffect != nullptr)
    {
        particleEffect->Reset();
        particleEffect->FollowObject = gameplayObject;
        particleEffect->SetPosition(position);
        activeParticleEffects.push_back(particleEffect);
    }

    return particleEffect;
}

void ParticleEffectManager::RegisterParticleEffect(ParticleEffectType effectType, std::function<std::shared_ptr<ParticleEffect>(void) > effectFactory, int initialCount)
{
    auto itr = particleEffectCache.find(effectType);
    if (itr != particleEffectCache.end())
    {
        return;
    }

    auto effect = effectFactory();
    effect->Initialize();
    effect->Stop(true);
    particleEffectCache[effectType].push_back(effect);

    for (int i = 1; i < initialCount; i++)
    {
        auto cloneEffect = effect->Clone();
        cloneEffect->Initialize();
        cloneEffect->Stop(true);
        particleEffectCache[effectType].push_back(cloneEffect);
    }
}

void ParticleEffectManager::UnregisterParticleEffect(ParticleEffectType effectType)
{
    auto itr = particleEffectCache.find(effectType);
    if (itr != particleEffectCache.end())
    {
        for (auto &effect : itr->second)
        {
            auto activeItr = std::find(activeParticleEffects.begin(), activeParticleEffects.end(), effect);
            if (activeItr != activeParticleEffects.end())
            {
                activeParticleEffects.erase(activeItr);
            }
        }

        particleEffectCache.erase(itr);
    }
}



// Particle effect factory methods.
// These replace the XML files that the XNA version of NetRumble deserialized.
// You shouldn't directly create these effects, but instead use these functions with the ParticleEffectManager::RegisterParticleEffect function.

std::shared_ptr<ParticleEffect> ParticleEffectManager::CreateLaserExplosionEffect()
{
    auto effect = std::make_shared<ParticleEffect>();
    effect->SetPosition(SimpleMath::Vector2(0, 0));

    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Smoke";
        system->ParticleCount = 36;
        system->ParticlesPerSecond = 256;
        system->Duration = 1;
        system->InitialDelay = 0;
        system->TextureName = L"Assets\\Textures\\Particles\\smoke.png";
        system->BlendMode = SpriteBlendMode::Alpha;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color = Colors::Gray;
        system->VelocityMinimum = 4;
        system->VelocityMaximum = 8;
        system->AccelerationMinimum = -2;
        system->AccelerationMaximum = -1;
        system->ScaleMinimum = 0.07f;
        system->ScaleMaximum = 0.12f;
        system->OpacityMinimum = 0.5f;
        system->OpacityMaximum = 0.75f;
        system->DurationMinimum = 1;
        system->DurationMaximum = 1;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 6;
        system->ScaleDeltaPerSecond = 0.0f;
        system->AngularVelocity = 1.5f;
        system->OpacityDeltaPerSecond = -0.4f;
        effect->ParticleSystems.push_back(system);
    }

    effect->Name = L"LaserExplosion";
    effect->SetPosition(SimpleMath::Vector2(0, 0));

    return effect;
}

std::shared_ptr<ParticleEffect> ParticleEffectManager::CreateMineExplosionEffect()
{
    auto effect = std::make_shared<ParticleEffect>();

    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Fire";
        system->ParticleCount = 128;
        system->ParticlesPerSecond = 2048;
        system->Duration = 2;
        system->InitialDelay = 0;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color.f[0] = 1.0f;
        system->Color.f[1] = 0.3019f;
        system->Color.f[2] = 0.0654f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 48;
        system->VelocityMaximum = 150;
        system->AccelerationMinimum = -16;
        system->AccelerationMaximum = -4;
        system->ScaleMinimum = 0.2f;
        system->ScaleMaximum = 0.3f;
        system->OpacityMinimum = 0.5f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 2;
        system->DurationMaximum = 2;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 8;
        system->ScaleDeltaPerSecond = 0.5f;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -0.5f;
        effect->ParticleSystems.push_back(system);
    }
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Sparks";
        system->ParticleCount = 64;
        system->ParticlesPerSecond = 2048;
        system->Duration = 2;
        system->InitialDelay = 0.05f;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color.f[0] = 1.0f;
        system->Color.f[1] = 0.5f;
        system->Color.f[2] = 0.1254f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 48;
        system->VelocityMaximum = 64;
        system->AccelerationMinimum = -32;
        system->AccelerationMaximum = -32;
        system->ScaleMinimum = 0.1f;
        system->ScaleMaximum = 0.2f;
        system->OpacityMinimum = 0.25f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 2;
        system->DurationMaximum = 2;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 0.0f;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -0.5f;
        effect->ParticleSystems.push_back(system);
    }
    
    effect->Name = L"MineExplosion";
    effect->SetPosition(SimpleMath::Vector2(0, 0));

    return effect;
}

std::shared_ptr<ParticleEffect> ParticleEffectManager::CreateRocketExplosionEffect()
{
    auto effect = std::make_shared<ParticleEffect>();

    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Fire";
        system->ParticleCount = 128;
        system->ParticlesPerSecond = 2048;
        system->Duration = 3;
        system->InitialDelay = 0.1f;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color.f[0] = 1.0f;
        system->Color.f[1] = 0.502f;
        system->Color.f[2] = 0.1255f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 20;
        system->VelocityMaximum = 100;
        system->AccelerationMinimum = -24;
        system->AccelerationMaximum = -4;
        system->ScaleMinimum = 0.25f;
        system->ScaleMaximum = 0.5f;
        system->OpacityMinimum = 0.25f;
        system->OpacityMaximum = 0.75f;
        system->DurationMinimum = 3;
        system->DurationMaximum = 3;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 8;
        system->ScaleDeltaPerSecond = 0.5f;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -0.5f;
        effect->ParticleSystems.push_back(system);
    }
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Sparks";
        system->ParticleCount = 64;
        system->ParticlesPerSecond = 2048;
        system->Duration = 3;
        system->InitialDelay = 0.05f;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color.f[0] = 1.0f;
        system->Color.f[1] = 1.0f;
        system->Color.f[2] = 0.1255f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 150;
        system->VelocityMaximum = 200;
        system->AccelerationMinimum = -48;
        system->AccelerationMaximum = -32;
        system->ScaleMinimum = 0.2f;
        system->ScaleMaximum = 0.4f;
        system->OpacityMinimum = 0.25f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 3;
        system->DurationMaximum = 3;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 0.0f;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -0.5f;
        effect->ParticleSystems.push_back(system);
    }
    
    effect->Name = L"RocketExplosion";
    effect->SetPosition(SimpleMath::Vector2(0, 0));

    return effect;
}

std::shared_ptr<ParticleEffect> ParticleEffectManager::CreateRocketTrailEffect()
{
    auto effect = std::make_shared<ParticleEffect>();

    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Smoke";
        system->ParticleCount = 256;
        system->ParticlesPerSecond = 64;
        system->Duration = 3;
        system->InitialDelay = 0;
        system->TextureName = L"Assets\\Textures\\Particles\\smoke.png";
        system->BlendMode = SpriteBlendMode::Alpha;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color = Colors::Gray;
        system->VelocityMinimum = 8;
        system->VelocityMaximum = 24;
        system->AccelerationMinimum = 0;
        system->AccelerationMaximum = 0;
        system->ScaleMinimum = 0.25f;
        system->ScaleMaximum = 0.5f;
        system->OpacityMinimum = 0.75f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 3;
        system->DurationMaximum = 3;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 0.25f;
        system->AngularVelocity = 1.0f;
        system->OpacityDeltaPerSecond = -0.5f;
        effect->ParticleSystems.push_back(system);
    }
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Fire";
        system->ParticleCount = 256;
        system->ParticlesPerSecond = 128;
        system->Duration = 3;
        system->InitialDelay = 0;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color.f[0] = 1.0f;
        system->Color.f[1] = 0.502f;
        system->Color.f[2] = 0.251f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 8;
        system->VelocityMaximum = 12;
        system->AccelerationMinimum = 0;
        system->AccelerationMaximum = 0;
        system->ScaleMinimum = 0.75f;
        system->ScaleMaximum = 1.0f;
        system->OpacityMinimum = 1.0f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 1;
        system->DurationMaximum = 1.5f;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 0.25f;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -8.0f;
        effect->ParticleSystems.push_back(system);
    }
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Sparks";
        system->ParticleCount = 128;
        system->ParticlesPerSecond = 40;
        system->Duration = 3;
        system->InitialDelay = 0;
        system->TextureName = L"Assets\\Textures\\Particles\\spark.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color.f[0] = 1.0f;
        system->Color.f[1] = 1.0f;
        system->Color.f[2] = 0.1255f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 8;
        system->VelocityMaximum = 12;
        system->AccelerationMinimum = 0;
        system->AccelerationMaximum = 0;
        system->ScaleMinimum = 1.0f;
        system->ScaleMaximum = 1.0f;
        system->OpacityMinimum = 0.25f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 3;
        system->DurationMaximum = 3;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 0.0f;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -0.33f;
        effect->ParticleSystems.push_back(system);
    }
    
    effect->Name = L"RocketTrail";
    effect->SetPosition(SimpleMath::Vector2(0, 0));

    return effect;
}

std::shared_ptr<ParticleEffect> ParticleEffectManager::CreateShipExplosionEffect()
{
    auto effect = std::make_shared<ParticleEffect>();
    
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Wave";
        system->ParticleCount = 1;
        system->ParticlesPerSecond = 100;
        system->Duration = 4;
        system->InitialDelay = 0;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color.f[0] = 0.7529f;
        system->Color.f[1] = 0.6274f;
        system->Color.f[2] = 0.5019f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 0;
        system->VelocityMaximum = 0;
        system->AccelerationMinimum = 0;
        system->AccelerationMaximum = 0;
        system->ScaleMinimum = 0;
        system->ScaleMaximum = 0;
        system->OpacityMinimum = 0.75f;
        system->OpacityMaximum = 0.75f;
        system->DurationMinimum = 4;
        system->DurationMaximum = 4;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 24;
        system->AngularVelocity = 0;
        system->OpacityDeltaPerSecond = -0.5f;
        effect->ParticleSystems.push_back(system);
    }
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Smoke";
        system->ParticleCount = 320;
        system->ParticlesPerSecond = 1024;
        system->Duration = 4;
        system->InitialDelay = 0.25f;
        system->TextureName = L"Assets\\Textures\\Particles\\smoke.png";
        system->BlendMode = SpriteBlendMode::Alpha;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color = Colors::Gray;
        system->VelocityMinimum = 16;
        system->VelocityMaximum = 128;
        system->AccelerationMinimum = -32;
        system->AccelerationMaximum = -8;
        system->ScaleMinimum = 0.25f;
        system->ScaleMaximum = 0.5f;
        system->OpacityMinimum = 0.75f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 4;
        system->DurationMaximum = 4;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 0.75f;
        system->AngularVelocity = 1.5f;
        system->OpacityDeltaPerSecond = -0.25f;
        effect->ParticleSystems.push_back(system);
    }
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Fire";
        system->ParticleCount = 512;
        system->ParticlesPerSecond = 2048;
        system->Duration = 4.0f;
        system->InitialDelay = 0.1f;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color.f[0] = 1.0f;
        system->Color.f[1] = 0.5f;
        system->Color.f[2] = 0.1254f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 16;
        system->VelocityMaximum = 128;
        system->AccelerationMinimum = -32;
        system->AccelerationMaximum = -8;
        system->ScaleMinimum = 0.5f;
        system->ScaleMaximum = 0.75f;
        system->OpacityMinimum = 0.25f;
        system->OpacityMaximum = 0.75f;
        system->DurationMinimum = 4;
        system->DurationMaximum = 4;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 2.5f;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -0.5f;
        effect->ParticleSystems.push_back(system);
    }
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Sparks";
        system->ParticleCount = 256;
        system->ParticlesPerSecond = 1024;
        system->Duration = 4;
        system->InitialDelay = 0;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(0, 0);
        system->Color.f[0] = 1.0f;
        system->Color.f[1] = 1.0f;
        system->Color.f[2] = 0.1254f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 128;
        system->VelocityMaximum = 256;
        system->AccelerationMinimum = -64;
        system->AccelerationMaximum = -32;
        system->ScaleMinimum = 0.1f;
        system->ScaleMaximum = 0.2f;
        system->OpacityMinimum = 0.25f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 4;
        system->DurationMaximum = 4;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 0.0f;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -0.33f;
        effect->ParticleSystems.push_back(system);
    }
    
    effect->Name = L"ShipExplosion";
    effect->SetPosition(SimpleMath::Vector2(0, 0));

    return effect;
}

std::shared_ptr<ParticleEffect> ParticleEffectManager::CreateShipSpawnEffect()
{
    auto effect = std::make_shared<ParticleEffect>();
    
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Noise";
        system->ParticleCount = 256;
        system->ParticlesPerSecond = 32;
        system->Duration = 0;
        system->InitialDelay = 0;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(400, 300);
        system->Color.f[0] = 0.7529f;
        system->Color.f[1] = 0.5019f;
        system->Color.f[2] = 0.1254f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 0;
        system->VelocityMaximum = 0;
        system->AccelerationMinimum = 0;
        system->AccelerationMaximum = 0;
        system->ScaleMinimum = 0;
        system->ScaleMaximum = 0;
        system->OpacityMinimum = 1.0f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 3;
        system->DurationMaximum = 3;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 32;
        system->ScaleDeltaPerSecond = 0.5f;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -0.5f;
        effect->ParticleSystems.push_back(system);
    }
    {
        auto system = std::make_shared<ParticleSystem>();
        system->Name = L"Blast";
        system->ParticleCount = 8;
        system->ParticlesPerSecond = 1000;
        system->Duration = 1;
        system->InitialDelay = 0;
        system->TextureName = L"Assets\\Textures\\Particles\\particle.png";
        system->BlendMode = SpriteBlendMode::Additive;
        system->Position = SimpleMath::Vector2(400, 300);
        system->Color.f[0] = 0.25f;
        system->Color.f[1] = 0.25f;
        system->Color.f[2] = 0.75f;
        system->Color.f[3] = 1.0f;
        system->VelocityMinimum = 0;
        system->VelocityMaximum = 0;
        system->AccelerationMinimum = 0;
        system->AccelerationMaximum = 0;
        system->ScaleMinimum = 0;
        system->ScaleMaximum = 0;
        system->OpacityMinimum = 0.75f;
        system->OpacityMaximum = 1.0f;
        system->DurationMinimum = 3;
        system->DurationMaximum = 3;
        system->ReleaseAngleMinimum = 0;
        system->ReleaseAngleMaximum = 360;
        system->ReleaseDistanceMinimum = 0;
        system->ReleaseDistanceMaximum = 0;
        system->ScaleDeltaPerSecond = 4;
        system->AngularVelocity = 0.0f;
        system->OpacityDeltaPerSecond = -0.5f;
        effect->ParticleSystems.push_back(system);
    }
    
    effect->Name = L"ShipSpawn";
    effect->SetPosition(SimpleMath::Vector2(400, 300));

    return effect;
}
