//--------------------------------------------------------------------------------------
// Managers.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Manager.h"

#include "AsyncTaskManager.h"
#include "AudioManager.h"
#include "CollisionManager.h"
#include "ContentManager.h"
#include "GameStateManager.h"
#include "InputManager.h"
#include "OnlineManager.h"
#include "ParticleManager.h"
#include "PlayFabPartyManager.h"
#include "RenderManager.h"
#include "ScreenManager.h"
#include "XboxLiveOnlineManager.h"
#include "XboxUserManager.h"
#include "SpeechManager.h"

namespace NetRumble
{
    class Manager;

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
            static_assert(std::is_base_of_v<Manager, ManagerType>, "Manager must be derived from Manager base class");
            AddManager<ManagerType, ManagerType>();
        }

        template<typename InterfaceType, typename ManagerType>
        static void AddManager()
        {
            static_assert(std::is_base_of_v<InterfaceType, ManagerType>, "Manager must be derived from Interface");
            static_assert(std::is_base_of_v<Manager, ManagerType>, "Manager must be derived from Manager base class");

            Manager *mgr = new ManagerType();
            m_managersByType.emplace(typeid(InterfaceType).hash_code(), mgr);
        }

        static std::unordered_map<size_t, std::unique_ptr<Manager>> m_managersByType;
    };

}

