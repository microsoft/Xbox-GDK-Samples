//--------------------------------------------------------------------------------------
// UserManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleLogManager.h"
#include "UserManager.h"

using namespace DirectX;
UserManager::UserManager(SimpleLogManager* logger):
    m_userHandle(nullptr),
    m_taskQueue(nullptr),
    m_userAddInProgress(false),
    m_userHandleIsAvailable(false),
    m_gamerPic({}),
    m_gamerPicGamertag(""),
    m_logger(logger)
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

    struct AsyncBlockContext
    {
        UserManager* userManager;
        SimpleLogManager* logger;

    };
    AsyncBlockContext* ctx = new AsyncBlockContext();
    ctx->userManager = this;
    ctx->logger = m_logger;

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = ctx;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        AsyncBlockContext* context = static_cast<AsyncBlockContext*>(asyncBlock->context);
        UserManager* pThis = context->userManager;

        XUserHandle newUser = nullptr;
        HRESULT hr = XUserAddResult(asyncBlock, &newUser);

        if (SUCCEEDED(hr))
        {
            if (pThis->m_userHandle)
            {
                XUserCloseHandle(pThis->m_userHandle);
            }
            pThis->SetHandle(newUser);
            context->logger->Log("Successfully obtained User Handle");
            pThis->LoadGamerPic();
        }
        else
        {
            context->logger->LogFailedHR(hr, "XUserAddResult");
        }

        pThis->m_userAddInProgress = false;
        delete context;
        delete asyncBlock;
    };

    HRESULT hr = XUserAddAsync(userAddOption, asyncBlock);
    if (SUCCEEDED(hr))
    {
        m_userAddInProgress = true;
    }
    else
    {
        m_logger->LogFailedHR(hr, "XUserAddAsync");
        delete asyncBlock;
    }

    return hr;
}

HRESULT UserManager::LoadGamerPic()
{
    char gamertagBuffer[XUserGamertagComponentUniqueModernMaxBytes + 1] = {};
    size_t gamertagSize = 0;
    DX::ThrowIfFailed(XUserGetGamertag(m_userHandle, XUserGamertagComponent::UniqueModern, sizeof(gamertagBuffer), gamertagBuffer, &gamertagSize));

    struct AsyncBlockContext
    {
        GamerPicBytes* gamerPicBytes;
        std::string potentialGamertag;
        UserManager* pThis;
    };

    AsyncBlockContext* ctx = new AsyncBlockContext();
    ctx->gamerPicBytes = &m_gamerPic;
    ctx->potentialGamertag = gamertagBuffer;
    ctx->pThis = this;

    // Setup gamerpic request
    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = ctx;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        AsyncBlockContext* ctx = static_cast<AsyncBlockContext*>(asyncBlock->context);
        // Get buffer size
        size_t bufferSize = 0;
        DX::ThrowIfFailed(XUserGetGamerPictureResultSize(asyncBlock, &bufferSize));

        size_t bufferUsed = 0;

        ctx->gamerPicBytes->size = bufferSize;
        ctx->gamerPicBytes->data = std::make_unique<uint8_t[]>(bufferSize);

        DX::ThrowIfFailed(XUserGetGamerPictureResult(asyncBlock, ctx->gamerPicBytes->size, ctx->gamerPicBytes->data.get(), &bufferUsed));

        // Now that we know the gamerpic is valid, we can set the gamertag of our gamerpic to the gamertag we recieved earlier.
        ctx->pThis->SetGamerPicGamerTag(ctx->potentialGamertag);

        delete ctx;
        delete asyncBlock;
    };

    // Request gamerpic
    HRESULT hr = XUserGetGamerPictureAsync(m_userHandle, XUserGamerPictureSize::Medium, asyncBlock);
    if (FAILED(hr))
    {
        m_logger->LogFailedHR(hr, "XUserGetGamerPictureAsync");
    }
    return hr;
}

std::pair<uint8_t*, size_t> UserManager::GetGamerPicData()
{
    // data, size in bytes
    std::pair<uint8_t*, size_t> toReturn;
    toReturn.first = m_gamerPic.data.get();
    toReturn.second = m_gamerPic.size;
    return toReturn;
}

void UserManager::SetHandle(XUserHandle handle)
{
    m_userHandle = handle;
    m_userHandleIsAvailable = true;
}

void UserManager::SetGamerPicGamerTag(std::string gamerTag)
{
    m_gamerPicGamertag = gamerTag;
}

std::string UserManager::GetGamerPicGamerTag()
{
    return m_gamerPicGamertag;
}

XUserHandle UserManager::GetUserHandle()
{
    if (m_userHandleIsAvailable)
    {
        return m_userHandle;
    }
    return nullptr;
}
