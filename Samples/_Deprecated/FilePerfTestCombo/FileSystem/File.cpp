//--------------------------------------------------------------------------------------
// File.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "File.h"

using namespace FileSystem;

#ifdef THREAD_SAFE
ATG::CriticalSectionLockable File::s_threadMutex(500);
#endif

HANDLE File::s_ioCompletionPort = nullptr;
std::atomic<uint32_t> File::s_eventCacheInitialSize;
File::OpenFileList* File::s_openFiles = nullptr;
File::PendingOverlapList* File::s_masterPendingOverlap = nullptr;
File::EventCacheList* File::s_eventCache = nullptr;
std::atomic<uint64_t> File::s_asyncReadIndex = 0;
std::atomic<uint32_t> File::s_refCount = 0;
ATG::LockLessMemoryPool<File::PendingOverlapData, 1536> File::s_pendingOverlapAllocator;
ATG::LockLessMemoryPool<OVERLAPPED, 1536> File::s_overlappedAllocator;

#ifdef USE_COMPLETION_PORTS
extern "C"
void Win32OverlappedCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    File::PendingOverlapData* data = (File::PendingOverlapData*)lpOverlapped->hEvent;
    SetEvent(data->completeEvent);
    if (data->callback)
    {
        DWORD bytesRead;
        if (!GetOverlappedResultEx(data->owner, data->pending, &bytesRead, 0, FALSE))
            data->callback(dwErrorCode, data->requestId, data->userData);
        else
            data->callback(S_OK, data->requestId, data->userData);
    }
}
#endif

void File::InitFileSystem(bool useCompletionPorts, uint32_t initialEventCount)
{
    if (s_refCount.fetch_add(1) == 0)
    {
#ifdef THREAD_SAFE
        std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
        if (useCompletionPorts)
        {
            s_ioCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, s_ioCompletionPort, 0, 1);
        }
        s_openFiles = new OpenFileList;
        s_masterPendingOverlap = new PendingOverlapList;
        s_masterPendingOverlap->reserve(2000);

        if (initialEventCount > c_maxEventCacheInitialSize)
            initialEventCount = c_maxEventCacheInitialSize;
        s_eventCache = new EventCacheList;
        s_eventCacheInitialSize = initialEventCount;
        s_eventCache->reserve(s_eventCacheInitialSize);
        for (uint32_t i = 0; i < s_eventCacheInitialSize; i++)
        {
            // NOTE: All events are created with manual reset
            // This avoids race conditions that can exist when used with asynchronous operations
            s_eventCache->push_back(CreateEvent(nullptr, TRUE, FALSE, nullptr));
        }
    }
}

void File::ShutdownFileSystem(void)
{
    assert(s_refCount > 0);
    if (s_refCount.fetch_sub(1) == 1)
    {
        assert(s_openFiles);
        assert(s_eventCache);
        if (s_openFiles)
        {
            // need to copy the list because delete on file will modify the list
            // the list stores pointers so this operation is very fast
            OpenFileList tempList = *s_openFiles;
            OpenFileList::iterator iter, endIter;
            endIter = tempList.end();
            for (iter = tempList.begin(); iter != endIter; ++iter)
            {
                delete* iter;
            }
            s_openFiles->clear();
        }

        if (s_eventCache)
        {
            EventCacheList::iterator iter, endIter;
            endIter = s_eventCache->end();
            for (iter = s_eventCache->begin(); iter != endIter; ++iter)
            {
                CloseHandle(*iter);
            }
        }

        s_openFiles = nullptr;
        s_masterPendingOverlap = nullptr;
        if (s_ioCompletionPort)
            CloseHandle(s_ioCompletionPort);
        s_ioCompletionPort = nullptr;
    }
}

HANDLE File::RequestEvent(void)
{
#ifdef THREAD_SAFE
    std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
    size_t cacheSize = s_eventCache->size();
    if (cacheSize == 0)
    {
        // NOTE: All events are created with manual reset
        // This avoid race conditions that can exist when used with asynchronous operations
        return CreateEvent(nullptr, TRUE, FALSE, nullptr);
    }

    // pull from the end of the list to avoid shift operation
    HANDLE toret = (*s_eventCache)[cacheSize - 1];
    s_eventCache->resize(cacheSize - 1);
    return toret;
}

void File::ReleaseEvent(HANDLE event)
{
#ifdef THREAD_SAFE
    std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
    if (s_eventCache->size() >= s_eventCacheInitialSize)
    {
        CloseHandle(event);
    }
    else
    {
        // need to make sure it's set back to not signaled for next asynchronous operation
        // This function is a noop if the event is already not signalled
        ResetEvent(event);
        s_eventCache->push_back(event);
    }
}

void File::FileClosed(File* file)
{
#ifdef THREAD_SAFE
    std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
    OpenFileList::iterator iter, endIter;
    endIter = s_openFiles->end();
    for (iter = s_openFiles->begin(); iter != endIter; ++iter)
    {
        if (*iter == file)
        {
            // flow to call this function goes through destructor on File
            // so the pointer has already been cleaned up
            s_openFiles->erase(iter);
            break;
        }
    }
}

File* File::OpenFileForWriting(const std::wstring& fileName, HRESULT& errorResult)
{
#ifdef THREAD_SAFE
    std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
    errorResult = S_OK;
    if (s_openFiles == nullptr)
        InitFileSystem(false, 0);

    File* newFile = new File(fileName, 0, 0);
    if (!newFile)
    {
        return nullptr;
    }
    // The ability to use non-default flags is not supported for file opened for writing
    errorResult = newFile->InternalCreateFile();
    if (FAILED(errorResult))
    {
        delete newFile;
        return nullptr;
    }
    s_openFiles->push_back(newFile);
    return newFile;
}

File* File::OpenFile(const std::string& fileName, HRESULT& errorResult, uint32_t creationDisposition, uint32_t creationFlags)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(fileName);

    return OpenFile(wide, errorResult, creationDisposition, creationFlags);
}

File* File::OpenFile(const std::wstring& fileName, HRESULT& errorResult, uint32_t creationDisposition, uint32_t flags)
{
#ifdef THREAD_SAFE
    std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
    errorResult = S_OK;
    if (s_openFiles == nullptr)
        InitFileSystem(false, 0);

    File* newFile = new File(fileName, creationDisposition, flags);
    if (!newFile)
    {
        return nullptr;
    }

    errorResult = newFile->InternalOpenFile();
    if (FAILED(errorResult))
    {
        delete newFile;
        return nullptr;
    }
    s_openFiles->push_back(newFile);
    return newFile;
}

void File::CloseFile(File* file)
{
#ifdef THREAD_SAFE
    std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
    OpenFileList::iterator iter, endIter;
    endIter = s_openFiles->end();
    for (iter = s_openFiles->begin(); iter != endIter; ++iter)
    {
        if (*iter == file)
        {
            // destructor will end up calling FileClosed which will clean up s_openFiles
            delete* iter;
            break;
        }
    }
}

File::File(const std::wstring& fileName, uint32_t creationDisposition, uint32_t creationFlags)
{
    m_fileName = fileName;
    m_creationDisposition = creationDisposition;
    m_creationFlags = creationFlags;
    m_fileHandle = INVALID_HANDLE_VALUE;
    m_preloadBuffer = nullptr;
}

File::~File()
{
    // Tell the internal FileManager to cleanup s_openFiles;
    FileClosed(this);
    if (m_fileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_fileHandle);
    }

    if (m_preloadBuffer)
    {
        VirtualFree(m_preloadBuffer, 0, MEM_RELEASE);
    }
}

HRESULT File::InternalCreateFile(void)
{
    memset(&m_fileAttributes, 0, sizeof(m_fileAttributes));
    m_fileSize = 0;
    uint32_t creationFlags = FILE_ATTRIBUTE_NORMAL;

    CREATEFILE2_EXTENDED_PARAMETERS params;
    memset(&params, 0, sizeof(params));

    params.dwSize = sizeof(params);
    params.dwFileAttributes = creationFlags;
    m_fileHandle = CreateFile2(m_fileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, CREATE_ALWAYS, &params);
    if (!m_fileHandle)
        return HRESULT_FROM_WIN32(GetLastError());

    m_filePointer = 0;

    return S_OK;
}

HRESULT File::InternalOpenFile(void)
{
    // Make sure the file actually exists, if not then just return error
    if (!GetFileAttributesEx(m_fileName.c_str(), GetFileExInfoStandard, &m_fileAttributes))
        return HRESULT_FROM_WIN32(GetLastError());

    m_fileSize = m_fileAttributes.nFileSizeHigh;
    m_fileSize <<= 32ULL;
    m_fileSize += m_fileAttributes.nFileSizeLow;
    uint32_t attributeFlags = FILE_ATTRIBUTE_NORMAL;
    uint32_t fileFlags = 0;

    if (m_creationFlags & OpenFlags::e_openFlag_Preload)
    {
        // need to force no buffering since that requires 4k alignment, size of file may not be aligned to 4k
        // technically not set at this point in the code, but here to avoid possible bugs if previous code is changed
        fileFlags -= FILE_FLAG_NO_BUFFERING;
    }
    if (m_creationFlags & e_openFlag_NoBuffering)
    {
        fileFlags |= FILE_FLAG_NO_BUFFERING;
    }
    if (m_creationFlags & e_openFlag_EnableAsync)
    {
        fileFlags = FILE_FLAG_OVERLAPPED;
        // asynchronous operations require opening the file without the internal OS cache enabled
        fileFlags |= FILE_FLAG_NO_BUFFERING;
    }

    // enable this flag to force all file to be opened with no buffering, however all read requests must be aligned to 4k in this case.
    fileFlags |= FILE_FLAG_NO_BUFFERING;

    CREATEFILE2_EXTENDED_PARAMETERS params;
    memset(&params, 0, sizeof(params));

    params.dwSize = sizeof(params);
    params.dwFileAttributes = attributeFlags;
    params.dwFileFlags = fileFlags;

    m_fileHandle = CreateFile2(m_fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, m_creationDisposition, &params);
    if (m_fileHandle == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (s_ioCompletionPort && (fileFlags & FILE_FLAG_OVERLAPPED))
    {
        HANDLE newPort = CreateIoCompletionPort(m_fileHandle, s_ioCompletionPort, 0, 1);
        if ((newPort == nullptr) || (newPort != s_ioCompletionPort))
        {
            HRESULT toret = HRESULT_FROM_WIN32(GetLastError());
            if (newPort != nullptr)
                CloseHandle(newPort);
            CloseHandle(m_fileHandle);
            m_fileHandle = INVALID_HANDLE_VALUE;
            return toret;
        }
    }
    m_filePointer = 0;

    if (m_creationFlags & OpenFlags::e_openFlag_Preload)
    {
        Preload();
    }
    return S_OK;
}

HRESULT File::StartInternalAsyncRead(void* buffer, uint32_t bytesToRead, uint64_t position, AsyncRequestId& requestId, AsyncCallback callback, uintptr_t userData, bool useEvent)
{
    AsyncRequestList temp;

    temp.push_back(AsyncRequestData(buffer, bytesToRead, position, useEvent));
    return StartInternalAsyncRead(temp, requestId, callback, userData);
}

HRESULT File::StartInternalAsyncRead(const AsyncRequestList& list, AsyncRequestId& requestId, AsyncCallback callback, uintptr_t userData)
{
    DWORD errorCode;
    requestId = UINT64_MAX;
    {
        AsyncRequestList::const_iterator iter, endIter;
        endIter = list.end();
        for (iter = list.begin(); iter != endIter; ++iter)
        {
            if (m_creationFlags & e_openFlag_EnableAsync)
            {
                // all asynchronous operations require a unique OVERLAPPED structure that is valid during the entire read
                OVERLAPPED* winData = s_overlappedAllocator.allocate();
                memset(winData, 0, sizeof(OVERLAPPED));
                winData->Pointer = reinterpret_cast<void*> (iter->position);
#ifdef USE_COMPLETION_PORTS
                HANDLE specialEvent = RequestEvent();
                PendingOverlapData* completeData = new(s_pendingOverlapAllocator.allocate()) PendingOverlapData(this, callback, userData, requestId, winData);
                completeData->completeEvent = specialEvent;
                winData->hEvent = completeData;
#else
                if (iter->useEvent)
                    winData->hEvent = RequestEvent();
                else
                    winData->hEvent = nullptr;
#endif

                // Attempt to start the asynchronous read operation.
                // In some cases this may return immediately with the actual data already available
#ifdef USE_COMPLETION_PORTS
                if (ReadFileEx(m_fileHandle, iter->buffer, iter->bytesToRead, winData, Win32OverlappedCompletionRoutine))
#else
                if (!ReadFile(m_fileHandle, iter->buffer, iter->bytesToRead, nullptr, winData))
#endif
                {
                    errorCode = GetLastError();
                    // if the OS actually starts an asynchronous operation then the error ERROR_IO_PENDING will be returned
                    // This is not an actual error even though ReadFile returned false
                    if ((errorCode != ERROR_IO_PENDING) && (errorCode != ERROR_SUCCESS))
                    {
#ifdef USE_COMPLETION_PORTS
                        ReleaseEvent(specialEvent);
                        s_pendingOverlapAllocator.deallocate(completeData);
#else
                        if (winData->hEvent)
                            ReleaseEvent(winData->hEvent);
#endif
                        s_overlappedAllocator.deallocate(winData);
                        return HRESULT_FROM_WIN32(errorCode);
                    }
                    requestId = s_asyncReadIndex++;
#ifdef USE_COMPLETION_PORTS
                    PendingOverlapData* newData = completeData;
#else
                    PendingOverlapData* newData = new(s_pendingOverlapAllocator.allocate()) PendingOverlapData(this, callback, userData, requestId, winData);
#endif
                    {
#ifdef THREAD_SAFE
                        std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
                        s_masterPendingOverlap->push_back(newData);
                    }
                    return S_OK;
                }
                else	// asynchronous call to ReadFile was converted to a synchronous call by the OS, data is available now
                {		// result will be picked up on the Check call later by the user. Want to avoid calling the callback here, always treat as asynchronous
#ifdef USE_COMPLETION_PORTS
                    ReleaseEvent(specialEvent);
                    s_pendingOverlapAllocator.deallocate(completeData);
#else
                    if (winData->hEvent)
                        ReleaseEvent(winData->hEvent);
#endif
                    s_overlappedAllocator.deallocate(winData);
                }
            }
            else  // start a synchronous read operation
            {
                // It is possible to use an OVERLAPPED structure here to avoid the two OS calls
                // This helps with thread safety if desired
                LARGE_INTEGER newPosition;
                newPosition.QuadPart = static_cast<LONGLONG> (iter->position);
                {
#ifdef THREAD_SAFE
                    std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
                    SetFilePointerEx(m_fileHandle, newPosition, nullptr, FILE_BEGIN);
                    BOOL success = ReadFile(m_fileHandle, iter->buffer, iter->bytesToRead, nullptr, nullptr);
                    if (!success)
                    {
                        return HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
        }
    }
    return S_OK;
}

HRESULT File::WaitCompletionPortRead(bool shouldWait)
{
    if (s_ioCompletionPort == nullptr)
        return S_FALSE;
    while (true)
    {
        DWORD bytesTransfered;
        ULONG_PTR completionKey;
        OVERLAPPED* overlappedResult;
        if (!GetQueuedCompletionStatus(s_ioCompletionPort, &bytesTransfered, &completionKey, &overlappedResult, shouldWait ? INFINITE : 0))
            return WAIT_TIMEOUT;

        PendingOverlapData* signaledEntry = nullptr;
        {
#ifdef THREAD_SAFE
            std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
            auto endIter = s_masterPendingOverlap->end();
            for (auto iter = s_masterPendingOverlap->begin(); iter != endIter; ++iter)
            {
                if ((*iter)->pending == overlappedResult)
                {
                    signaledEntry = *iter;
                    s_masterPendingOverlap->erase(iter);
                    break;
                }
            }
        }

        if (signaledEntry != nullptr)
        {
            // The event could be signaled (which means complete) but the OS encountered a general error reading the data
            DWORD bytesRead;
            if (!GetOverlappedResultEx(signaledEntry->owner, signaledEntry->pending, &bytesRead, 0, FALSE))
                signaledEntry->callback(HRESULT_FROM_WIN32(GetLastError()), signaledEntry->requestId, signaledEntry->userData);
            else
                signaledEntry->callback(S_OK, signaledEntry->requestId, signaledEntry->userData);

            if (signaledEntry->pending->hEvent)
                ReleaseEvent(signaledEntry->pending->hEvent);
            s_pendingOverlapAllocator.deallocate(signaledEntry);

            shouldWait = false;		// we had at least one success so never wait for the next set, only want to see if more have finished
        }
        else
        {
            break;
        }
    }
    return S_OK;
}

HRESULT File::WaitAnyAsyncRead(bool shouldWait)
{
    while (true)
    {
        // it's ugly but we need to recreate this list each time through because we modify the vector on each completion
        HANDLE waitEvents[MAXIMUM_WAIT_OBJECTS];
        PendingOverlapData* matchingWaitEntries[MAXIMUM_WAIT_OBJECTS];
        uint32_t numWaitEvents = 0;

        {
#ifdef THREAD_SAFE
            std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
            auto endIter = s_masterPendingOverlap->end();
            for (auto iter = s_masterPendingOverlap->begin(); iter != endIter; ++iter)
            {
#ifdef USE_COMPLETION_PORTS
                if ((*iter)->completeEvent)
                {
                    matchingWaitEntries[numWaitEvents] = *iter;
                    waitEvents[numWaitEvents++] = (*iter)->completeEvent;
                    if (numWaitEvents >= 64)
                        break;
                }
#else
                if ((*iter)->pending->hEvent)
                {
                    matchingWaitEntries[numWaitEvents] = *iter;
                    waitEvents[numWaitEvents++] = (*iter)->pending->hEvent;
                    if (numWaitEvents >= 64)
                        break;
                }
#endif
            }
        }

        if (numWaitEvents == 0)
            return S_OK;
        PendingOverlapData* signaledEntry = nullptr;
        uint32_t signaledIndex = 0;
        DWORD waitReturn = UINT32_MAX;

#ifdef USE_COMPLETION_PORTS
        waitReturn = WaitForMultipleObjectsEx(numWaitEvents, waitEvents, FALSE, shouldWait ? INFINITE : 0, TRUE);
#else
        waitReturn = WaitForMultipleObjects(numWaitEvents, waitEvents, FALSE, shouldWait ? INFINITE : 0);
#endif
        if ((waitReturn >= WAIT_OBJECT_0) && (waitReturn < WAIT_OBJECT_0 + numWaitEvents))
            signaledEntry = matchingWaitEntries[waitReturn - WAIT_OBJECT_0];
#ifdef USE_COMPLETION_PORTS
        else if (waitReturn == WAIT_IO_COMPLETION)
            return WaitAnyAsyncRead(false);
#endif
        else if (waitReturn == WAIT_FAILED)
            return HRESULT_FROM_WIN32(GetLastError());
        else if ((waitReturn == WAIT_TIMEOUT) && (!shouldWait))
            return WAIT_TIMEOUT;
        else    // some form of abandoned or timeout (when INFINITE), don't care in this case just say the data is invalid
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        // had to be a WAIT_OBJECT return value here
        signaledIndex = waitReturn - WAIT_OBJECT_0;

        if (signaledEntry != nullptr)
        {
            // The event could be signaled (which means complete) but the OS encountered a general error reading the data
#ifndef USE_COMPLETION_PORTS
            DWORD bytesRead;
            if (!GetOverlappedResultEx(signaledEntry->owner, signaledEntry->pending, &bytesRead, 0, FALSE))
                signaledEntry->callback(HRESULT_FROM_WIN32(GetLastError()), signaledEntry->requestId, signaledEntry->userData);
            else
                signaledEntry->callback(S_OK, signaledEntry->requestId, signaledEntry->userData);
#endif
            {
#ifdef THREAD_SAFE
                std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
                auto endIter = s_masterPendingOverlap->end();
                for (auto iter = s_masterPendingOverlap->begin(); iter != endIter; ++iter)
                {
                    if ((*iter) == signaledEntry)
                    {
                        s_masterPendingOverlap->erase(iter);
                        break;
                    }
                }
            }

#ifdef USE_COMPLETION_PORTS
            if (signaledEntry->completeEvent)
                ReleaseEvent(signaledEntry->completeEvent);
#else
            if (signaledEntry->pending->hEvent)
                ReleaseEvent(signaledEntry->pending->hEvent);
#endif
            s_pendingOverlapAllocator.deallocate(signaledEntry);

            shouldWait = false;		// we had at least one success so never wait for the next set, only want to see if more have finished
        }
        else
        {
            break;
        }
    }
    return S_OK;
}

HRESULT File::CheckAnyAsyncRead()
{
    DWORD bytesRead;

    bool foundWork = true;
    while (foundWork)
    {
        foundWork = false;
        s_threadMutex.lock();
        for (auto iter = s_masterPendingOverlap->begin(); iter != s_masterPendingOverlap->end(); )
        {
            if ((*iter)->pending->hEvent == nullptr)
            {
                assert((*iter)->callback != nullptr);

                if (HasOverlappedIoCompleted((*iter)->pending))
                {
                    PendingOverlapData* pendingData = *iter;
                    s_masterPendingOverlap->erase(iter);
                    s_threadMutex.unlock();

                    if (!GetOverlappedResultEx(pendingData->owner, pendingData->pending, &bytesRead, 0, FALSE))
                    {
                        uint32_t errorCode = GetLastError();
                        pendingData->callback(HRESULT_FROM_WIN32(errorCode), pendingData->requestId, pendingData->userData);
                        if (pendingData->pending->hEvent)
                            ReleaseEvent(pendingData->pending->hEvent);
                        s_pendingOverlapAllocator.deallocate(pendingData);
                        return HRESULT_FROM_WIN32(errorCode);
                    }

                    pendingData->callback(S_OK, pendingData->requestId, pendingData->userData);
                    if (pendingData->pending->hEvent)
                        ReleaseEvent(pendingData->pending->hEvent);
                    s_pendingOverlapAllocator.deallocate(pendingData);
                    foundWork = true;
                    break;
                }
            }
            ++iter;
        }
        if (!foundWork)
            s_threadMutex.unlock();
    }
    return S_OK;
}

HRESULT File::WaitAsyncRead(AsyncRequestId asyncId, bool shouldWait)
{
    if (asyncId == UINT64_MAX)
        return S_OK;

    DWORD bytesRead;

    PendingOverlapList::iterator iter, endIter;
    PendingOverlapData* foundEntry = nullptr;
    {
#ifdef THREAD_SAFE
        std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
        endIter = s_masterPendingOverlap->end();
        for (iter = s_masterPendingOverlap->begin(); iter != endIter; ++iter)
        {
            if ((*iter)->requestId == asyncId)
            {
                foundEntry = *iter;
                break;
            }
        }
    }
    if (foundEntry == nullptr)      // TODO, determine if this is an error or not, the user passed in a bogus id
        return S_OK;

    if (shouldWait && (foundEntry->pending->hEvent != nullptr))
    {
        if (WaitForSingleObject(foundEntry->pending->hEvent, INFINITE) == WAIT_TIMEOUT)
            return WAIT_TIMEOUT;
    }
    else
    {
        if (!HasOverlappedIoCompleted(foundEntry->pending))
            return  WAIT_TIMEOUT;
    }

    // The event could be signaled (which means complete) but the OS encountered a general error reading the data
    HRESULT errorCode = S_OK;
    if (!GetOverlappedResultEx(foundEntry->owner, foundEntry->pending, &bytesRead, 0, FALSE))
    {
        errorCode = HRESULT_FROM_WIN32(GetLastError());
    }

    foundEntry->callback(errorCode, foundEntry->requestId, foundEntry->userData);
    {
#ifdef THREAD_SAFE
        std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif
        endIter = s_masterPendingOverlap->end();
        for (iter = s_masterPendingOverlap->begin(); iter != endIter; ++iter)
        {
            if ((*iter) == foundEntry)
            {
                s_masterPendingOverlap->erase(iter);
                break;
            }
        }
    }

    if (foundEntry->pending->hEvent)
        ReleaseEvent(foundEntry->pending->hEvent);
    s_pendingOverlapAllocator.deallocate(foundEntry);
    return S_OK;
}

HRESULT File::Preload(void)
{
    if (m_fileHandle == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }
    if (m_fileSize > UINT32_MAX)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (m_preloadBuffer)
    {
        VirtualFree(m_preloadBuffer, 0, MEM_RELEASE);
        m_preloadBuffer = nullptr;
    }

    m_preloadBuffer = reinterpret_cast<uint8_t*> (VirtualAlloc(nullptr, m_fileSize, MEM_COMMIT, PAGE_READWRITE));
    if (!m_preloadBuffer)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!ReadFile(m_fileHandle, m_preloadBuffer, static_cast<uint32_t> (m_fileSize), nullptr, nullptr))
    {
        Unload();
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}
HRESULT File::Unload(void)
{
    if (m_preloadBuffer)
    {
        VirtualAlloc(m_preloadBuffer, 0, MEM_DECOMMIT, PAGE_READWRITE);
        m_preloadBuffer = nullptr;
    }
    return S_OK;
}

size_t File::SetReadPointer(uint64_t newLocation)
{
    if (m_fileHandle == INVALID_HANDLE_VALUE)
    {
        return SIZE_MAX;
    }
    m_filePointer = newLocation;
    if (m_filePointer > m_fileSize)
        m_filePointer = m_fileSize;
    return m_filePointer;
}

HRESULT File::Write(void* buffer, uint32_t bytesToWrite, uint32_t* bytesActuallyWritten)
{
    if (m_fileHandle == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);

    if ((m_creationFlags & e_openFlag_EnableAsync) != 0)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    if (bytesActuallyWritten)
        *bytesActuallyWritten = 0;
    DWORD actuallyWritten;
    if (!WriteFile(m_fileHandle, buffer, bytesToWrite, &actuallyWritten, nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    if (bytesActuallyWritten)
        *bytesActuallyWritten = actuallyWritten;
    return S_OK;
}

HRESULT File::Read(void* buffer, uint32_t bytesToRead)
{
    if (m_fileHandle == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    // If the entire contents of the file have been preloaded then just memcpy from the internal buffer
    if (m_preloadBuffer)
    {
        if (m_filePointer >= m_fileSize)
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        if ((m_filePointer + bytesToRead) < m_filePointer)					// overflow
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        if ((m_filePointer + bytesToRead) >= m_fileSize)
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        memcpy(buffer, m_preloadBuffer + m_filePointer, bytesToRead);
        m_filePointer += bytesToRead;
        return S_OK;
    }

    // This function may be called for a file that is opened for asynchronous operation
    // The internal read function will work for all cases
    AsyncRequestId requestId;
    HRESULT error = StartInternalAsyncRead(buffer, bytesToRead, m_filePointer, requestId, DummyCallbackFunction, 0, true);
    if (!SUCCEEDED(error))
        return error;

    // Just wait for the read operation to complete, if the previous read call already read in the data
    // requestId will be UINT64_MAX, the WaitAsyncRead will return immediately
    // otherwise will wait until the read call completes with error or success
    error = WaitAsyncRead(requestId, true);
    if (!SUCCEEDED(error))
        return error;

    m_filePointer += bytesToRead;
    return S_OK;
}

void File::CheckForCompleteAsync(void)
{
    PendingOverlapList::iterator iter;
#ifdef THREAD_SAFE
    std::scoped_lock<ATG::CriticalSectionLockable> lock(s_threadMutex);
#endif

    for (iter = s_masterPendingOverlap->begin(); iter != s_masterPendingOverlap->end();)
    {
        // This is a quick call with a timeout of 0, no transition to ring 0 happens in that case

        if (HasOverlappedIoCompleted((*iter)->pending))
            //if (WaitForSingleObject((*iter)->pending->hEvent, 0) != WAIT_TIMEOUT)
        {
            DWORD bytesRead;
            if (!GetOverlappedResultEx((*iter)->owner->m_fileHandle, (*iter)->pending, &bytesRead, 0, FALSE))
            {
                DWORD lastError = GetLastError();
                // GetOverlappedResult will only return true if the read is complete.
                // ERROR_IO_PENDING is not a true error
                if (lastError != ERROR_IO_PENDING)
                {
                    //TODO look at canceling the other load requests associated with the full read
                    (*iter)->callback(HRESULT_FROM_WIN32(lastError), (*iter)->requestId, (*iter)->userData);
                    if ((*iter)->pending->hEvent)
                        ReleaseEvent((*iter)->pending->hEvent);
                    s_pendingOverlapAllocator.deallocate(*iter);
                    iter = s_masterPendingOverlap->erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
            else  // read is complete
            {
                (*iter)->callback(S_OK, (*iter)->requestId, (*iter)->userData);
                if ((*iter)->pending->hEvent)
                    ReleaseEvent((*iter)->pending->hEvent);
                s_pendingOverlapAllocator.deallocate(*iter);
                iter = s_masterPendingOverlap->erase(iter);
            }
        }
        else  // read has not signaled completing yet
        {
            ++iter;
        }
    }
}

HRESULT File::AsyncRead(void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t userData, AsyncRequestId& requestId, AsyncCallback callback, bool useEvent)
{
    if (m_fileHandle == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    // If the entire contents of the file have been preloaded then just memcpy from the internal buffer
    if (m_preloadBuffer)
    {
        requestId = UINT64_MAX;
        memcpy(buffer, m_preloadBuffer + m_filePointer, bytesToRead);
        m_filePointer += bytesToRead;
        return S_OK;
    }

    // The file was not opened for asynchronous operation so just perform a synchronous read
    if ((m_creationFlags & e_openFlag_EnableAsync) == 0)
    {
        requestId = UINT64_MAX;
        SetReadPointer(readLocation);
        HRESULT result = Read(buffer, bytesToRead);
        callback(result, UINT64_MAX, userData);
        return result;
    }

    uint8_t* curOutputBuffer = static_cast<uint8_t*> (buffer);
    uint64_t startPosition, endPosition;
    uint64_t curPosition = readLocation;
    startPosition = curPosition;
    endPosition = curPosition + bytesToRead;

    AsyncRequestList pendingAsyncRequestList;

    pendingAsyncRequestList.push_back(AsyncRequestData(curOutputBuffer, bytesToRead, curPosition, useEvent));

    return StartInternalAsyncRead(pendingAsyncRequestList, requestId, callback, userData);
}
