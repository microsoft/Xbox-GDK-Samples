//--------------------------------------------------------------------------------------
// GameUser.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

    // Represents a single user who is currently signed into the game
    class GameUser
    {
    public:

        GameUser(XUserHandle userHandle);
        ~GameUser();

        XUserHandle GetUserHandle() const;
        XUserLocalId GetLocalId() const;

        void AssociateDevice(const APP_LOCAL_DEVICE_ID& deviceId);
        void DisassociateDevice(const APP_LOCAL_DEVICE_ID& deviceId);
        bool IsAssociatedWith(const APP_LOCAL_DEVICE_ID& deviceId) const;
        const std::vector<APP_LOCAL_DEVICE_ID>& GetAssociatedDevices() const;

    protected:

        // User Data
        XUserHandle                         m_userHandle;
        XUserLocalId                        m_localId;

        // Game Input Data
        std::vector<APP_LOCAL_DEVICE_ID>    m_associatedDevices;
    };

    // Manages all signed-in users
    class GameUserManager
    {
    public:

        // Callback types used by this manager and users of this manager
        using CallbackHandle = uint64_t;
        using AsyncHandle = uint64_t;
        using SignInUserResultCallback = void(HRESULT result, XUserHandle newUser);
        using UserEventCallback = void(void* context, XUserHandle userHandle, XUserChangeEvent event);
        using GamerPicResultCallback = void(HRESULT result, XUserHandle userHandle, std::vector<uint8_t> gamerpicBuffer);

    protected:

        struct UserEventCallbackData
        {
            std::function<UserEventCallback>    m_callback;
            void*                               m_context;
        };

    public:

        GameUserManager();
        ~GameUserManager();

        void Update();

        void OnSuspend();
        void OnResume();

        size_t NumUsers() const;
        std::vector<XUserHandle> GetUsers() const;
        const std::vector<APP_LOCAL_DEVICE_ID>& GetUserDevices(XUserHandle userHandle) const;

        // Attempts to sign in the default user. If true, then an async action was started and the passed-in
        // callback will be called with the result. If false, then the default user sign-in wasn't started
        // due to some reason or another (such as users already being signed-in to the app).
        // The resultCallback will return the results of XUserAddResult() and is required.
        bool SignInDefaultUser(std::function<SignInUserResultCallback> resultCallback);

        // Attempts to sign in a user. Multiple users can be signed-in to an app and it's up to the app to
        // manage how the game functions with multiple users. If a default user wasn't able to be signed in
        // with SignInDefaultUser(), then this function needs to be users to allow the system to provide a user
        // sign-in dialog. Returns true if the async action was started.
        bool SignInUser(std::function<SignInUserResultCallback> resultCallback, bool allowGuests);

        bool IsSignInUserInProgress() const;

        // Signs out a user such that it is no longer tracked by this manager and has to be re-added via on of
        // the sign-in functions. Returns true if the user was found and removed. Returns false if the user
        // wasn't found.
        bool SignOutUser(XUserHandle userHandle);

        // Signs out all users
        void SignOutAllUsers();

        // Registers a callback with the manager to notify upon user events such as gamertag change, gamerpic change,
        // and signouts.
        CallbackHandle RegisterUserEventCallback(std::function<UserEventCallback> callback, void* context);

        // Unregisters a callback
        void UnregisterUserEventCallback(CallbackHandle handle);

        // Asynchronous actions to download a gamerpic for a user is handled here. Individual screens need only call this function to start the download.
        // A callback is fired when the action finishes, whether succeed or fail. Use the returned handle to cancel a request in-progress. If canceling,
        // the callback will not be returned so that individual screens can have simpler error handling.
        // Returns true if the async action was started properly.
        bool GetGamerPicAsync(XUserHandle userHandle, XUserGamerPictureSize pictureSize, std::function<GamerPicResultCallback> completionCallback, AsyncHandle* outHandle);
        void CancelGamerPicRequest(AsyncHandle asyncHandle);

    protected:

        // Sign-in function to reduce duplication
        bool TryStartSignInAsync(std::function<SignInUserResultCallback> resultCallback, bool autoDefaultUser, bool allowGusts);

        // Internal function to add a user that was returned by one of the sign-in functions above.
        // If the user is a duplicate of a user already added with a different handle, the new handle
        // will be closed and the old user will be returned
        XUserHandle AddUser(XUserHandle potentialNewUser);

        // Handle external events for a user signing-out. These are generated from outside the application in some way.
        void HandleUserSignOutEvent(XUserLocalId userLocalId);

        // Handle external events for user device associations changing
        void HandleUserDeviceAssociationChangedEvent(APP_LOCAL_DEVICE_ID deviceId, XUserLocalId oldUser, XUserLocalId newUser);

        // Helper function to find an already-tracked gamed user. Not intended for external callers.
        GameUser* FindGameUser(XUserLocalId localId) const;

    private:

        static void CALLBACK UserChangeEventCallback(
            _In_opt_ void* context,
            _In_ XUserLocalId userLocalId,
            _In_ XUserChangeEvent event
        );

        static void CALLBACK UserDeviceAssociationChangedCallback(
            _In_opt_ void* context,
            _In_ const XUserDeviceAssociationChange* change
        );

    protected:

        // Users
        std::vector<std::unique_ptr<GameUser>>              m_users;

        // Callbacks
        XTaskQueueRegistrationToken                         m_userChangeEventCallbackToken;
        XTaskQueueRegistrationToken                         m_userDeviceAssociationChangedCallbackToken;
        CallbackHandle                                      m_nextCallbackHandle;
        std::map<CallbackHandle, UserEventCallbackData>     m_userEventCallbacks;

        // Async data
        XTaskQueueHandle                                    m_taskQueue;
        bool                                                m_userAddInProgress;
        std::function<SignInUserResultCallback>             m_signInUserResultCallback;

        // Gamerpic handling
        AsyncHandle                                         m_nextGamerPicAsyncHandle;
        std::map<AsyncHandle, std::unique_ptr<XAsyncBlock>> m_pendingGamerPicAsyncBlocks;
        std::map<AsyncHandle, std::unique_ptr<XAsyncBlock>> m_abortedGamerPicAsyncBlocks;
    };

}
