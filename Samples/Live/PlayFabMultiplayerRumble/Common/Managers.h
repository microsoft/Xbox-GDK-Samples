//--------------------------------------------------------------------------------------
// Managers.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "IManager.h"

#include "AsyncTaskManager.h"
#include "AudioManager.h"
#include "CollisionManager.h"
#include "ContentManager.h"
#include "GameStateManager.h"
#include "InputManager.h"
#include "IOnlineManager.h"
#include "ParticleManager.h"
#include "RenderManager.h"
#include "ScreenManager.h"
#include "XboxUserManager.h"
#include "PlayFabPartyManager.h"
#include "OnlineManager.h"
#include "MPAManager.h"
#include "FriendsManager.h"

namespace PlayFabMultiplayerRumble
{
    class IManager;

    class Managers
    {
    public:
        static void Initialize();
        static void Shutdown()
        {
            m_managersByType.clear();
        }

        template<class T>
        static T* Get()
        {
            return static_cast<T*>(m_managersByType[typeid(T).hash_code()].get());
        }

    private:

        template<typename ManagerType>
        static void AddManager()
        {
            static_assert(std::is_base_of_v<IManager, ManagerType>, "Manager must be derived from Manager base class");
            AddManager<ManagerType, ManagerType>();
        }

        template<typename InterfaceType, typename ManagerType>
        static void AddManager()
        {
            static_assert(std::is_base_of_v<InterfaceType, ManagerType>, "Manager must be derived from Interface");
            static_assert(std::is_base_of_v<IManager, ManagerType>, "Manager must be derived from Manager base class");

            IManager *mgr = new ManagerType();
            m_managersByType.emplace(typeid(InterfaceType).hash_code(), mgr);
        }

        static std::unordered_map<size_t, std::unique_ptr<IManager>> m_managersByType;
    };

}

