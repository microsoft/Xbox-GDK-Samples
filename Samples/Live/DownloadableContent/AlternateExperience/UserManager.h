//--------------------------------------------------------------------------------------
// UserManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "DeviceResources.h"

class UserManager
{
public:
    UserManager();
    ~UserManager();
    HRESULT LoadUserHandle(XUserAddOptions xUserAddOptions);
    void SetHandle(XUserHandle handle);
    XUserHandle GetUserHandle();
private:

    // User sign in.
    XUserHandle                                 m_userHandle;
    XTaskQueueHandle                            m_taskQueue;
    bool                                        m_userAddInProgress;
    std::atomic_bool                            m_userHandleIsAvailable;
};
