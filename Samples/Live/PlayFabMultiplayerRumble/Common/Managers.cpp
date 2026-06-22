//--------------------------------------------------------------------------------------
// Managers.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "IManager.h"
#include "Managers.h"

#include <vcruntime_typeinfo.h>

using namespace PlayFabMultiplayerRumble;

std::unordered_map<size_t, std::unique_ptr<IManager>> Managers::m_managersByType;

void Managers::Initialize()
{
    AddManager<AsyncTaskManager>();
    AddManager<AudioManager>();
    AddManager<RenderManager>();
    AddManager<InputManager>();
    AddManager<XboxUserManager>();

    AddManager<OnlineManager>();

    AddManager<ContentManager>();
    AddManager<CollisionManager>();
    AddManager<ParticleEffectManager>();
    AddManager<ScreenManager>();
    AddManager<GameStateManager>();
    AddManager<PlayFabPartyManager>();
    AddManager<MPAManager>();
    AddManager<FriendsManager>();
}
