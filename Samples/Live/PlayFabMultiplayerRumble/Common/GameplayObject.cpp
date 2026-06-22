//--------------------------------------------------------------------------------------
// GameplayObject.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameplayObject.h"

#include "Managers.h"
#include "Texture.h"

#include "RenderContext.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

std::atomic_uint32_t GameplayObject::nextUniqueID = 1;

GameplayObject::GameplayObject() :
    m_uniqueID(nextUniqueID++)
{
}

void GameplayObject::Initialize()
{
    if (!m_active)
    {
        m_active = true;
        Managers::Get<CollisionManager>()->Collection().push_back(shared_from_this());
    }
}

void GameplayObject::Update(float /*elapsedTime*/)
{
    CollidedThisFrame = false;
}

void GameplayObject::Draw(float /*elapsedTime*/, RenderContext* renderContext, const TextureHandle &texture, XMVECTOR color)
{
    renderContext->Draw(
        texture,
        Position,
        Rotation,
        2.0f * Radius / static_cast<float>(std::min<int>(texture.Texture->Width(), texture.Texture->Height())),
        color,
        TexturePosition::Centered);
}

/// <summary>
/// Defines the interaction between this GameplayObject and 
/// a target GameplayObject when they touch.
/// </summary>
/// <param name="target">The GameplayObject that is touching this one.</param>
/// <returns>True if the objects meaningfully interacted.</returns>
bool GameplayObject::OnTouch(GameplayObject*)
{
    return true;
}

/// <summary>
/// Damage this object by the amount provided.
/// </summary>
/// <remarks>
/// This function is provided in lieu of a Life mutation property to allow 
/// classes of objects to restrict which kinds of objects may damage them,
/// and under what circumstances they may be damaged.
/// </remarks>
/// <param name="source">The GameplayObject responsible for the damage.</param>
/// <param name="damageAmount">The amount of damage.</param>
/// <returns>If true, this object was damaged.</returns>
bool GameplayObject::TakeDamage(GameplayObject*, float)
{
    return false;
}

/// <summary>
/// Kills this object, in response to the given GameplayObject.
/// </summary>
/// <param name="source">The GameplayObject responsible for the kill.</param>
/// <param name="cleanupOnly">If true, the object dies without any further effects.</param>
void GameplayObject::Die(GameplayObject*, bool)
{
    if (m_active)
    {
        m_active = false;
        Managers::Get<CollisionManager>()->Collection().QueuePendingRemoval(shared_from_this());
    }
}