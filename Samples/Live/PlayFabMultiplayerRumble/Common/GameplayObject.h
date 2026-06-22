//--------------------------------------------------------------------------------------
// GameplayObject.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "RenderContext.h"

namespace PlayFabMultiplayerRumble
{

enum class GameplayObjectType
{
    Unknown,
    Ship,
    Asteroid,
    Projectile,
    PowerUp
};

class GameplayObject : public std::enable_shared_from_this<GameplayObject>
{
public:
    GameplayObject();
    virtual ~GameplayObject() = default;

    // Prevent copying.
    GameplayObject(GameplayObject const&) = delete;
    GameplayObject& operator= (GameplayObject const&) = delete;

    void Initialize();
    virtual void Update(float elapsedTime);

    virtual bool OnTouch(GameplayObject* target);
    virtual bool TakeDamage(GameplayObject* source, float damageAmount);
    virtual void Die(GameplayObject* source, bool cleanupOnly);

    // Since we don't have an "is" keyword in C++, objects must be honest and return back what type they are
    virtual GameplayObjectType GetType() const { return GameplayObjectType::Unknown; }

    inline uint32_t GetID() const { return m_uniqueID; }
    inline void SetID(uint32_t id) { m_uniqueID = id; } // careful, should only be called when told by host network message

    inline bool Active() const { return m_active; }

    DirectX::SimpleMath::Vector2 Position = DirectX::SimpleMath::Vector2::Zero;
    float Rotation = 0.0f;
    float Radius = 1.0f;

    DirectX::SimpleMath::Vector2 Velocity = DirectX::SimpleMath::Vector2::Zero;
    float Mass = 1.0f;
    bool CollidedThisFrame = false;

    float Life = 0.0f;
    
protected:
    bool m_active = false;

    void Draw(float elapsedTime, RenderContext *renderContext, const TextureHandle &sprite, DirectX::XMVECTOR color);

private:
    uint32_t m_uniqueID;
    static std::atomic_uint32_t nextUniqueID;
};

}
