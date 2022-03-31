//--------------------------------------------------------------------------------------
// DirectStorageWin32Wrapper.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <OSLockable.h>
#include "dstorage_win32.h"

#pragma warning(push)
#pragma warning(disable:4201)   // nonstandard extension used : nameless struct/union

// This is an implementation of the console DirectStorage API set that uses only standard Win32 calls
// This implementation is only used for the Xbox One family consoles which does not have native support for DirectStorage
// Titles are free to use these interfaces if desired, but it's strongly recommended to use the native interfaces which will have much better performance and a richer feature set

namespace DirectStorageWin32Wrapper
{
    //////////////////////////////////////////////////////////////////////////
    /// \brief DirectStorageWin32Wrapper::Internal
    /// \details Each DirectStorage interface has a matching class within DirectStorageWin32Wrapper
    /// \details All of these classes can be considered internal, title code does not need access
    /// \details Interaction only needs to be done through the original DirectStorage interfaces
    //////////////////////////////////////////////////////////////////////////
    namespace Internal
    {
        //////////////////////////////////////////////////////////////////////////
        /// \brief DirectStorageFile
        /// \details Internal container for IDStorageFileWin32 data needs
        //////////////////////////////////////////////////////////////////////////
        class DirectStorageFile final : public IDStorageFileWin32
        {
            friend class DirectStorageFactory;
        private:
            std::atomic<uint32_t> m_refCount;								// DirectStorageFile is a reference counted object
            HANDLE m_file;													// The factory actually owns this pointer

            DirectStorageFile(HANDLE file) : m_refCount(0), m_file(file) {}
            DirectStorageFile() = delete;
            ~DirectStorageFile();

        public:
            HANDLE GetHandle() override;
            void Close() override;

            HANDLE GetRawHandle() { return m_file; }

        public:

            // Default IUnknown interfaces
            HRESULT STDMETHODCALLTYPE QueryInterface(/* [in] */ REFIID riid,/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;
            ULONG STDMETHODCALLTYPE AddRef(void) override;
            ULONG STDMETHODCALLTYPE Release(void) override;
        };

        //////////////////////////////////////////////////////////////////////////
        /// \brief DirectStorageStatusArray
        /// \details Internal representation of an IDStorageStatusArrayWin32
        //////////////////////////////////////////////////////////////////////////
        class DirectStorageStatusArray final : public IDStorageStatusArrayWin32
        {
            friend class DirectStorageFactory;
        private:
            std::vector<HRESULT> m_resultCodes;								// default is S_OK, set to first error that occurs since the last status field
            std::vector<bool> m_completeFlags;								// value is undefined until a particular index is placed within a queue
            std::atomic<uint32_t> m_refCount;								// DirectStorageStatusArray is a reference counted object
            std::string m_name;

            DirectStorageStatusArray(uint32_t capacity, const std::string& name);
            ~DirectStorageStatusArray() {}

        public:
            bool IsComplete(UINT32 index) override;
            HRESULT GetHResult(UINT32 index) override;

            void MarkState(uint32_t index, bool complete, HRESULT error);	// Internal only function, not part of IDStorageStatusArrayWin32

        public:

            // Default IUnknown interfaces
            HRESULT STDMETHODCALLTYPE QueryInterface(/* [in] */ REFIID riid,/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;
            ULONG STDMETHODCALLTYPE AddRef(void) override;
            ULONG STDMETHODCALLTYPE Release(void) override;
        };

        //////////////////////////////////////////////////////////////////////////
        /// \brief DirectStorageQueue
        /// \details Internal representation of an IDStorageQueueWin32
        /// \details Most work is done within this class
        /// \details Handles creating read requests and marking completion of status or fence events
        //////////////////////////////////////////////////////////////////////////
        class DirectStorageQueue final : public IDStorageQueueWin32
        {
            friend class DirectStorageFactory;
        public:
            enum class SubmissionResult
            {
                RESULT_SUCCESS,
                RESULT_NOTHING_TO_SUBMIT,
                RESULT_WAITING_ON_MEMORY,
            };

            enum class DecompressionResult
            {
                RESULT_SUCCESS,
                RESULT_NOTHING_TO_DECOMPRESS,
            };

        private:
            static constexpr uint32_t c_fileAlignment = 4096;

            enum class State
            {
                STATE_PENDING,			// request has been enqueued but has not been marked from a call to Submit
                STATE_SUBMITTED,		// request is valid for the actual read from the disk, marked from a call to Submit
                STATE_READING,			// request has been passed off to the file system
                STATE_READY_DECOMPRESS,  // request is ready for decompressed
                STATE_DECOMPRESSING,    // request is being decompressed
                STATE_ERROR,			// request failed due to an error
                STATE_FINISHED,			// request has finished, however there are still pending requests before a following status/fence can be signaled
                STATE_BLANK,			// slot is empty and ready for a new request
                STATE_CANCELLED,		// request has been canceled, however there are still pending requests before a following status/fence can be signaled
            };

            //////////////////////////////////////////////////////////////////////////
            /// \brief QueueEntry
            /// \details Maintains data for a single entry in the queue
            /// \details Can be either a read requests, status block, or a fence
            //////////////////////////////////////////////////////////////////////////
            struct QueueEntry
            {
            private:
                State		currentState;       // made private to help with error detection of bugs

            public:
                enum class EntryType
                {
                    REQUEST_ENTRY,
                    STATUS_ENTRY,
                    FENCE_ENTRY,
                };

                union
                {
                    DSTORAGE_REQUEST request;
                    struct
                    {
                        DirectStorageStatusArray* statusArray;
                        uint32_t statusIndex;
                    };
                    struct
                    {
                        ID3D12Fence* fence;
                        uint64_t fenceValue;
                    };
                };

                EntryType	entryType;
                size_t stagingBufferSize;	// all read requests are first read into a staging buffer, this handles unaligned reads as well as decompression
                void* stagingBuffer;		// TODO: Consider removing this buffer if the request is aligned and not being decompressed
                void* stagingBuffer2;       // only used for data compressed with both zlib and bcpack

                bool stagingBufferUsed;

                QueueEntry() :
                    currentState(State::STATE_BLANK),
                    request{},
                    entryType{},
                    stagingBufferSize(0),
                    stagingBuffer(nullptr),
                    stagingBuffer2(nullptr),
                    stagingBufferUsed(false) {}
                void SwitchState(const State newState);

                // Helper Functions for conditional checks around aggregated states
                bool IsRequestComplete() const noexcept
                {
                    if ((currentState == State::STATE_ERROR) || (currentState == State::STATE_FINISHED) || (currentState == State::STATE_CANCELLED))
                        return true;
                    if (entryType == EntryType::STATUS_ENTRY)
                        return true;
                    if (entryType == EntryType::FENCE_ENTRY)
                        return true;
                    return false;
                }
                bool IsRequestValidForReading() const noexcept
                {
                    if (IsFenceEntry() || IsStatusEntry())
                        return false;
                    if (currentState != State::STATE_SUBMITTED)
                        return false;
                    return true;
                }
                bool IsFenceEntry() const noexcept { return entryType == EntryType::FENCE_ENTRY; }
                bool IsStatusEntry() const noexcept { return entryType == EntryType::STATUS_ENTRY; }
                bool IsRequestEntry() const noexcept { return entryType == EntryType::REQUEST_ENTRY; }
                bool IsBlank() const noexcept { return currentState == State::STATE_BLANK; }
                bool IsError() const noexcept { return currentState == State::STATE_ERROR; }
                bool IsPending() const noexcept { return currentState == State::STATE_PENDING; }
                bool IsSubmitted() const noexcept { return currentState == State::STATE_SUBMITTED; }
                bool IsReading() const noexcept { return currentState == State::STATE_READING; }
                bool IsCancellable() const noexcept { return (currentState == State::STATE_PENDING) || (currentState == State::STATE_SUBMITTED); }
                bool IsReadyDecompression() const noexcept { return currentState == State::STATE_READY_DECOMPRESS; }
                bool IsCompressed() const noexcept { assert(IsRequestEntry()); return (request.Options.ZlibDecompress) || (request.Options.BcpackMode != 0); }
                bool IsMemoryDecompression() const noexcept { return (entryType == EntryType::REQUEST_ENTRY) && (request.Options.SourceType == DSTORAGE_REQUEST_SOURCE_MEMORY); }
            };
            struct EntryList
            {
            private:
                std::unique_ptr<QueueEntry[]> entries;
                size_t size;
            public:
                EntryList(size_t totalSize) { entries.reset(new QueueEntry[totalSize]()); size = totalSize; }
                QueueEntry& operator[] (uint64_t index) { return entries[index % size]; }
                const QueueEntry& operator[] (uint64_t index) const { return entries[index % size]; }
                operator bool() const { return entries.get() != nullptr; }
                void reset() { entries.reset(); }
            };

            HRESULT m_firstErrorSinceStatusSignal;						// Used to correctly save the error to the following status entry.
            DSTORAGE_ERROR_RECORD m_errorRecord;						// Full details on first error since last request by title for errors
            std::atomic<uint32_t> m_refCount;							// DirectStorageQueue is a reference counted object
            EntryList m_entries;										// Circular array for all requests in the queue, could be null if the queue has been closed
            mutable ATG::CriticalSectionLockable m_criticalSection;		// Thread safety mutex
            std::atomic<uint64_t> m_nextRequestSubmitted;				// next slot that should be submitted, prev was the last slot that was submitted
            std::atomic<uint64_t> m_nextRequestEnqueued;				// next slot to use when enqueuing a request
            std::atomic<uint64_t> m_nextRequestNotComplete;				// all requests before this slot have been marked complete in some form
            std::atomic<uint64_t> m_nextRequestRead;					// which request should be passed to the filesystem
            HANDLE m_errorEvent;										// Automatic reset event signaled when m_errorRecord is updated
            DSTORAGE_QUEUE_DESC m_description;							// Creation details on queue

            void DSCallbackFunction(HRESULT, uintptr_t);				// Callback registered with asynchronous read request

            SubmissionResult SubmitNextRequest();						// Attempt to submit the next request in the queue for reading. Status/Fence objects are skipped
            DecompressionResult DecompressNextRequest();                // perform one decompression task
            void SignalStatusOrFence();									// After a request is complete check if any status/fence entry needs to be signalled

            // Helper functions for checking the status of the queue
            bool AnyRequestsWaitingForRead();
            bool AnyRequestsWaitingForDecompression();
            size_t NumberFreeSlots() const;
            size_t NumberPending() const;

            void MarkRequestCompleted(size_t index);					// after a request is marked as completed checks are made against the next status/fence for marking completed as well
            void MarkRequestError(size_t index, HRESULT errorResult, bool signalStatusOrFence);		// It's possible for an enqueued request to immediately fail, in this case don't check for a following status/fence
                                                                                                    // Technically the requests have not been submitted yet and there is no following status/fence yet

            // Helper functions to update internal index variables to handle circular queue
            // The numbers increase to UINT64_MAX
            // Worse case: assuming 2GB/s using 1 byte reads the number won't wrap for ~272 years
            void IncrementNextRequestSubmitted() { m_nextRequestSubmitted++; }
            void IncrementNextRequestEnqueued() { m_nextRequestEnqueued++; }
            void IncrementNextRequestNotComplete() { m_nextRequestNotComplete++; }
            void IncrementNextRequestRead() { m_nextRequestRead++; }

            size_t LZInflate(void* output, size_t outputSize, const void* input, size_t length);

            DSTORAGE_PRIORITY Priority() const { return m_description.Priority; }
            const char* Name() const { return m_description.Name; }

            DirectStorageQueue(const DSTORAGE_QUEUE_DESC* desc);
            ~DirectStorageQueue();

        public:
            void EnqueueRequest(const DSTORAGE_REQUEST* request) override;
            void EnqueueStatus(IDStorageStatusArrayWin32* statusArray, UINT32 index) override;
            void EnqueueSignal(ID3D12Fence* fence, UINT64 value) override;
            void Submit() override;
            void CancelRequestsWithTag(UINT64 mask, UINT64 value) override;
            void Close() override;
            HANDLE GetErrorEvent() override { return m_errorEvent; }
            void RetrieveErrorRecord(_Out_ DSTORAGE_ERROR_RECORD* record) override;

        public:
            void Query(_Out_ DSTORAGE_QUEUE_INFO* info) override;

            // Default IUnknown interfaces
            HRESULT STDMETHODCALLTYPE QueryInterface(/* [in] */ REFIID riid,/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;
            ULONG STDMETHODCALLTYPE AddRef(void) override;
            ULONG STDMETHODCALLTYPE Release(void) override;
        };

        //////////////////////////////////////////////////////////////////////////
        /// \brief DirectStorageFactory
        /// \details Internal representation of an IDStorageFactoryWin32
        /// \details Singleton object
        /// \details Owner of DirectStorageQueue and DirectStorageFile instances
        /// \details Maintains all pending read requests and manages request priorities for submission to the hardware
        //////////////////////////////////////////////////////////////////////////
        class DirectStorageFactory final : public IDStorageFactoryWin32
        {
        public:
            typedef std::function<void(HRESULT, uint64_t)> AsyncCallback;
            enum class ManagementThreadControlValues
            {
                SHUTDOWN = 0,
                PROCESS_QUEUES = 1,
            };

            static constexpr uint32_t c_realtimePriorityThreshold = 1;			// Total number of requests submitted before submitting one at this priority
            static constexpr uint32_t c_highPriorityThreshold = 1;				// Total number of requests submitted before submitting one at this priority
            static constexpr uint32_t c_mediumPriorityThreshold = 10;			// This is handled as the mod of the total requests submitted and this value is 0
            static constexpr uint32_t c_lowPriorityThreshold = 100;

            static constexpr uint32_t c_maxAsyncRequestsInFlight = 64;			// Not all requests are submitted to Win32 at once, a max of this amount is allowed to be in flight at a time
            static constexpr uint32_t c_managementThreadSleepTimeMS = 2;		// Heartbeat for the management thread, it can be kicked with m_kickThreadEvent
            static constexpr uint32_t c_decompressionThreadSleepTimeMS = 2;		// Heartbeat for the decompression threads
            static constexpr uint32_t c_maxOpenFiles = 5000;					// Used to preallocate the array of open files, TODO: Check if this number is reasonable, could be larger
            static constexpr uint32_t c_maxQueues = 100;						// Used to preallocate the array of queue object, TODO: Check if this number is reasonable, could be larger
            static constexpr uint64_t c_initialStagingBufferSize = 32ULL * 1024 * 1024;	// Staging buffer heap is created initially at this size.
                                                                                    // However the heap can grow beyond this size due to limitiations in Win32 Heap objects
            static constexpr uint32_t c_numDecompressionThreads = 1;

        private:

            //////////////////////////////////////////////////////////////////////////
            /// \brief PendingOverlapData
            /// \details Maintains data related to a single asynchronous ReadFile call
            //////////////////////////////////////////////////////////////////////////
            struct PendingOverlapData
            {
                OVERLAPPED* pending;			// note: not owned by this object, owned by outer DirectStorageFactory
                HANDLE file;					// Which file is being used for the read
                AsyncCallback callback;			// Points back to DirectStorageQueue::DSCallbackFunction, called on completetion of the read, either success or failure
                uintptr_t userData;				// Data passed to callback function on completion. In this case it holds the index in the queue that matches this read request
                PendingOverlapData() :pending(nullptr), file(INVALID_HANDLE_VALUE), callback(nullptr), userData(0) {}
                PendingOverlapData(HANDLE p1, AsyncCallback p2, uintptr_t p3, OVERLAPPED* p4) :pending(p4), file(p1), callback(p2), userData(p3) {}
            };
            typedef std::vector<PendingOverlapData> PendingOverlapList;
            PendingOverlapList m_pendingOverlap;

            //////////////////////////////////////////////////////////////////////////
            /// \brief OpenFileEntry
            /// \details DirectStorageFactory owns all the open file handles with a reference count to know when to close the handle
            //////////////////////////////////////////////////////////////////////////
            struct OpenFileEntry
            {
                WIN32_FILE_ATTRIBUTE_DATA fileAttributes;
                uint64_t nameHash;
                HANDLE file;
                mutable uint32_t refCount;
                std::wstring fileName;
                OpenFileEntry() : nameHash(0), file(nullptr), refCount(0) {}
            };
            HANDLE								m_stagingBuffer;		// A Win32 Heap object that manages the staging buffer used for unaligned and compressed reads
            uint32_t							m_priorityThreshold[DSTORAGE_PRIORITY_COUNT];	// how many requests to process before this level, effectively how many high before this level
            std::atomic<uint32_t>				m_refCount;				// A reference count is still kept on the factory even though it is a singleton
            std::vector<DirectStorageQueue*>	m_queues;
            mutable ATG::CriticalSectionLockable		m_threadMutex;
            std::thread* m_managementThread;
            std::vector<std::thread*> m_decompressionThreads;
            std::atomic<ManagementThreadControlValues> m_threadMode;
            uint64_t							m_requestsSubmitted;	// total requests submitted
            size_t								m_lastQueueSubmitted;	// index of last queue submitted, rotate through queues at certain priority until next priority is needed
            uint64_t							m_requestsDecompressed;	// total requests decompressed
            size_t								m_lastQueueDecompressed;// index of last queue decompressed, rotate through queues at certain priority until next priority is needed
            std::vector<OpenFileEntry>			m_openFiles;			// hash of filename for quick lookup
                                                                        // TODO: Consider converting this to a map using the hash of the filename
            HANDLE								m_kickThreadEvent;		// Event the thread waits on so it can be kicked to wake up early
            HANDLE								m_kickDecompressionEvent;

            std::vector<OpenFileEntry>::const_iterator FindOpenFileByHash(const uint64_t fileNameHash) const;
            std::vector<OpenFileEntry>::const_iterator FindOpenFileByHandle(HANDLE fileHandle) const;

            HANDLE OpenFile(const std::wstring& fileName, HRESULT& errorResult);
            void CheckAsyncRead();										// Iterates over the list of pending reads for completion and notifies the callback if completed

            void QueueManagementProc();									// Entry point for worker thread
            void ProcessSubmissionQueues();								// Check for completed requests and then iterate over the queues in priority order submitting requests for read
            bool AnyOpenRequests();
            size_t FindNextSubmissionQueue();							// Based on priority order find a queue that can submit a request

            void DecompressionProc();
            void ProcessDecompressionQueues();							// Iterate over the queues in priority order decompressing requests
            size_t FindNextDecompressionQueue();						// Based on priority order find a queue that can decompress a request

            ~DirectStorageFactory();

        public:
            DirectStorageFactory();

            // there should be no path where this is valid in the DS on Win32 implementation. A title also cannot call this function
            static DirectStorageFactory* GetInstance();

            void* AllocateStagingMemory(size_t amount) { return HeapAlloc(m_stagingBuffer, 0, amount); }
            void FreeStagingMemory(void* baseAddress) { if (baseAddress != nullptr) HeapFree(m_stagingBuffer, 0, baseAddress); }

            void RemoveQueue(DirectStorageQueue* queue);				// A queue instance manages its own lifetime through a reference count, when it goes to zero it asks the factory instance for cleanup
            void RemoveFile(HANDLE file);								// A file instance manages its own lifetime through a reference count, when it goes to zero it asks the factory instance for cleanup

            size_t GetFileSize(HANDLE file) const;						// Lookup file in the master list and use cached file attributes
            const std::wstring& GetFileName(HANDLE file) const;			// Lookup file in the master list and use cached attributes

            // Issue a ReadFile call and update the list of pending read operations. Called by DirectStorageQueue when asked to submit a read request
            HRESULT AsyncRead(HANDLE file, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t userData, AsyncCallback callback);

            HRESULT CreateQueue(const DSTORAGE_QUEUE_DESC* desc, REFIID riid, _COM_Outptr_ void** ppv) override;
            HRESULT OpenFile(_In_z_ const WCHAR* path, REFIID riid, _COM_Outptr_ void** ppv) override;
            HRESULT CreateStatusArray(UINT32 capacity, PCSTR name, REFIID riid, _COM_Outptr_ void** ppv) override;
            HRESULT SetCpuAffinity(UINT64 affinity) override;
            void SetDebugFlags(UINT32 /*flags*/) override {}

            void KickManagementThread() { SetEvent(m_kickThreadEvent); }

        public:
            HRESULT SetStagingBufferSize(UINT32 size) override;

            // Default IUnknown interfaces
            HRESULT STDMETHODCALLTYPE QueryInterface(/* [in] */ REFIID riid,/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;
            ULONG STDMETHODCALLTYPE AddRef(void) override;
            ULONG STDMETHODCALLTYPE Release(void) override;
        };
    }
}

#pragma warning(pop)
