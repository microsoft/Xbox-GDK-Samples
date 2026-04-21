//--------------------------------------------------------------------------------------
// UserManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UserManager.h"

using namespace DirectX;
UserManager::UserManager():
    m_userHandle(nullptr),
    m_taskQueue(nullptr),
    m_userAddInProgress(false),
    m_userHandleIsAvailable(false)
{
    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::ThreadPool,
        &m_taskQueue));
}

UserManager::~UserManager()
{
    XTaskQueueTerminate(m_taskQueue, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Work, INFINITE);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, INFINITE);
    XTaskQueueCloseHandle(m_taskQueue);

    XUserCloseHandle(m_userHandle);
}

HRESULT UserManager::LoadUserHandle(XUserAddOptions userAddOption)
{
    if (m_userAddInProgress)
    {
        return S_FALSE;
    }

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        UserManager* pThis = static_cast<UserManager*>(asyncBlock->context);

        XUserHandle newUser = nullptr;
        HRESULT hr = XUserAddResult(asyncBlock, &newUser);

        if (SUCCEEDED(hr))
        {
            if (pThis->m_userHandle)
            {
                XUserCloseHandle(pThis->m_userHandle);
            }
            pThis->SetHandle(newUser);
            OutputDebugStringA("Successfully obtained User Handle\n");
        }
        else
        {
            char buffer[100];
            sprintf_s(buffer, sizeof(buffer), u8"XUserAddResult HR=%08X\n", hr);
            std::string message = buffer;
            OutputDebugStringA(buffer);
        }

        pThis->m_userAddInProgress = false;
        delete asyncBlock;
    };

    HRESULT hr = XUserAddAsync(userAddOption, asyncBlock);
    if (SUCCEEDED(hr))
    {
        m_userAddInProgress = true;
    }
    else
    {
        char buffer[100];
        sprintf_s(buffer, sizeof(buffer), u8"XUserAddResult HR=%08X\n", hr);
        OutputDebugStringA(buffer);
        delete asyncBlock;
    }

    return hr;
}

void UserManager::SetHandle(XUserHandle handle)
{
    m_userHandle = handle;
    m_userHandleIsAvailable = true;
}

XUserHandle UserManager::GetUserHandle()
{
    if (m_userHandleIsAvailable)
    {
        return m_userHandle;
    }
    OutputDebugStringA("User Handle not loaded yet.\n");
    return nullptr;
}
