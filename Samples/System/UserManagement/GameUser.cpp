//--------------------------------------------------------------------------------------
// GameUser.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "GameUser.h"

using namespace ATG;

GameUser::GameUser(XUserHandle userHandle)
    : m_userHandle(userHandle)
{
    assert(m_userHandle);

    DX::ThrowIfFailed(XUserGetLocalId(m_userHandle, &m_localId));
}

GameUser::~GameUser()
{
    XUserCloseHandle(m_userHandle);
}

XUserHandle GameUser::GetUserHandle() const
{
    return m_userHandle;
}

XUserLocalId GameUser::GetLocalId() const
{
    return m_localId;
}

void GameUser::AssociateDevice(const APP_LOCAL_DEVICE_ID& deviceId)
{
    if (!IsAssociatedWith(deviceId))
    {
        m_associatedDevices.push_back(deviceId);
    }
}

void GameUser::DisassociateDevice(const APP_LOCAL_DEVICE_ID& deviceId)
{
    for (unsigned int index = 0; index < m_associatedDevices.size(); ++index)
    {
        if (m_associatedDevices[index] == deviceId)
        {
            m_associatedDevices.erase(m_associatedDevices.begin() + index);
            return;
        }
    }
}

bool GameUser::IsAssociatedWith(const APP_LOCAL_DEVICE_ID& deviceId) const
{
    for (auto& existingDeviceId : m_associatedDevices)
    {
        if (existingDeviceId == deviceId)
        {
            return true;
        }
    }

    return false;
}

const std::vector<APP_LOCAL_DEVICE_ID>& GameUser::GetAssociatedDevices() const
{
    return m_associatedDevices;
}

GameUserManager::GameUserManager()
    : m_nextCallbackHandle(0)
    , m_taskQueue(nullptr)
    , m_userAddInProgress(false)
    , m_nextGamerPicAsyncHandle(0)
{
    // Create a task queue that will process in the background on system threads and fire callbacks on a thread we choose in a serialized order
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_taskQueue)
    );

    // Register for any change events for user.
    // This allows us to know when users sign out, changes to gamertags, and more.
    XUserRegisterForChangeEvent(
        m_taskQueue,
        this,
        &UserChangeEventCallback,
        &m_userChangeEventCallbackToken
    );

    // Registers for any change to device association so that the application can keep
    // up-to-date information about users and their associated devices.
    XUserRegisterForDeviceAssociationChanged(
        m_taskQueue,
        this,
        &UserDeviceAssociationChangedCallback,
        &m_userDeviceAssociationChangedCallbackToken
    );
}

GameUserManager::~GameUserManager()
{
    // Be sure to pass false so that the unregistration does not block for pending callbacks to fire.
    // There shouldn't be any callbacks anyways, but we know there's won't be another dispatch of callbacks
    // at this point.
    XUserUnregisterForDeviceAssociationChanged(m_userDeviceAssociationChangedCallbackToken, false);
    XUserUnregisterForChangeEvent(m_userChangeEventCallbackToken, false);

    if (m_taskQueue)
    {
        XTaskQueueCloseHandle(m_taskQueue);
    }
}

void GameUserManager::Update()
{
    // Handle callbacks in the main thread to ensure thread safety
    while (XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 0))
    {
    }
}

void GameUserManager::OnSuspend()
{
    // There's nothing to do for basic user management upon suspending. If game saves were used, then saving would need
    // to happen for each user. It may also be necessary to abort any pending async blocks if those don't gracefully handle
    // suspend failures themselves.
}

void GameUserManager::OnResume()
{
    // Upon resuming, callbacks will be fired to the application based on what happened while suspended.
}

size_t GameUserManager::NumUsers() const
{
    return m_users.size();
}

std::vector<XUserHandle> GameUserManager::GetUsers() const
{
    std::vector<XUserHandle> users;
    users.reserve(m_users.size());

    for (auto& user : m_users)
    {
        users.push_back(user->GetUserHandle());
    }

    return users;
}

const std::vector<APP_LOCAL_DEVICE_ID>& GameUserManager::GetUserDevices(XUserHandle userHandle) const
{
    // Search via local id to ensure always getting the correct user
    XUserLocalId localId;
    DX::ThrowIfFailed(XUserGetLocalId(userHandle, &localId));
    for (auto& user : m_users)
    {
        if (user->GetLocalId().value == localId.value)
        {
            return user->GetAssociatedDevices();
        }
    }

    static std::vector<APP_LOCAL_DEVICE_ID> emptyDevices;
    return emptyDevices;
}

bool GameUserManager::SignInDefaultUser(std::function<SignInUserResultCallback> resultCallback)
{
    return TryStartSignInAsync(resultCallback, true, false);
}

bool GameUserManager::SignInUser(std::function<SignInUserResultCallback> resultCallback, bool allowGuests)
{
    return TryStartSignInAsync(resultCallback, false, allowGuests);
}

bool GameUserManager::IsSignInUserInProgress() const
{
    return m_userAddInProgress;
}

bool GameUserManager::SignOutUser(XUserHandle userHandle)
{
    for(unsigned int index = 0; index < m_users.size(); ++index)
    {
        // Only test for handle equivalence. Different handles can refer to the same user,
        // but if an external party wants to sign-out a specific handle, then sign-out only
        // that handle
        if (m_users[index]->GetUserHandle() == userHandle)
        {
            // Log the sign-out
            {
                char logBuffer[512] = {};
                sprintf_s(logBuffer, 512, u8"Signing out user LocalId: %llu\n", m_users[index]->GetLocalId().value);
                OutputDebugStringA(logBuffer);
            }

            m_users.erase(m_users.begin() + index);
            return true;
        }
    }

    return false;
}

void GameUserManager::SignOutAllUsers()
{
    m_users.clear();
}

GameUserManager::CallbackHandle GameUserManager::RegisterUserEventCallback(std::function<UserEventCallback> callback, void* context)
{
    assert(callback);

    CallbackHandle newHandle = m_nextCallbackHandle++;
    m_userEventCallbacks.emplace(newHandle, UserEventCallbackData{callback, context});
    return newHandle;
}

void GameUserManager::UnregisterUserEventCallback(CallbackHandle handle)
{
    m_userEventCallbacks.erase(handle);
}

bool GameUserManager::GetGamerPicAsync(XUserHandle userHandle, XUserGamerPictureSize pictureSize, std::function<GamerPicResultCallback> completionCallback, AsyncHandle* outHandle)
{
    assert(userHandle);
    assert(outHandle);

    // Context information
    struct AsyncContext
    {
        GameUserManager*                        m_this;
        AsyncHandle                             m_asyncHandle;
        XUserHandle                             m_userHandle;
        std::function<GamerPicResultCallback>   m_completionCallback;
    };
    AsyncHandle asyncHandle = m_nextGamerPicAsyncHandle++;
    AsyncContext* context = new AsyncContext{this, asyncHandle, userHandle, completionCallback};

    // Setup async
    auto asyncBlock = std::make_unique<XAsyncBlock>();
    ZeroMemory(asyncBlock.get(), sizeof(XAsyncBlock));
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = context;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        AsyncContext* context = static_cast<AsyncContext*>(asyncBlock->context);

        // Check for an abort
        // Functions below could return E_ABORT, or XAsyncGetStatus() could be checked, but we are just tracking
        // it ourselves. Aborted async operations still fire their callbacks.
        auto abortIter = context->m_this->m_abortedGamerPicAsyncBlocks.find(context->m_asyncHandle);
        if (abortIter != context->m_this->m_abortedGamerPicAsyncBlocks.end())
        {
            context->m_this->m_abortedGamerPicAsyncBlocks.erase(abortIter);
            delete context;
            return;
        }

        // Ensure it's on the pending map now
        auto pendingIter = context->m_this->m_pendingGamerPicAsyncBlocks.find(context->m_asyncHandle);
        assert(pendingIter != context->m_this->m_pendingGamerPicAsyncBlocks.end());

        // Get buffer size
        size_t bufferSize = 0;
        HRESULT hr = XUserGetGamerPictureResultSize(asyncBlock, &bufferSize);
        if (FAILED(hr))
        {
            context->m_this->m_pendingGamerPicAsyncBlocks.erase(pendingIter);
            context->m_completionCallback(hr, context->m_userHandle, {});
            delete context;
            return;
        }

        // Get buffer data
        std::vector<uint8_t> buffer;
        buffer.resize(bufferSize);
        size_t bufferUsed = 0;
        hr = XUserGetGamerPictureResult(asyncBlock, bufferSize, buffer.data(), &bufferUsed);

        // Return hr and data in both success and fail cases
        context->m_this->m_pendingGamerPicAsyncBlocks.erase(pendingIter);
        context->m_completionCallback(hr, context->m_userHandle, buffer);
        delete context;
    };

    // Store callback information
    m_pendingGamerPicAsyncBlocks.insert({asyncHandle, std::move(asyncBlock)});

    // Start the request
    if (FAILED(XUserGetGamerPictureAsync(userHandle, pictureSize, m_pendingGamerPicAsyncBlocks[asyncHandle].get())))
    {
        delete context;
        return false;
    }

    // Log the start of the request
    {
        XUserLocalId localId;
        DX::ThrowIfFailed(XUserGetLocalId(userHandle, &localId));
        char logBuffer[512] = {};
        sprintf_s(logBuffer, 512, u8"Requesting gamerpic for user %llu. AsyncHandle = %llu\n", localId.value, asyncHandle);
        OutputDebugStringA(logBuffer);
    }

    (*outHandle) = asyncHandle;
    return true;
}

void GameUserManager::CancelGamerPicRequest(AsyncHandle asyncHandle)
{
    // Don't do anything if not in progress
    if (m_pendingGamerPicAsyncBlocks.find(asyncHandle) == m_pendingGamerPicAsyncBlocks.end())
    {
        return;
    }

    // Log canceling of a request
    {
        char logBuffer[512] = {};
        sprintf_s(logBuffer, 512, u8"Aborting gamerpic request for async handle %llu\n", asyncHandle);
        OutputDebugStringA(logBuffer);
    }

    // If canceling, the user doesn't want to be notified of the result, so move to the aborted list so the callback can know to ignore user callback
    std::unique_ptr<XAsyncBlock> asyncBlock = std::move(m_pendingGamerPicAsyncBlocks[asyncHandle]);
    m_pendingGamerPicAsyncBlocks.erase(asyncHandle);
    m_abortedGamerPicAsyncBlocks.insert({asyncHandle, std::move(asyncBlock)});

    // Abort request
    XAsyncCancel(m_abortedGamerPicAsyncBlocks[asyncHandle].get());
}

bool GameUserManager::TryStartSignInAsync(std::function<SignInUserResultCallback> resultCallback, bool autoDefaultUser, bool allowGusts)
{
    if (m_userAddInProgress)
    {
        return false;
    }

    if (autoDefaultUser && m_users.size() > 0)
    {
        return false;
    }

    // Store callback
    assert(resultCallback);
    m_signInUserResultCallback = resultCallback;

    // Setup async block and function
    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        GameUserManager* pThis = static_cast<GameUserManager*>(asyncBlock->context);

        XUserHandle newUser = nullptr;
        HRESULT result = XUserAddResult(asyncBlock, &newUser);

        // Log callback
        {
            XUserLocalId localId = {};
            if (SUCCEEDED(result))
            {
                XUserGetLocalId(newUser, &localId);
            }

            char logBuffer[512] = {};
            sprintf_s(logBuffer, 512, u8"Sign-in Callback : UserLocalId: %llx, Result = 0x%08x\n", localId.value, result);
            OutputDebugStringA(logBuffer);
        }

        if (SUCCEEDED(result))
        {
            newUser = pThis->AddUser(newUser);
        }

        // Clear flag in-case the callback wants to start another login immediately
        pThis->m_userAddInProgress = false;

        // Inform 3rd-party caller of the result regardless of success/fail
        pThis->m_signInUserResultCallback(result, newUser);

        delete asyncBlock;
    };

    // Try to perform default user sign-in
    XUserAddOptions userAddOptions = XUserAddOptions::None;
    if (autoDefaultUser)
    {
        userAddOptions = XUserAddOptions::AddDefaultUserSilently;
    }
    else if (allowGusts)
    {
        userAddOptions = XUserAddOptions::AllowGuests;
    }
    if (SUCCEEDED(XUserAddAsync(userAddOptions, asyncBlock)))
    {
        // Async action started
        m_userAddInProgress = true;
        return true;
    }
    else
    {
        // Failed, so be sure to clean async block
        delete asyncBlock;
    }

    return false;
}

XUserHandle GameUserManager::AddUser(XUserHandle potentialNewUser)
{
    for (auto& existingUser : m_users)
    {
        if (existingUser->GetUserHandle() == potentialNewUser)
        {
            // Exact handle match. Return without doing anything. User is a duplicate of someone already tracked
            return potentialNewUser;
        }
        else if (XUserCompare(existingUser->GetUserHandle(), potentialNewUser) == 0)
        {
            // New user is a new handle to an already-tracked used. Close the new handle and return the old one
            XUserCloseHandle(potentialNewUser);
            return existingUser->GetUserHandle();
        }
    }

    // If through list without finding user and handling, then this is actually a new user
    std::unique_ptr<GameUser> newGameUser = std::make_unique<GameUser>(potentialNewUser);
    m_users.push_back(std::move(newGameUser));
    return potentialNewUser;
}

void GameUserManager::HandleUserSignOutEvent(XUserLocalId userLocalId)
{
    for (unsigned int index = 0; index < m_users.size(); ++index)
    {
        if (m_users[index]->GetLocalId().value == userLocalId.value)
        {
            // Log the sign-out
            {
                char logBuffer[512] = {};
                sprintf_s(logBuffer, 512, u8"Signed out user LocalId: %llu\n", m_users[index]->GetLocalId().value);
                OutputDebugStringA(logBuffer);
            }

            m_users.erase(m_users.begin() + index);
            return;
        }
    }
}

void GameUserManager::HandleUserDeviceAssociationChangedEvent(APP_LOCAL_DEVICE_ID deviceId, XUserLocalId oldUser, XUserLocalId newUser)
{
    GameUser* oldGameUser = FindGameUser(oldUser);
    if (oldGameUser)
    {
        oldGameUser->DisassociateDevice(deviceId);
    }

    GameUser* newGameUser = FindGameUser(newUser);
    if (newGameUser)
    {
        char logBuffer[512] = {};
        sprintf_s(logBuffer, 512, u8" ->Associating device to user %llu\n", newUser.value);
        OutputDebugStringA(logBuffer);

        newGameUser->AssociateDevice(deviceId);
    }
    else if(newUser.value != 0)
    {
        // TODO: chcoope - Bug opened by feature team to fix this out-of-order issue.
        //                 https://microsoft.visualstudio.com/Xbox/_workitems/edit/23379915

        char logBuffer[512] = {};
        sprintf_s(logBuffer, 512, u8" ->Failed to associate device to user %llu\n", newUser.value);
        OutputDebugStringA(logBuffer);
    }
}

GameUser* GameUserManager::FindGameUser(XUserLocalId localId) const
{
    for (auto& gameUser : m_users)
    {
        if (gameUser->GetLocalId().value == localId.value)
        {
            return gameUser.get();
        }
    }

    return nullptr;
}

void CALLBACK GameUserManager::UserChangeEventCallback(
    _In_opt_ void* context,
    _In_ XUserLocalId userLocalId,
    _In_ XUserChangeEvent event
)
{
    // Log the callback
    {
        char debugString[512] = {};
        sprintf_s(debugString, 512, u8"UserChangeEventCallback() : userLocalId = 0x%llx, event = %d\n",
            userLocalId.value,
            event
        );
        OutputDebugStringA(debugString);
    }

    GameUserManager* pThis = static_cast<GameUserManager*>(context);

    // Find out the associated user handle
    XUserHandle userHandle = nullptr;
    for (auto& trackedUser : pThis->m_users)
    {
        if (trackedUser->GetLocalId().value == userLocalId.value)
        {
            userHandle = trackedUser->GetUserHandle();
            break;
        }
    }

    // Handle sign-outs
    if (event == XUserChangeEvent::SignedOut)
    {
        pThis->HandleUserSignOutEvent(userLocalId);
    }

    // Send callbacks after handling internally so that user code always can query up-to-date state.
    // If it was a logout, then the user might call SignOutUser again which is fine.
    if (userHandle)
    {
        // Iterate a copy in-case the user callback tries to modify the list
        std::map<CallbackHandle, UserEventCallbackData> callbacksCopy = pThis->m_userEventCallbacks;
        for (auto& callbackData : callbacksCopy)
        {
            callbackData.second.m_callback(callbackData.second.m_context, userHandle, event);
        }
    }
}

void CALLBACK GameUserManager::UserDeviceAssociationChangedCallback(
    _In_opt_ void* context,
    _In_ const XUserDeviceAssociationChange* change
)
{
    assert(change);

    // Log the callback
    {
        char debugString[512] = {};
        sprintf_s(debugString, 512, u8"UserDeviceAssociationChangedCallback() : deviceId = %08x-%08x-%08x-%08x-%08x-%08x-%08x-%08x, oldUser = 0x%llx, newUser = 0x%llx\n",
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[0]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[4]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[8]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[12]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[16]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[20]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[24]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[28]),
            change->oldUser.value,
            change->newUser.value
        );
        OutputDebugStringA(debugString);
    }

    GameUserManager* pThis = static_cast<GameUserManager*>(context);

    pThis->HandleUserDeviceAssociationChangedEvent(change->deviceId, change->oldUser, change->newUser);
}
