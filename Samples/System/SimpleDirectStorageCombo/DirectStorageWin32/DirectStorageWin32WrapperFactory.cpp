//--------------------------------------------------------------------------------------
// DirectStorageWin32WrapperFactory.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DirectStorageWin32Wrapper.h"
#include <algorithm>

using namespace DirectStorageWin32Wrapper;
using namespace DirectStorageWin32Wrapper::Internal;

namespace
{
    DirectStorageFactory* g_masterFactory = nullptr;
    std::wstring		  s_nullFileName;
}

//////////////////////////////////////////////////////////////////////////
/// \brief GetInstance
/// \details DStorageGetFactory::GetInstance
/// \details DStorageGetFactory is a singleton object, matching GetInstance static function
//////////////////////////////////////////////////////////////////////////
DirectStorageFactory* DirectStorageFactory::GetInstance()
{
    assert(g_masterFactory);		// there should be no path where this is valid in the DS on Win32 implementation. A title also cannot call this function
    return g_masterFactory;
}

extern "C" {
    //////////////////////////////////////////////////////////////////////////
    /// \brief DStorageGetFactory
    /// \details DStorageGetFactory
    /// \details Instantiation of DStorageGetFactory function from Game Core DirectStorage spec
    //////////////////////////////////////////////////////////////////////////
    HRESULT DStorageGetFactory(REFIID riid, void** ppv)
    {
        if (riid != __uuidof(IDStorageFactoryWin32))
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        if (ppv == nullptr)
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

        if (g_masterFactory == nullptr)
        {
            g_masterFactory = new DirectStorageFactory();
        }
        g_masterFactory->AddRef();
        *ppv = g_masterFactory;
        if (g_masterFactory)
            return S_OK;
        return S_FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////
/// \brief QueryInterface
/// \details DirectStorageFactory::QueryInterface
/// \details Instantiation of IUnknown::QueryInterface
//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE DirectStorageFactory::QueryInterface(/* [in] */ REFIID riid,/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
{
    if (riid != __uuidof(IDStorageFactoryWin32))
        return E_NOINTERFACE;
    if (g_masterFactory == nullptr)
        return E_POINTER;
    *ppvObject = g_masterFactory;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
/// \brief AddRef
/// \details DirectStorageFactory::AddRef
/// \details Instantiation of IUnknown::AddRef
//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE DirectStorageFactory::AddRef(void)
{
    m_refCount++;
    return m_refCount.load();
}

//////////////////////////////////////////////////////////////////////////
/// \brief Release
/// \details DirectStorageFactory::Release
/// \details Instantiation of IUnknown::Release
//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE DirectStorageFactory::Release(void)
{
    assert(m_refCount != 0);
    if (m_refCount == 0)		// this case in theory should never happen since there is already an internal reference of 1. This could happen from a title error though
        return 0;
    m_refCount.fetch_sub(1);
    return m_refCount.load();
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageFactory
/// \details DirectStorageFactory::SetStagingBufferSize
/// \details Adjust the size of the internal staging buffer
/// \details Create or Destroy the internal Heap used as the staging buffer
//////////////////////////////////////////////////////////////////////////
HRESULT DirectStorageFactory::SetStagingBufferSize(UINT32 size)
{
    if ((size == 0) && (m_stagingBuffer != nullptr))
    {
        m_stagingBuffer = HeapCreate(0, c_initialStagingBufferSize, 0);
        if (!HeapDestroy(m_stagingBuffer))
            return GetLastError();
        m_stagingBuffer = nullptr;
    }
    else if ((size != 0) && (m_stagingBuffer == nullptr))
    {
        m_stagingBuffer = HeapCreate(0, c_initialStagingBufferSize, 0);
        if (m_stagingBuffer == nullptr)
            return GetLastError();
    }
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageFactory
/// \details DirectStorageFactory::DirectStorageFactory
/// \details Construct the singleton DirectStorageFactory object
//////////////////////////////////////////////////////////////////////////
DirectStorageFactory::DirectStorageFactory() :
    m_refCount(0)
    , m_threadMutex(100)
    , m_threadMode(ManagementThreadControlValues::PROCESS_QUEUES)
    , m_requestsSubmitted(1)
    , m_lastQueueSubmitted(0)
{
    static_assert (c_highPriorityThreshold == 1, "High priority threshold should always be 1 or there could be a stall");
    static_assert (DSTORAGE_PRIORITY_COUNT == 4, "If this fails then the default initialize table needs to be updated");
    m_priorityThreshold[DSTORAGE_PRIORITY_REALTIME - DSTORAGE_PRIORITY_FIRST] = c_realtimePriorityThreshold;
    m_priorityThreshold[DSTORAGE_PRIORITY_HIGH - DSTORAGE_PRIORITY_FIRST] = c_highPriorityThreshold;
    m_priorityThreshold[DSTORAGE_PRIORITY_NORMAL - DSTORAGE_PRIORITY_FIRST] = c_mediumPriorityThreshold;
    m_priorityThreshold[DSTORAGE_PRIORITY_LOW - DSTORAGE_PRIORITY_FIRST] = c_lowPriorityThreshold;
    m_stagingBuffer = HeapCreate(0, c_initialStagingBufferSize, 0);
    m_queues.reserve(c_maxQueues);
    m_kickThreadEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_kickDecompressionEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_managementThread = new std::thread(&DirectStorageFactory::QueueManagementProc, this);
    for (uint32_t i = 0; i < c_numDecompressionThreads; i++)
    {
        m_decompressionThreads.push_back(new std::thread(&DirectStorageFactory::DecompressionProc, this));
    }
}

//////////////////////////////////////////////////////////////////////////
/// \brief ~DirectStorageFactory
/// \details DirectStorageFactory::~DirectStorageFactory
/// \details The singleton is technically never destroyed until title shutdown
/// \details This is done to match functionality of Xbox DirectStorage behavior
//////////////////////////////////////////////////////////////////////////
DirectStorageFactory::~DirectStorageFactory()
{
    // destroy the background thread before processing any outstanding requests
    m_threadMode = ManagementThreadControlValues::SHUTDOWN;
    m_managementThread->join();
    for (auto& iter : m_decompressionThreads)
    {
        iter->join();
    }

    // Clean up any outstanding requests
    // TODO: Do we really need to do this if the singleton can't be deleted until title shutdown
    while (AnyOpenRequests())
    {
        ProcessSubmissionQueues();
        ProcessDecompressionQueues();
        Sleep(c_managementThreadSleepTimeMS);
    }
    CloseHandle(m_kickThreadEvent);
    CloseHandle(m_kickDecompressionEvent);
    for (auto& iter : m_queues)
    {
        delete iter;
    }

    for (auto& iter : m_openFiles)
    {
        CloseHandle(iter.file);
    }
    HeapDestroy(m_stagingBuffer);
}

//////////////////////////////////////////////////////////////////////////
/// \brief FindOpenFileByHash
/// \details DirectStorageFactory::FindOpenFileByHash
/// \details Brute force lookup for an open file entry based on the hash of the filename
//////////////////////////////////////////////////////////////////////////
std::vector<DirectStorageFactory::OpenFileEntry>::const_iterator DirectStorageFactory::FindOpenFileByHash(const uint64_t fileNameHash) const
{
    auto endIter = m_openFiles.end();
    for (auto iter = m_openFiles.begin(); iter != endIter; ++iter)
    {
        if (iter->nameHash == fileNameHash)
            return iter;
    }
    return endIter;
}

//////////////////////////////////////////////////////////////////////////
/// \brief FindOpenFileByHash
/// \details DirectStorageFactory::FindOpenFileByHash
/// \details Brute force lookup for an open file entry based on the file handle
//////////////////////////////////////////////////////////////////////////
std::vector<DirectStorageFactory::OpenFileEntry>::const_iterator DirectStorageFactory::FindOpenFileByHandle(HANDLE fileHandle) const
{
    auto endIter = m_openFiles.end();
    for (auto iter = m_openFiles.begin(); iter != endIter; ++iter)
    {
        if (iter->file == fileHandle)
            return iter;
    }
    return endIter;
}

//////////////////////////////////////////////////////////////////////////
/// \brief OpenFile
/// \details DirectStorageFactory::OpenFile
/// \details Instantiation of IDStorageFactoryWin32::OpenFile function
//////////////////////////////////////////////////////////////////////////
HRESULT DirectStorageFactory::OpenFile(_In_z_ const WCHAR* fileName, REFIID riid, _COM_Outptr_ void** ppv)
{
    if (riid != __uuidof(IDStorageFileWin32))
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    if (ppv == nullptr)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    if (fileName == nullptr)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    *ppv = nullptr;
    if (m_openFiles.size() == c_maxOpenFiles)
        return E_DSTORAGE_TOO_MANY_FILES;

    // quick check to make sure the file actually exists
    WIN32_FILE_ATTRIBUTE_DATA fileAttributes = {};
    if (!GetFileAttributesExW(fileName, GetFileExInfoStandard, &fileAttributes))
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    std::wstring normalizedFileName(fileName);
    std::for_each(normalizedFileName.begin(), normalizedFileName.end(), [](wchar_t& letter) {letter = towlower(letter); });
    std::for_each(normalizedFileName.begin(), normalizedFileName.end(), [](wchar_t& letter) {if (letter == L'/') letter = L'\\'; });

    std::hash<std::wstring> hash_fn;
    uint64_t uniqueFileID = hash_fn(normalizedFileName);

    std::lock_guard<ATG::CriticalSectionLockable> lock(m_threadMutex);
    auto fileEntry = FindOpenFileByHash(uniqueFileID);
    HANDLE realFile = INVALID_HANDLE_VALUE;
    HRESULT openError = S_OK;

    // A file by this name has not been opened yet so create a new entry
    if (fileEntry == m_openFiles.end())
    {
        OpenFileEntry newEntry;
        newEntry.refCount = 1;
        newEntry.nameHash = uniqueFileID;
        newEntry.file = OpenFile(fileName, openError);
        newEntry.fileAttributes = fileAttributes;
        if (newEntry.file != INVALID_HANDLE_VALUE)
        {
            m_openFiles.push_back(newEntry);
            realFile = newEntry.file;
        }
    }
    // A file with this has already been opened, reuse the existing HANDLE and increment its reference count
    else
    {
        realFile = fileEntry->file;
        fileEntry->refCount++;
        openError = S_OK;
    }

    // File was successfully opened
    if (realFile != INVALID_HANDLE_VALUE)
    {
        assert(SUCCEEDED(openError));
        DirectStorageFile* newFile = new DirectStorageFile(realFile);
        if (!newFile)
            return E_OUTOFMEMORY;
        newFile->AddRef();
        *ppv = newFile;
    }
    return openError;
}

//////////////////////////////////////////////////////////////////////////
/// \brief CreateQueue
/// \details DirectStorageFactory::CreateQueue
/// \details Instantiation of IDStorageFactoryWin32::CreateQueue function
//////////////////////////////////////////////////////////////////////////
HRESULT DirectStorageFactory::CreateQueue(const DSTORAGE_QUEUE_DESC* desc, REFIID riid, _COM_Outptr_ void** ppv)
{
    if (riid != __uuidof(IDStorageQueueWin32))
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    if (ppv == nullptr)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    if (desc == nullptr)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    *ppv = nullptr;

    if (desc->Capacity < DSTORAGE_MIN_QUEUE_CAPACITY)
        return E_DSTORAGE_INVALID_QUEUE_CAPACITY;
    if (desc->Capacity > DSTORAGE_MAX_QUEUE_CAPACITY)
        return E_DSTORAGE_INVALID_QUEUE_CAPACITY;

    if (desc->SourceType == DSTORAGE_REQUEST_SOURCE_MEMORY)
        return E_DSTORAGE_INVALID_SOURCE_TYPE;

    {
        std::lock_guard<ATG::CriticalSectionLockable> lock(m_threadMutex);

        if (m_queues.size() == c_maxQueues)
            return E_DSTORAGE_TOO_MANY_QUEUES;

        DirectStorageQueue* newQueue = new DirectStorageQueue(desc);
        *ppv = newQueue;
        m_queues.push_back(newQueue);
        newQueue->AddRef();					// one for the copy being returned
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
/// \brief CreateStatusArray
/// \details DirectStorageFactory::CreateStatusArray
/// \details Instantiation of IDStorageFactoryWin32::CreateStatusArray function
//////////////////////////////////////////////////////////////////////////
HRESULT DirectStorageFactory::CreateStatusArray(UINT32 capacity, PCSTR name, REFIID riid, _COM_Outptr_ void** ppv)
{
    if (riid != __uuidof(IDStorageStatusArrayWin32))
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    if (ppv == nullptr)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    *ppv = nullptr;
    DirectStorageStatusArray* newArray = new DirectStorageStatusArray(capacity, name);
    if (!newArray)
        return E_OUTOFMEMORY;
    *ppv = newArray;
    newArray->AddRef();					// one for the copy being returned

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
/// \brief RemoveQueue
/// \details DirectStorageFactory::RemoveQueue
/// \details Queues have their own reference count but they need to inform the factory when they are destroyed
/// \details This allows the factory to remove it from future processing
//////////////////////////////////////////////////////////////////////////
void DirectStorageFactory::RemoveQueue(DirectStorageQueue* queue)
{
    std::lock_guard<ATG::CriticalSectionLockable> lock(m_threadMutex);
    auto endIter = m_queues.end();
    for (auto iter = m_queues.begin(); iter != endIter; ++iter)
    {
        if (*iter == queue)
        {
            m_queues.erase(iter);
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/// \brief RemoveFile
/// \details DirectStorageFactory::RemoveFile
/// \details Files have their own reference count but they need to inform the factory when they are destroyed
/// \details The matching reference count on the file HANDLE is updated and the Win32 file object is closed if needed
//////////////////////////////////////////////////////////////////////////
void DirectStorageFactory::RemoveFile(HANDLE file)
{
    std::lock_guard<ATG::CriticalSectionLockable> lock(m_threadMutex);
    auto endIter = m_openFiles.end();
    for (auto iter = m_openFiles.begin(); iter != endIter; ++iter)
    {
        if (iter->file == file)
        {
            assert(iter->refCount > 0);
            iter->refCount--;
            if (iter->refCount == 0)
            {
                CloseHandle(iter->file);
                m_openFiles.erase(iter);
            }
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/// \brief SetCpuAffinity
/// \details DirectStorageFactory::SetCpuAffinity
/// \details Instantiation of IDStorageFactorX::SetCpuAffinity function
//////////////////////////////////////////////////////////////////////////
HRESULT DirectStorageFactory::SetCpuAffinity(UINT64 affinity)
{
    if (!m_managementThread)
        return HRESULT_FROM_WIN32(S_FALSE);
    if (m_managementThread->native_handle() == nullptr)
        return HRESULT_FROM_WIN32(S_FALSE);
    if (SetThreadAffinityMask(m_managementThread->native_handle(), affinity) == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
/// \brief QueueManagementProc
/// \details DirectStorageFactory::QueueManagementProc
/// \details Worker thread entry point
//////////////////////////////////////////////////////////////////////////
void DirectStorageFactory::QueueManagementProc()
{
    while (m_threadMode != ManagementThreadControlValues::SHUTDOWN)
    {
        if (m_threadMode == ManagementThreadControlValues::PROCESS_QUEUES)
        {
            ProcessSubmissionQueues();
        }

        if (m_pendingOverlap.size() == 0)
            WaitForSingleObject(m_kickThreadEvent, c_managementThreadSleepTimeMS);
    }
}

//////////////////////////////////////////////////////////////////////////
/// \brief ProcessQueues
/// \details DirectStorageFactory::ProcessQueues
/// \details Check for completed read requests, iterate over the queues based on priority submitting new requests if able
//////////////////////////////////////////////////////////////////////////
void DirectStorageFactory::ProcessSubmissionQueues()
{
    if (m_queues.size() == 0)
        return;

    if (m_openFiles.size() == 0)
        return;
    // Query all current Win32 read requests to see if any have completed
    CheckAsyncRead();

    if (m_pendingOverlap.size() >= c_maxAsyncRequestsInFlight)
        return;

    std::lock_guard<ATG::CriticalSectionLockable> lock(m_threadMutex);

    while (m_pendingOverlap.size() < c_maxAsyncRequestsInFlight)
    {
        size_t submissionQueue = FindNextSubmissionQueue();
        if (submissionQueue == SIZE_MAX)
            return;

        switch (m_queues[submissionQueue]->SubmitNextRequest())
        {
        case DirectStorageQueue::SubmissionResult::RESULT_SUCCESS:
            m_requestsSubmitted++;
            m_lastQueueSubmitted = submissionQueue;
            break;
        case DirectStorageQueue::SubmissionResult::RESULT_NOTHING_TO_SUBMIT:
            m_lastQueueSubmitted = submissionQueue;
            break;
        case  DirectStorageQueue::SubmissionResult::RESULT_WAITING_ON_MEMORY:
            return;		// explicitly return and don't update any processed queues so this queue doesn't lose its place in line just because the staging buffer heap is empty
                        // as soon as any current requests finish memory will become available and new requests can be submitted from this point
        default:
            assert(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/// \brief FindNextSubmissionQueue
/// \details DirectStorageFactory::FindNextSubmissionQueue
/// \details Iterate over the active queues looking for one that could possibly submit a new request
/// \details Handles searching based on queue priority order
//////////////////////////////////////////////////////////////////////////
size_t DirectStorageFactory::FindNextSubmissionQueue()
{
    if (m_queues.size() == 0)
        return SIZE_MAX;

    int32_t checkPriority;
    // check in reverse priority order since we're checking against the mod of the total number of submitted requests
    // otherwise a high priority queue will ALWAYS preempt any lower priority queues
    for (checkPriority = DSTORAGE_PRIORITY_COUNT - 1; checkPriority > DSTORAGE_PRIORITY_FIRST - DSTORAGE_PRIORITY_FIRST; --checkPriority)
    {
        if ((m_requestsSubmitted % m_priorityThreshold[checkPriority]) == 0)
            break;
    }

    int32_t startPriority = checkPriority;
    do
    {
        // check all queues at this priority level for something to submit
        for (size_t checkQueue = 0; checkQueue < m_queues.size(); ++checkQueue)
        {
            size_t queueIndex = (m_lastQueueSubmitted + checkQueue + 1ULL) % m_queues.size();
            if (((m_queues[queueIndex]->Priority() - DSTORAGE_PRIORITY_FIRST) == checkPriority) && (m_queues[queueIndex]->AnyRequestsWaitingForRead()))
                return queueIndex;
        }

        // nothing at this priority so check at the next priority level as a circular level going lower first
        checkPriority++;
        if (checkPriority >= DSTORAGE_PRIORITY_COUNT)
            checkPriority = 0;
    } while (checkPriority != startPriority);		// did we check against all the valid priority levels, if so there is nothing to submit

    return SIZE_MAX;
}

//////////////////////////////////////////////////////////////////////////
/// \brief QueueManagementProc
/// \details DirectStorageFactory::QueueManagementProc
/// \details Worker thread entry point
//////////////////////////////////////////////////////////////////////////
void DirectStorageFactory::DecompressionProc()
{
    while (m_threadMode != ManagementThreadControlValues::SHUTDOWN)
    {
        if (m_threadMode == ManagementThreadControlValues::PROCESS_QUEUES)
        {
            ProcessDecompressionQueues();
        }

        WaitForSingleObject(m_kickDecompressionEvent, c_decompressionThreadSleepTimeMS);
    }
}

//////////////////////////////////////////////////////////////////////////
/// \brief ProcessQueues
/// \details DirectStorageFactory::ProcessQueues
/// \details Check for completed read requests, iterate over the queues based on priority submitting new requests if able
//////////////////////////////////////////////////////////////////////////
void DirectStorageFactory::ProcessDecompressionQueues()
{
    if (m_queues.size() == 0)
        return;

    if (m_openFiles.size() == 0)
        return;
    while (true)
    {
        size_t submissionQueue = FindNextDecompressionQueue();
        if (submissionQueue == SIZE_MAX)
            return;

        switch (m_queues[submissionQueue]->DecompressNextRequest())
        {
        case DirectStorageQueue::DecompressionResult::RESULT_SUCCESS:
            m_requestsDecompressed++;
            m_lastQueueDecompressed = submissionQueue;
            break;
        case DirectStorageQueue::DecompressionResult::RESULT_NOTHING_TO_DECOMPRESS:
            m_lastQueueDecompressed = submissionQueue;
            // This case can happen if another decompression thread happens to get the request that FindNextDecompressionQueue returned
            // Just count it as if we had decompressed a request so we can't sit in a really long spin loop
            break;
        default:
            assert(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/// \brief FindNextDecompressionQueue
/// \details DirectStorageFactory::FindNextDecompressionQueue
/// \details Iterate over the active queues looking for one that could possibly submit a new request
/// \details Handles searching based on queue priority order
//////////////////////////////////////////////////////////////////////////
size_t DirectStorageFactory::FindNextDecompressionQueue()
{
    std::lock_guard<ATG::CriticalSectionLockable> lock(m_threadMutex);
    if (m_queues.size() == 0)
        return SIZE_MAX;

    int32_t checkPriority;
    // check in reverse priority order since we're checking against the mod of the total number of submitted requests
    // otherwise a high priority queue will ALWAYS preempt any lower priority queues
    for (checkPriority = DSTORAGE_PRIORITY_COUNT - 1; checkPriority > DSTORAGE_PRIORITY_FIRST - DSTORAGE_PRIORITY_FIRST; --checkPriority)
    {
        if ((m_requestsDecompressed % m_priorityThreshold[checkPriority]) == 0)
            break;
    }

    int32_t startPriority = checkPriority;
    do
    {
        // check all queues at this priority level for something to submit
        for (size_t checkQueue = 0; checkQueue < m_queues.size(); ++checkQueue)
        {
            size_t queueIndex = (m_lastQueueDecompressed + checkQueue + 1ULL) % m_queues.size();
            if (((m_queues[queueIndex]->Priority() - DSTORAGE_PRIORITY_FIRST) == checkPriority) && (m_queues[queueIndex]->AnyRequestsWaitingForDecompression()))
                return queueIndex;
        }

        // nothing at this priority so check at the next priority level as a circular level going lower first
        checkPriority++;
        if (checkPriority >= DSTORAGE_PRIORITY_COUNT)
            checkPriority = 0;
    } while (checkPriority != startPriority);		// did we check against all the valid priority levels, if so there is nothing to submit

    return SIZE_MAX;
}
//////////////////////////////////////////////////////////////////////////
/// \brief AnyOpenRequests
/// \details DirectStorageFactory::AnyOpenRequests
/// \details Brute force method checking if there is outstanding work for the factory to perform
//////////////////////////////////////////////////////////////////////////
bool DirectStorageFactory::AnyOpenRequests()
{
    if (m_pendingOverlap.size() != 0)
        return true;

    for (auto& iter : m_queues)
    {
        if (iter->NumberPending() != 0)
            return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
/// \brief OpenFile
/// \details DirectStorageFactory::OpenFile
/// \details Open a new Win32 file and return the HANDLE for the file
//////////////////////////////////////////////////////////////////////////
HANDLE DirectStorageFactory::OpenFile(const std::wstring& fileName, HRESULT& errorResult)
{
    errorResult = S_OK;
    uint32_t attributeFlags = FILE_ATTRIBUTE_NORMAL;

    // Files are always opened for overlapped reads
    uint32_t fileFlags = FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED;

    CREATEFILE2_EXTENDED_PARAMETERS params;
    memset(&params, 0, sizeof(params));

    params.dwSize = sizeof(params);
    params.dwFileAttributes = attributeFlags;
    params.dwFileFlags = fileFlags;

    // Files are always opened for reading only, but allow sharing of reads
    HANDLE toret = CreateFile2(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &params);
    if (toret == INVALID_HANDLE_VALUE)
    {
        errorResult = GetLastError();
        return toret;
    }

    return toret;
}

//////////////////////////////////////////////////////////////////////////
/// \brief GetFileSize
/// \details DirectStorageFactory::GetFileSize
/// \details Helper function to convert Win32 file attributes on file size to a 64-bit value
//////////////////////////////////////////////////////////////////////////
size_t DirectStorageFactory::GetFileSize(HANDLE file) const
{
    size_t toret(0);

    auto entry = FindOpenFileByHandle(file);
    if (entry != m_openFiles.end())
    {
        toret = entry->fileAttributes.nFileSizeHigh;
        toret <<= 32ULL;
        toret += entry->fileAttributes.nFileSizeLow;
    }
    return toret;
}

//////////////////////////////////////////////////////////////////////////
/// \brief GetFileName
/// \details DirectStorageFactory::GetFileName
/// \details Convert from a file HANDLE to its name using existing open files
//////////////////////////////////////////////////////////////////////////
const std::wstring& DirectStorageFactory::GetFileName(HANDLE file) const
{
    auto entry = FindOpenFileByHandle(file);
    if (entry != m_openFiles.end())
    {
        return entry->fileName;
    }
    return s_nullFileName;
}

//////////////////////////////////////////////////////////////////////////
/// \brief AsyncRead
/// \details DirectStorageFactory::AsyncRead
/// \details Called by DirectStorageQueue to perform an actual read of the Win32 file
//////////////////////////////////////////////////////////////////////////
HRESULT DirectStorageFactory::AsyncRead(HANDLE file, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t userData, AsyncCallback callback)
{
    DWORD errorCode;

    if (file == INVALID_HANDLE_VALUE)
        return E_UNEXPECTED;

    // all asynchronous operations require a unique OVERLAPPED structure that is valid during the entire read
    OVERLAPPED* winData = new OVERLAPPED;
    if (winData == nullptr)
        return E_OUTOFMEMORY;
    memset(winData, 0, sizeof(OVERLAPPED));
    winData->Pointer = reinterpret_cast<void*> (readLocation);

    // Attempt to start the asynchronous read operation.
    // In some cases this may return immediately with the actual data already available
    if (!ReadFile(file, buffer, bytesToRead, nullptr, winData))
    {
        errorCode = GetLastError();
        // if the OS actually starts an asynchronous operation then the error ERROR_IO_PENDING will be returned
        // This is not an actual error even though ReadFile returned false
        if (errorCode != ERROR_IO_PENDING)
        {
            DebugBreak();
            delete winData;
            return HRESULT_FROM_WIN32(errorCode);
        }
        m_pendingOverlap.emplace_back(PendingOverlapData(this, callback, userData, winData));
        return S_OK;
    }
    else	// asynchronous call to ReadFile was converted to a synchronous call by the OS, data is available now
    {		// result will be picked up on the Check call later by the user. Want to avoid calling the callback here, always treat as asynchronous
        m_pendingOverlap.emplace_back(PendingOverlapData(this, callback, userData, nullptr));
        delete winData;
    }
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
/// \brief CheckAsyncRead
/// \details DirectStorageFactory::CheckAsyncRead
/// \details Iterate over all existing read operations checking for completion
/// \details For any completed read operations call the matching callback function
//////////////////////////////////////////////////////////////////////////
void DirectStorageFactory::CheckAsyncRead()
{
    DWORD bytesRead;

    for (auto iter = m_pendingOverlap.begin(); iter != m_pendingOverlap.end(); )
    {
        assert(iter->callback != nullptr);

        if (iter->pending == nullptr)
        {
            iter->callback(S_OK, iter->userData);
            iter = m_pendingOverlap.erase(iter);
        }
        else if (HasOverlappedIoCompleted(iter->pending))
        {
            if (!GetOverlappedResultEx(iter->file, iter->pending, &bytesRead, 0, FALSE))
            {
                uint32_t errorCode = GetLastError();
                DWORD lastError = GetLastError();
                // GetOverlappedResult will only return true if the read is complete.
                // ERROR_IO_PENDING is not a true error
                if (lastError != ERROR_IO_PENDING)
                {
                    iter->callback(HRESULT_FROM_WIN32(errorCode), iter->userData);
                    delete iter->pending;
                    iter = m_pendingOverlap.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
            else
            {
                iter->callback(S_OK, iter->userData);
                delete iter->pending;
                iter = m_pendingOverlap.erase(iter);
            }
        }
        else  // read has not signaled completing yet
        {
            ++iter;
        }
    }
}
