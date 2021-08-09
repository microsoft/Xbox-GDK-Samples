//--------------------------------------------------------------------------------------
// UserManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
struct UserContext
{
    UserContext() :
#ifdef _GAMING_XBOX
        PreviousReading(nullptr),
        CurrentReading(nullptr),
#endif
        UserHandle(nullptr),
        ContextHandle(nullptr),
        LocalUserId{},
        HasMic(false)
    {
    }

    ~UserContext()
    {
        XblContextCloseHandle(ContextHandle);
        XUserCloseHandle(UserHandle);
    }

#ifdef _GAMING_XBOX
    Microsoft::WRL::ComPtr<IGameInputReading> PreviousReading;
    Microsoft::WRL::ComPtr<IGameInputReading> CurrentReading;
#endif
    XUserHandle UserHandle;
    XblContextHandle ContextHandle;
    XUserLocalId LocalUserId;
    bool HasMic;
};

class UserManager
{
public:
    UserManager() noexcept;
    ~UserManager();

    UserManager(UserManager&&) = delete;
    UserManager& operator= (UserManager&&) = delete;

    UserManager(UserManager const&) = delete;
    UserManager& operator=(UserManager const&) = delete;

    HRESULT Initialize();
    void InvokeSystemAddUser();
    std::shared_ptr<UserContext> GetUserByHandle(XUserHandle user);
    std::shared_ptr<UserContext> GetUserByLocalId(XUserLocalId user);
    std::vector<std::shared_ptr<UserContext>> GetUsers();

#ifdef _GAMING_XBOX
    void ScanUserInput();
#endif

private:
    void OnAudioEndpointChangeEvent(XUserLocalId user, XUserDefaultAudioEndpointKind defaultAudioEndpointKind, const wchar_t* endpointIdUtf16);
    void OnDeviceAssocationChangeEvent(const XUserDeviceAssociationChange* change);

private:
#ifdef _GAMING_XBOX
    Microsoft::WRL::ComPtr<IGameInput> m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputDevice> m_pendingDevice;
    GameInputGamepadState m_previousBindingState;
#endif
    XTaskQueueRegistrationToken m_audioEndpointChangedToken;
    XTaskQueueRegistrationToken m_deviceChangedToken;
    std::map<APP_LOCAL_DEVICE_ID, std::shared_ptr<UserContext>> m_mapDeviceToUser;
    std::mutex m_usersLock;
    bool m_initialized;
};

