//--------------------------------------------------------------------------------------
// UserManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "InGameChat_Desktop.h"
#include "UserManager.h"

using Microsoft::WRL::ComPtr;

UserManager::UserManager() noexcept :
    m_audioEndpointChangedToken{},
    m_deviceChangedToken{},
    m_initialized(false)
{
}

UserManager::~UserManager()
{
}

HRESULT UserManager::Initialize()
{
    if (m_initialized)
    {
        return E_FAIL;
    }

    // Register for notifications of audio endpoint changes
    auto hr = XUserRegisterForDefaultAudioEndpointUtf16Changed(
        nullptr,
        this,
        [](void* context, XUserLocalId user, XUserDefaultAudioEndpointKind defaultAudioEndpointKind, const wchar_t* endpointIdUtf16)
        {
            static_cast<UserManager*>(context)->OnAudioEndpointChangeEvent(user, defaultAudioEndpointKind, endpointIdUtf16);
        },
        &m_audioEndpointChangedToken
        );

    if (FAILED(hr))
    {
        DebugTrace("Unable to register for audio device events!");
        return hr;
    }

    // Register for notifications of device/user assocation changes
    hr = XUserRegisterForDeviceAssociationChanged(
        nullptr,
        this,
        [](void* context, const XUserDeviceAssociationChange* change)
        {
            static_cast<UserManager*>(context)->OnDeviceAssocationChangeEvent(change);
        },
        &m_deviceChangedToken
        );

    if (FAILED(hr))
    {
        DebugTrace("Unable to register for device association events!");
        return hr;
    }

    // Attempt to get the default user, i.e. the user who launched the game
    auto async = new XAsyncBlock{};

    async->context = this;
    async->callback = [](XAsyncBlock *async)
    {
        XUserHandle user = nullptr;

        // Get the result just to complete the operation.  We can't trust this XUserHandle
        // instead we wait for the device pairing event and then initialize the user.
        auto hr = XUserAddResult(
            async,
            &user
            );

        if (FAILED(hr))
        {
            DebugTrace("No default user found");
        }

        delete async;
    };

    hr = XUserAddAsync(
        XUserAddOptions::AddDefaultUserSilently,
        async
        );

    if (FAILED(hr))
    {
        DebugTrace("Unable to add user!");

        delete async;
        return hr;
    }

    m_initialized = true;

    return S_OK;
}

std::shared_ptr<UserContext> UserManager::GetUserByHandle(XUserHandle user)
{
    std::lock_guard<std::mutex> lock(m_usersLock);

    for (const auto& item : m_mapDeviceToUser)
    {
        if (XUserCompare(user, item.second->UserHandle) == 0)
        {
            return item.second;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<UserContext>> UserManager::GetUsers()
{
    std::lock_guard<std::mutex> lock(m_usersLock);
    std::vector<std::shared_ptr<UserContext>> users;

    for (const auto& item : m_mapDeviceToUser)
    {
        users.push_back(item.second);
    }

    return users;
}

std::shared_ptr<UserContext> UserManager::GetUserByLocalId(XUserLocalId user)
{
    std::lock_guard<std::mutex> lock(m_usersLock);

    for (const auto& item : m_mapDeviceToUser)
    {
        if (item.second->LocalUserId == user)
        {
            return item.second;
        }
    }

    return nullptr;
}

void UserManager::OnAudioEndpointChangeEvent(XUserLocalId user, XUserDefaultAudioEndpointKind defaultAudioEndpointKind, const wchar_t * endpointIdUtf16)
{
    // A microphone was dis/connected
    if (defaultAudioEndpointKind == XUserDefaultAudioEndpointKind::CommunicationCapture)
    {
        auto context = GetUserByLocalId(user);

        if (context != nullptr)
        {
            context->HasMic = (endpointIdUtf16 != nullptr);
        }
    }
}

void UserManager::OnDeviceAssocationChangeEvent(const XUserDeviceAssociationChange *change)
{
    if (change->oldUser == NULL_USER_LOCAL_ID)
    {
        // New association
        XUserHandle handle = nullptr;

        auto hr = XUserFindUserByLocalId(
            change->newUser,
            &handle
            );

        if (FAILED(hr))
        {
            DebugTrace("Unable to get user from local id!");
            return;
        }

        auto user = std::make_shared<UserContext>();

        user->LocalUserId = change->newUser;
        user->UserHandle = handle;

        // Create a Live Context handle
        hr = XblContextCreateHandle(
            user->UserHandle,
            &user->ContextHandle
            );

        if (FAILED(hr))
        {
            DebugTrace("Failed to create Xbl Context!");
        }

        {
            std::lock_guard<std::mutex> lock(m_usersLock);
            m_mapDeviceToUser[change->deviceId] = user;
        }

        DebugTrace("Assigned device to user %lu", user->UserHandle);

        auto xblm = Sample::Instance()->GetLiveManager();
        if (xblm->IsInitialized())
        {
            xblm->AddLocalUser(user->UserHandle);
        }
    }
    else if (change->newUser == NULL_USER_LOCAL_ID)
    {
        std::lock_guard<std::mutex> lock(m_usersLock);
        // Removing association
        auto it = m_mapDeviceToUser.find(change->deviceId);
        if (it != m_mapDeviceToUser.end())
        {
            m_mapDeviceToUser.erase(it);
        }
    }
}
