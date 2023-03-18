//--------------------------------------------------------------------------------------
// File: PlayFabResources.h
//
// Handles Login to PlayFab services using Xbox authentication
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------
#pragma once

#include <XUser.h>
#include <playfab/core/PFErrors.h>
#include <playfab/services/PFServices.h>

namespace ATG
{
    class PlayFabResources
    {
    public:
        PlayFabResources(_In_z_ const char* titleId, XUserHandle xuser);
        ~PlayFabResources();

        PlayFabResources(PlayFabResources&&) = default;
        PlayFabResources& operator= (PlayFabResources&&) = default;

        PlayFabResources(PlayFabResources const&) = delete;
        PlayFabResources& operator= (PlayFabResources const&) = delete;

        void Initialize(const char* titleId, XUserHandle xuser);
        void Cleanup();
        void RequestXUserToken();
        void LoginToPlayFab();
        PFEntityHandle GetEntityHandle();
        PFEntityKey const* GetPlayerEntityKey();
        const char* GetXboxToken();
    };
}
