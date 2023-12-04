//--------------------------------------------------------------------------------------
// UserManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "SimpleLogManager.h"

class UserManager
{
public:
    UserManager(SimpleLogManager* m_logger);
    ~UserManager();
    HRESULT LoadUserHandle(XUserAddOptions xUserAddOptions);
    HRESULT LoadGamerPic();
    void SetHandle(XUserHandle handle);
    XUserHandle GetUserHandle();
    std::pair<uint8_t*, size_t> GetGamerPicData();
    std::string GetGamerPicGamerTag();

    struct GamerPicBytes
    {
        std::unique_ptr<uint8_t[]> data;
        size_t size;
    };

private:
    void SetGamerPicGamerTag(std::string);

    // User sign in.
    XUserHandle                                 m_userHandle;
    XTaskQueueHandle                            m_taskQueue;
    bool                                        m_userAddInProgress;
    std::atomic_bool                            m_userHandleIsAvailable;
    GamerPicBytes                               m_gamerPic;
    std::string                                 m_gamerPicGamertag;

    // Logging set up
    SimpleLogManager*                             m_logger;
};
