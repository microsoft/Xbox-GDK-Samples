//--------------------------------------------------------------------------------------
// CollisionManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Manager.h"
#include "BatchRemovalCollection.h"

namespace NetRumble
{

class GameplayObject;

struct CollisionResult
{
    float                           Distance = 0;
    DirectX::SimpleMath::Vector2    Normal;
    GameplayObject*                 GameplayObject = nullptr;

    bool operator<(const CollisionResult &rhs) const { return Distance < rhs.Distance; }
};

class CollisionManager : public Manager
{
public:
    BatchRemovalCollection<std::shared_ptr<GameplayObject>> &Collection() { return m_collection; }
    RECT Dimensions() const { return m_dimensions; }
    void SetDimensions(RECT dimensions) { m_dimensions = dimensions; }
    std::vector<RECT>& Barriers() { return m_barriers; }
    void Update(float elapsedTime);
    void Collide(GameplayObject* gameplayObject, const DirectX::SimpleMath::Vector2 &movement);
    DirectX::SimpleMath::Vector2 FindSpawnPoint(GameplayObject* gameplayObject, float radius);
    void Explode(GameplayObject* source, GameplayObject* target, float damageAmount, const DirectX::SimpleMath::Vector2 &position, float damageRadius, bool damageOwner);

private:
    // The ratio of speed to damage applied, for explosions.
    static constexpr float speedDamageRatio = 0.5f;

    // The number of times that the FindSpawnPoint method will try to find a point.
    static constexpr int findSpawnPointAttempts = 25;

    DirectX::SimpleMath::Vector2 MoveAndCollide(GameplayObject* gameplayObject, const DirectX::SimpleMath::Vector2 &movement);
    void AdjustVelocities(GameplayObject* actor1, GameplayObject* actor2);

    BatchRemovalCollection<std::shared_ptr<GameplayObject>> m_collection;
    RECT m_dimensions = {};
    std::vector<RECT> m_barriers;
    std::vector<CollisionResult> m_collisionResults;
    std::mutex m_lock;
};

}
