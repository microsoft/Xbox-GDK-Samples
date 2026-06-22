//--------------------------------------------------------------------------------------
// File.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <winapifamily.h>
#include "lockLessPool.h"

#define THREAD_SAFE
//#define USE_COMPLETION_PORTS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifdef THREAD_SAFE
#include <OSLockable.h>
#endif

namespace FileSystem
{
    //////////////////////////////////////////////////////////////////////////
    /// \brief File
    /// \details Interface to a file
    /// \details Contains interfaces for synchronous or asynchronous access
    /// \details Contains static members that act as a singleton "FileManager"
    /// \details There is no thread safety in this class
    //////////////////////////////////////////////////////////////////////////
    class File
    {
        // static interfaces that act like a singleton "FileManager"
    public:
        typedef uint64_t AsyncRequestId;
        typedef std::function<void(HRESULT, AsyncRequestId, uint64_t)> AsyncCallback;

    private:
#ifdef THREAD_SAFE
        static ATG::CriticalSectionLockable s_threadMutex;
#endif
        // Constantly incrementing counter used as asynchronous read request index
        static std::atomic<uint64_t> s_asyncReadIndex;

        static std::atomic<uint32_t> s_refCount;

        typedef std::vector<File*> OpenFileList;
        // Internal list of all opened files, used by internal singleton FileManager
        static OpenFileList* s_openFiles;

        static HANDLE s_ioCompletionPort;

        typedef std::vector<HANDLE> EventCacheList;
        // Internal cache of OS Event objects, max size is c_eventCacheInitialSize
        static EventCacheList* s_eventCache;
        static const uint32_t c_minEventCacheInitialSize = 100;
        static const uint32_t c_maxEventCacheInitialSize = 2048;
        static std::atomic<uint32_t> s_eventCacheInitialSize;

        //////////////////////////////////////////////////////////////////////////
        /// \brief PendingOverlapData
        /// \details Reference to a pending asynchronous operation
        /// \details The internal FileManager keeps a list of all pending asynchronous operations
        /// \details Calls to various Wait function below will query this list to find the matching data
        //////////////////////////////////////////////////////////////////////////
        struct PendingOverlapData
        {
#ifdef USE_COMPLETION_PORTS
            HANDLE completeEvent;
#endif
            OVERLAPPED* pending;			// note the attached Event may be INVALID_HANDLE
            File* owner;
            AsyncCallback callback;
            uintptr_t userData = 0;
            AsyncRequestId requestId;
            PendingOverlapData()
            {
                owner = nullptr;
                callback = nullptr;
                userData = 0;
                requestId = 0;
                pending = nullptr;
            }
            PendingOverlapData(File* p1, AsyncCallback p2, uintptr_t p3, AsyncRequestId p4, OVERLAPPED* p5)
            {
                assert(p2 != nullptr);
                owner = p1;
                callback = p2;
                userData = p3;
                requestId = p4;
                pending = p5;
            }
            ~PendingOverlapData()
            {
                if (pending)
                {
                    if (pending->hEvent == nullptr)
                        ReleaseEvent(pending->hEvent);
                    s_overlappedAllocator.deallocate(pending);
                }
            }
            // We need a move operator to avoid the underlying OS Event from getting released on destruction
            PendingOverlapData(PendingOverlapData&& p1) noexcept
            {
                pending = p1.pending;
                owner = p1.owner;
                callback = p1.callback;
                requestId = p1.requestId;
                pending = nullptr;
            }
        };
        typedef std::vector<PendingOverlapData*> PendingOverlapList;
        // Internal list of all pending asynchronous operations, used by internal singleton FileManager
        static PendingOverlapList* s_masterPendingOverlap;

        static ATG::LockLessMemoryPool<PendingOverlapData, 1536> s_pendingOverlapAllocator;
        static ATG::LockLessMemoryPool<OVERLAPPED, 1536> s_overlappedAllocator;

        //////////////////////////////////////////////////////////////////////////
        /// \brief FileClosed
        /// \details FileSystem::File::FileClosed
        /// \details Tells the underlying FileManager data that a file has been closed
        /// \param file Which file has been closed
        //////////////////////////////////////////////////////////////////////////
        static void FileClosed(File* file);

        //////////////////////////////////////////////////////////////////////////
        /// \brief RequestEvent
        /// \details FileSystem::File::RequestEvent
        /// \details The internal FileManager object contains a cache of OS Events
        /// \details These events are used for asynchronous operations
        /// \return OS Event, either created or from the cache
        //////////////////////////////////////////////////////////////////////////
        static HANDLE RequestEvent(void);

        //////////////////////////////////////////////////////////////////////////
        /// \brief ReleaseEvent
        /// \details FileSystem::File::ReleaseEvent
        /// \details Returns an OS Event back into the event cache
        /// \param event The OS Event object
        //////////////////////////////////////////////////////////////////////////
        static void ReleaseEvent(HANDLE event);

    public:

        //////////////////////////////////////////////////////////////////////////
        /// \brief AsyncRequestData
        /// \details Represents one pending asynchronous request
        /// \details This structure format is public for a pending TODO that allows the user to submit multiple requests at the same time
        //////////////////////////////////////////////////////////////////////////
        struct AsyncRequestData
        {
            void* buffer;
            uint32_t bytesToRead;
            uint64_t position;
            bool useEvent;
            AsyncRequestData(void* p1, uint32_t p2, uint64_t p3, bool p4)
            {
                buffer = p1;
                bytesToRead = p2;
                position = p3;
                useEvent = p4;
            }
        };
        typedef std::vector<AsyncRequestData> AsyncRequestList;

        //////////////////////////////////////////////////////////////////////////
        /// \brief OpenFile
        /// \details Values to use as a bit mask into OpenFile and the creationFlags parameter
        //////////////////////////////////////////////////////////////////////////
        enum OpenFlags : uint32_t
        {
            e_openFlag_Preload = 0x01,
            e_openFlag_NoBuffering = 0x02,
            e_openFlag_EnableAsync = 0x04,
        };

        //////////////////////////////////////////////////////////////////////////
        /// \brief OpenFile
        /// \details FileSystem::File::OpenFile
        /// \details Asks the internal FileManager to open a file for reading
        /// \return The File object representing the newly opened file
        /// \param fileName The name of the file to open
        /// \param creationDisposition The dwCreationDisposition flags that are passed into CreateFile
        /// \param creationFlags bit mask of OpenFlags enum for opening the file fully cached or for asynchronous operations, some flags are mutually exclusive
        //////////////////////////////////////////////////////////////////////////
        static File* OpenFile(const std::wstring& fileName, HRESULT& errorResult, uint32_t creationDisposition, uint32_t creationFlags = 0);
        static File* OpenFile(const std::string& fileName, HRESULT& errorResult, uint32_t creationDisposition, uint32_t creationFlags = 0);

        //////////////////////////////////////////////////////////////////////////
        /// \brief OpenFileForWriting
        /// \details FileSystem::File::OpenFileForWriting
        /// \details Asks the internal FileManager to open a file for writing
        /// \details All files opened for writing are opened for synchronous operation
        /// \return The File object representing the newly opened file
        /// \param fileName The name of the file to open
        //////////////////////////////////////////////////////////////////////////
        static File* OpenFileForWriting(const std::wstring& fileName, HRESULT& errorResult);

        //////////////////////////////////////////////////////////////////////////
        /// \brief CloseFile
        /// \details FileSystem::File::CloseFile
        /// \details Asks the internal FileManager to close a file
        /// \param file The file to close
        //////////////////////////////////////////////////////////////////////////
        static void CloseFile(File* file);

        //////////////////////////////////////////////////////////////////////////
        /// \brief InitFileSystem
        /// \details FileSystem::File::InitFileSystem
        /// \details Initialize the internal FileManager object.
        /// \details No memory is allocated before this function is called
        /// \details This function is called automatically if needed
        /// \note This function will fill the event cache, which could take a bit of time
        //////////////////////////////////////////////////////////////////////////
        static void InitFileSystem(bool useCompletionPorts, uint32_t initialEventCount);

        //////////////////////////////////////////////////////////////////////////
        /// \brief ShutdownFileSystem
        /// \details FileSystem::File::ShutdownFileSystem
        /// \details Clean up the internal FileManager object
        /// \details The user is required to call this function when done with all file operations
        //////////////////////////////////////////////////////////////////////////
        static void ShutdownFileSystem(void);

        //////////////////////////////////////////////////////////////////////////
        /// \brief CheckForCompleteAsync
        /// \details FileSystem::File::CheckForCompleteAsync
        /// \details Iterate over all current asynchronous requests in flight and check for completion
        /// \details If the request has finished then call the associated callback function
        //////////////////////////////////////////////////////////////////////////
        static void CheckForCompleteAsync(void);

        //////////////////////////////////////////////////////////////////////////
        /// \brief DummyCallbackFunction
        /// \details FileSystem::File::DummyCallbackFunction
        /// \details Helper function to use when no callback function is needed
        //////////////////////////////////////////////////////////////////////////
        static void DummyCallbackFunction(HRESULT, FileSystem::File::AsyncRequestId, uintptr_t) {}

        // Interfaces that reference the File object that is an instance of a single file on disk
    private:

        // All constructors are private.  The user needs to go through the static Open/Create functions
        File();
        File(const File&);
        File(const std::wstring& fileName, uint32_t creationDisposition, uint32_t creationFlags);
        File& operator= (const File&);

        HRESULT InternalOpenFile(void);
        HRESULT InternalCreateFile(void);

        std::wstring m_fileName;
        uint32_t m_creationDisposition;
        uint32_t m_creationFlags;
        uint64_t m_fileSize = 0;
        uint8_t* m_preloadBuffer;
        HANDLE m_fileHandle;
        size_t m_numPages = 0;
        uint64_t m_filePointer = 0;
        WIN32_FILE_ATTRIBUTE_DATA m_fileAttributes = {};		// only valid for read-only files

        //////////////////////////////////////////////////////////////////////////
        /// \brief StartInternalAsyncRead
        /// \details FileSystem::File::StartInternalAsyncRead
        /// \details Internal function to perform the actual read operation from disk
        /// \details This is a helper function into the other StartInternalAsyncRead that takes a vector of requests
        /// \return ERROR_SUCCESS (0) on success, other OS error from underlying system
        //////////////////////////////////////////////////////////////////////////
        HRESULT StartInternalAsyncRead(void* buffer, uint32_t bytesToRead, uint64_t position, AsyncRequestId& requestId, AsyncCallback callback, uintptr_t userData, bool useEvent);

        //////////////////////////////////////////////////////////////////////////
        /// \brief StartInternalAsyncRead
        /// \details FileSystem::File::StartInternalAsyncRead
        /// \details Internal function to perform the actual read operation from disk
        /// \details The system handles requests as a list of several.  This is for a TODO to allow the user to create multiple requests
        /// \details requests for data at the same time.  For instance grouping sequential locations together
        /// \return ERROR_SUCCESS (0) on success, other OS error from underlying system
        //////////////////////////////////////////////////////////////////////////
        HRESULT StartInternalAsyncRead(const AsyncRequestList& list, AsyncRequestId& requestId, AsyncCallback callback, uintptr_t userData);

    public:
        virtual ~File();

        const std::wstring& GetFileName() const { return m_fileName; }
        const WIN32_FILE_ATTRIBUTE_DATA& GetWin32FileAttributes() const { return m_fileAttributes; }

        uint64_t GetFileSize() const { return m_fileSize; }

        //////////////////////////////////////////////////////////////////////////
        /// \brief Preload
        /// \details FileSystem::File::Preload
        /// \details Preloads the entire contents of the file into memory as a synchronous operation
        /// \details All read requests after this call are converted into a memcpy
        /// \details Memory is allocated directly through VirtualAlloc
        /// \return ERROR_SUCCESS (0) on success, other OS error from underlying system
        //////////////////////////////////////////////////////////////////////////
        HRESULT Preload(void);		// load the file into memory

                                    //////////////////////////////////////////////////////////////////////////
                                    /// \brief Unload
                                    /// \details FileSystem::File::Unload
                                    /// \details Free up the memory used from preloading a file, operations after this go to disk
                                    /// \return ERROR_SUCCESS (0) on success, other OS error from underlying system
                                    //////////////////////////////////////////////////////////////////////////
        HRESULT Unload(void);		// clear the file from memory

                                    //////////////////////////////////////////////////////////////////////////
                                    /// \brief SetReadPointer
                                    /// \details FileSystem::File::SetReadPointer
                                    /// \details Sets the location for the next call to Read, end of file if out of bounds
                                    /// \return The new file location, SIZE_MAX if the file is not open
                                    /// \param newLocation Where to move the file pointer
                                    //////////////////////////////////////////////////////////////////////////
        size_t SetReadPointer(uint64_t newLocation);

        //////////////////////////////////////////////////////////////////////////
        /// \brief GetReadPointer
        /// \details FileSystem::File::GetReadPointer
        /// \details Get the current file pointer for read operations
        /// \return The current read location
        //////////////////////////////////////////////////////////////////////////
        size_t GetReadPointer(void) const { return m_filePointer; }

        //////////////////////////////////////////////////////////////////////////
        /// \brief Read
        /// \details FileSystem::File::Read
        /// \details Perform a blocking read from the file
        /// \details If the file is open for asynchronous reads the read internally will use OVERLAPPED I/O
        /// \details However the function will wait for that read to complete.  Other reads may complete in the meantime
        /// \return Any error from the internal ReadFile call, ERROR_SUCCESS (0) on success
        /// \param buffer Where to place the data, buffer must remain valid through the entire call
        /// \param bytesToRead How much data to read from the file
        //////////////////////////////////////////////////////////////////////////
        HRESULT Read(void* buffer, uint32_t bytesToRead);

        //////////////////////////////////////////////////////////////////////////
        /// \brief AsyncRead
        /// \details FileSystem::File::AsyncRead
        /// \details Perform a read from the file.  This function will be synchronous if the file was not opened for asynchronous operation
        /// \details If the file is open for asynchronous operation then all read sizes must be aligned to a 4k, buffer needs to be 8 byte aligned
        /// \return Any error from the internal ReadFile call, ERROR_SUCCESS (0) on success
        /// \param buffer Where to place the data, buffer must remain valid through the entire read operation
        /// \param bytesToRead How much data to read from the file
        /// \param readLocation The location on disk to read the file
        /// \param userData A unique value to pass into the callback function when read operation is done
        /// \param[out] requestId Where to place the id of this asynchronous operation, UINT64_MAX if the operation completed and no error
        /// \param callback The callback function for when the asynchronous operation completes
        //////////////////////////////////////////////////////////////////////////
        HRESULT AsyncRead(void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t userData, AsyncRequestId& requestId, AsyncCallback callback = DummyCallbackFunction, bool useEvent = true);

        //////////////////////////////////////////////////////////////////////////
        /// \brief Write
        /// \details FileSystem::File::Write
        /// \details Outputs data to a file opened for writing
        /// \return ERROR_SUCCESS (0) on success, other OS error from underlying system
        /// \param buffer Data to output to the file
        /// \param bytesToWrite Size of the data to output
        //////////////////////////////////////////////////////////////////////////
        HRESULT Write(void* buffer, uint32_t bytesToWrite, uint32_t* bytesActuallyWritten = nullptr);

        //////////////////////////////////////////////////////////////////////////
        /// \brief WaitAsyncRead
        /// \details FileSystem::File::WaitAsyncRead
        /// \details Wait for a specific pending asynchronous operation to complete
        /// \details Will only wait if there is an associated Event, will return immediately in other cases (but still check for completion)
        /// \details Other reads may complete during this time, but they will not be signaled until requested through this function or CheckForCompleteAsync
        /// \return ERROR_SUCCESS (0) on success, WAIT_TIMEOUT if did not complete in time, other OS error from underlying system
        /// \param asyncId The id returned by the call to AsyncRead
        /// \param shouldWait The function should wait until the read operation has completed
        /// \param issueCallback Whether the matching callback function should be called if operation is complete
        //////////////////////////////////////////////////////////////////////////
        static HRESULT WaitAsyncRead(AsyncRequestId asyncId, bool shouldWait);

        //////////////////////////////////////////////////////////////////////////
        /// \brief WaitAnyAsyncRead
        /// \details FileSystem::File::WaitAnyAsyncRead
        /// \details Wait for any pending asynchronous operations to complete
        /// \details There is support for only the first 64 due to WaitForMultipleObjects limitations
        /// \details On completion the callback will be called
        /// \details Only async reads that have associated Events are handled by this call
        /// \return ERROR_SUCCESS (0) on success, WAIT_TIMEOUT if did not complete in time, other OS error from underlying system
        /// \param shouldWait Should the function wait until at least one call has completed
        //////////////////////////////////////////////////////////////////////////
        static HRESULT WaitAnyAsyncRead(bool shouldWait);

        //////////////////////////////////////////////////////////////////////////
        /// \brief CheckAnyAsyncRead
        /// \details FileSystem::File::CheckAnyAsyncRead
        /// \details Checks any pending asynchronous operations to complete
        /// \details This function does not use the attached events to check
        /// \details This function will call the associated callback for all reads that are complete
        /// \return ERROR_SUCCESS (0) on success, anything else on error
        //////////////////////////////////////////////////////////////////////////
        static HRESULT CheckAnyAsyncRead();

        //////////////////////////////////////////////////////////////////////////
        /// \brief WaitAnyAsyncRead
        /// \details FileSystem::File::WaitAnyAsyncRead
        /// \details Wait for any pending asynchronous operations to complete
        /// \details There is support for only the first 64 due to WaitForMultipleObjects limitations
        /// \details On completion the callback will be called
        /// \details Only async reads that have associated Events are handled by this call
        /// \return ERROR_SUCCESS (0) on success, WAIT_TIMEOUT if did not complete in time, other OS error from underlying system
        /// \param shouldWait Should the function wait until at least one call has completed
        //////////////////////////////////////////////////////////////////////////
        static HRESULT WaitCompletionPortRead(bool shouldWait);
    };
}
