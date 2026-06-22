//--------------------------------------------------------------------------------------
// CollisionManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameplayObject.h"
#include "CollisionManager.h"

#include "CollisionMath.h"
#include "RandomMath.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;
using namespace DirectX::SimpleMath;

enum class CollisionSide
{
    Top,
    Left,
    Bottom,
    Right
};

void CollisionManager::Update(float elapsedTime)
{
    std::lock_guard<std::mutex> lock(m_lock);

    m_collection.ApplyPendingRemovals();

    // move each object
    for(auto &item : m_collection)
    {
        if (item->Active())
        {
            // determine how far they are going to move
            Vector2 movement = item->Velocity * elapsedTime;
            
            // only allow collisionManager that have not collided yet 
            // collisionManager frame to collide
            // -- otherwise, objects can "double-hit" and trade their momentum
            if (item->CollidedThisFrame == false)
            {
                movement = MoveAndCollide(item.get(), movement);
            }
            // determine the new position
            item->Position += movement;

            std::array<std::pair<CollisionSide, float>, 4> DistanceToEdge = {
                std::pair<CollisionSide, float>{ CollisionSide::Left, item->Position.x - Dimensions().left },
                std::pair<CollisionSide, float>{ CollisionSide::Right, Dimensions().right - item->Position.x },
                std::pair<CollisionSide, float>{ CollisionSide::Top, item->Position.y - Dimensions().top },
                std::pair<CollisionSide, float>{ CollisionSide::Bottom, Dimensions().bottom - item->Position.y }
            };

            auto min = std::min_element(std::begin(DistanceToEdge), std::end(DistanceToEdge), [](auto &&left, auto &&right)
            {
                return left.second < right.second;
            });

            if (min->second < item->Radius)
            {
                if (item->GetType() == GameplayObjectType::Projectile)
                {
                    item->Die(nullptr, false);
                }
                else
                {
                    Vector2 closestPoint = item->Position;

                    switch (min->first)
                    {
                    case CollisionSide::Left:
                        closestPoint.x = static_cast<float>(Dimensions().left);
                        break;
                    case CollisionSide::Right:
                        closestPoint.x = static_cast<float>(Dimensions().right);
                        break;
                    case CollisionSide::Top:
                        closestPoint.y = static_cast<float>(Dimensions().top);
                        break;
                    case CollisionSide::Bottom:
                        closestPoint.y = static_cast<float>(Dimensions().bottom);
                        break;
                    }

                    auto normal = item->Position - closestPoint;
                    normal.Normalize();

                    float distance = item->Radius - min->second;

                    item->Velocity = Vector2::Reflect(item->Velocity, normal);
                    normal *= distance;
                    item->Position += normal;
                }
            }
        }
    }
}

void CollisionManager::Collide(GameplayObject* gameplayObject, const Vector2 &movement)
{
    m_collisionResults.clear();

    if (!gameplayObject->Active())
        return;

    float movementLength = movement.Length();
    if (movementLength <= 0)
        return;

    // check each gameplayObject
    for(auto &checkActor : m_collection)
    {
        if (gameplayObject == checkActor.get() || !checkActor->Active())
            continue;

        // calculate the target vector
        Vector2 checkVector = checkActor->Position - gameplayObject->Position;
        float checkVectorLength = checkVector.Length();
        if (checkVectorLength <= 0.0f)
        {
            continue;
        }

        float combinedRadius = checkActor->Radius + gameplayObject->Radius;
        float distanceBetween = std::max<float>(checkVectorLength - combinedRadius, 0.0f);

        // check if they could possibly touch no matter the direction
        if (movementLength < distanceBetween)
        {
            continue;
        }

        // determine how much of the movement is bringing the two together
        float movementTowards = movement.Dot(checkVector);

        // check to see if the movement is away from each other
        if (movementTowards < 0.0f)
        {
            continue;
        }

        if (movementTowards < distanceBetween)
        {
            continue;
        }

        CollisionResult result;
        result.Distance = distanceBetween;
        
        result.Normal = checkVector;
        result.Normal.Normalize();
        result.GameplayObject = checkActor.get();

        m_collisionResults.push_back(result);
    }
}

Vector2 CollisionManager::FindSpawnPoint(GameplayObject* spawnedObject, float radius)
{
    // try to find a valid point
    static int maxAttemptsRequired = 1;
    int attemptNum = 1;
    const float spawnPointPadding = 100.0f;
    float paddedRadius = radius + spawnPointPadding;

    Vector2 spawnPoint = Vector2(
        radius + m_dimensions.left + 
        RandomMath::RandomBetween(0.0f, m_dimensions.right - m_dimensions.left - 2 * radius),
        radius + m_dimensions.top + 
        RandomMath::RandomBetween(0.0f, m_dimensions.bottom - m_dimensions.top - 2 * radius));
    for (; attemptNum <= findSpawnPointAttempts; attemptNum++)
    {
        bool valid = true;

        // check the other objects
        for(auto &otherObject : m_collection)
        {
            if (!otherObject->Active() || otherObject.get() == spawnedObject)
            {
                continue;
            }
            if (CollisionMath::CircleCircleIntersect(spawnPoint, paddedRadius, otherObject->Position, otherObject->Radius))
            {
                valid = false;
                break;
            }
        }

        if (valid)
        {
            break;
        }

        spawnPoint = Vector2(
            radius + m_dimensions.left + 
            RandomMath::RandomBetween(0.0f, m_dimensions.right - m_dimensions.left - radius),
            radius + m_dimensions.top + 
            RandomMath::RandomBetween(0.0f, m_dimensions.bottom - m_dimensions.top - radius));
    }

    if (attemptNum > maxAttemptsRequired)
    {
        maxAttemptsRequired = attemptNum;
    }

    if (attemptNum > findSpawnPointAttempts)
    {
        DEBUGLOG("WARNING: FindSpawnPoint could not find suitable point after %d attempts! Spawn collision likely at (%f, %f)!", findSpawnPointAttempts, spawnPoint.x, spawnPoint.y);
    }
    else
    {
        DEBUGLOG("FindSpawnPoint found point (%f, %f) on attempt #%d", spawnPoint.x, spawnPoint.y, attemptNum);
    }
    return spawnPoint;
}

/// <summary>
/// Process an explosion in the world against the objects in it.
/// </summary>
/// <param name="source">The source of the explosion.</param>
/// <param name="target">The target of the attack.</param>
/// <param name="damageAmount">The amount of explosive damage.</param>
/// <param name="position">The position of the explosion.</param>
/// <param name="damageRadius">The radius of the explosion.</param>
/// <param name="damageOwner">If true, it will hit the source.</param>
void CollisionManager::Explode(GameplayObject* source, GameplayObject* target, float damageAmount, const Vector2 &position, float damageRadius, bool damageOwner)
{
    if (damageRadius <= 0.0f)
    {
        return;
    }

    for(auto &object : m_collection)
    {
        // don't bother if it's already dead
        if (!object->Active() || object->Life <= 0.0f)
        {
            continue;
        }

        // don't hurt the GameplayObject that the projectile hit, it's hurt
        if (object.get() == target)
        {
            continue;
        }

        // don't hit the owner if the damageOwner flag is off
        if ((object.get() == source) && !damageOwner)
        {
            continue;
        }

        // measure the distance to the GameplayObject and see if it's in range
        float damageRadiusSquared = damageRadius * damageRadius;
        Vector2 direction = object->Position - position;
        float distanceSquared = direction.LengthSquared();
        if (distanceSquared <= damageRadiusSquared)
        {
            float distance = std::sqrt(distanceSquared);

            // adjust the amount of damage based on the distance
            float adjustedDamage = damageAmount * (damageRadius - distance) / damageRadius;

            // if we're still damaging the GameplayObject, then apply it
            if (adjustedDamage > 0.0f)
            {
                object->TakeDamage(source, adjustedDamage);

                // move those affected by the blast
                if (object.get() != source)
                {
                    direction.Normalize();
                    Vector2 adjustedVelocity = direction * adjustedDamage * speedDamageRatio;
                    object->Velocity += adjustedVelocity;
                }
            }
        }
    }
}


Vector2 CollisionManager::MoveAndCollide(GameplayObject* gameplayObject, const Vector2 &movement)
{
    // make sure we care about where this gameplay object goes
    if (!gameplayObject->Active())
        return movement;

    // make sure the movement is significant
    float lengthSquared = movement.LengthSquared();
    if (lengthSquared <= 0.0f)
        return movement;

    // generate the list of collisions
    Collide(gameplayObject, movement);

    // determine if we had any collisions
    if (m_collisionResults.size() > 0)
    {
        std::sort(m_collisionResults.begin(), m_collisionResults.end());
        for(auto &collision : m_collisionResults)
        {
            // let the two objects touch each other, and see what happens
            if (gameplayObject->OnTouch(collision.GameplayObject) && collision.GameplayObject->OnTouch(gameplayObject))
            {
                gameplayObject->CollidedThisFrame = collision.GameplayObject->CollidedThisFrame = true;
                // they should react to the other, even if they just died
                AdjustVelocities(gameplayObject, collision.GameplayObject);
                return Vector2::Zero;
            }
        }
    }

    return movement;
}

void CollisionManager::AdjustVelocities(GameplayObject* actor1, GameplayObject* actor2)
{
    // don't adjust velocities if at least one has negative mass
    if (actor1->Mass <= 0.0f || actor2->Mass <= 0.0f)
    {
        return;
    }

    // determine the vectors normal and tangent to the collision
    Vector2 collisionNormal = actor2->Position - actor1->Position;
    float lengthsq = collisionNormal.LengthSquared();
    if (lengthsq > 0.0f)
    {
        collisionNormal /= sqrt(lengthsq);
    }
    else
    {
        return;
    }

    Vector2 collisionTangent = Vector2(-collisionNormal.y, collisionNormal.x);

    // determine the velocity components along the normal and tangent vectors
    float velocityNormal1 = actor1->Velocity.Dot(collisionNormal);
    float velocityTangent1 = actor1->Velocity.Dot(collisionTangent);
    float velocityNormal2 = actor2->Velocity.Dot(collisionNormal);
    float velocityTangent2 = actor2->Velocity.Dot(collisionTangent);

    // determine the new velocities along the normal
    float massDelta = actor1->Mass - actor2->Mass;
    float massSum = actor1->Mass + actor2->Mass;
    float velocityNormal1New = ((velocityNormal1 * massDelta) + (2.0f * actor2->Mass * velocityNormal2)) / massSum;
    float velocityNormal2New = ((velocityNormal2 * -massDelta) + (2.0f * actor1->Mass * velocityNormal1)) / massSum;

    // determine the new total velocities
    actor1->Velocity = (velocityNormal1New * collisionNormal) + (velocityTangent1 * collisionTangent);
    actor2->Velocity = (velocityNormal2New * collisionNormal) + (velocityTangent2 * collisionTangent);
}
