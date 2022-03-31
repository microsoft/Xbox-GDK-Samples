//--------------------------------------------------------------------------------------
// DirectStorageWin32WrapperQueue.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DirectStorageWin32Wrapper.h"
#include <d3d12.h>
#include "zlib/include/zlib.h"

#include <tuple> // for std::ignore

using namespace DirectStorageWin32Wrapper;
using namespace DirectStorageWin32Wrapper::Internal;

//////////////////////////////////////////////////////////////////////////
/// \brief QueryInterface
/// \details DirectStorageQueue::QueryInterface
/// \details Instantiation of IUnknown::QueryInterface
//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE DirectStorageQueue::QueryInterface(/* [in] */ REFIID riid,/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
{
	if (riid != __uuidof(IDStorageQueueWin32))
		return E_NOINTERFACE;
	*ppvObject = this;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
/// \brief AddRef
/// \details DirectStorageQueue::AddRef
/// \details Instantiation of IUnknown::AddRef
//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE DirectStorageQueue::AddRef(void)
{
	m_refCount++;
	return m_refCount.load();
}

//////////////////////////////////////////////////////////////////////////
/// \brief Release
/// \details DirectStorageQueue::Release
/// \details Instantiation of IUnknown::Release
//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE DirectStorageQueue::Release(void)
{
	if (m_refCount.fetch_sub(1) == 1)
	{
		delete this;
		return 0;
	}
	return m_refCount.load();
}

//////////////////////////////////////////////////////////////////////////
/// \brief SwitchState
/// \details DirectStorageQueue::QueueEntry::MarkState
/// \details All state switches are coalesced into this function to allow for easier error checking
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::QueueEntry::SwitchState(const State newState)
{
	switch (newState)
	{
	case State::STATE_PENDING:
		assert(currentState == State::STATE_BLANK);
		break;
	case State::STATE_SUBMITTED:
		assert(currentState == State::STATE_PENDING);
		break;
	case State::STATE_READING:
		assert(currentState == State::STATE_SUBMITTED);
		break;
	case State::STATE_READY_DECOMPRESS:
		if (IsMemoryDecompression())
			assert(currentState == State::STATE_PENDING);
		else
			assert(currentState == State::STATE_READING);
		break;
	case State::STATE_DECOMPRESSING:
		assert(currentState == State::STATE_READY_DECOMPRESS);
		break;
	case State::STATE_ERROR:
		assert((currentState != State::STATE_BLANK) && (currentState != State::STATE_CANCELLED) && (currentState != State::STATE_READING));
		break;
	case State::STATE_FINISHED:
		assert((currentState == State::STATE_READING) || (currentState == State::STATE_DECOMPRESSING));
		break;
	case State::STATE_BLANK:
		if (IsRequestEntry())
			assert((currentState == State::STATE_FINISHED) || (currentState == State::STATE_ERROR) || (currentState == State::STATE_CANCELLED));
		else
			assert(currentState == State::STATE_SUBMITTED);
		break;
	case State::STATE_CANCELLED:
		assert((currentState == State::STATE_SUBMITTED) || (currentState == State::STATE_PENDING));
		break;
	default:
		assert(!"Undefined state handled in DirectStorageQueue::QueueEntry::SwitchState");
		break;
	}
	currentState = newState;
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageQueue
/// \details DirectStorageQueue::DirectStorageQueue
/// \details The parameters must match the interface defined in the Microsoft GDK with Xbox extensions DirectStorage spec
//////////////////////////////////////////////////////////////////////////
DirectStorageQueue::DirectStorageQueue(const DSTORAGE_QUEUE_DESC* desc) :
    m_firstErrorSinceStatusSignal(S_OK),
    m_errorRecord{},
    m_refCount(0),
    m_entries(desc->Capacity),
    m_criticalSection(100),
    m_nextRequestSubmitted(0),
    m_nextRequestEnqueued(0),
    m_nextRequestNotComplete(0),
    m_nextRequestRead(0),
    m_errorEvent(nullptr),
    m_description{}
{
	m_errorRecord.FailureCount = 0;
	memcpy(&m_description, desc, sizeof(m_description));
	m_errorEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

//////////////////////////////////////////////////////////////////////////
/// \brief ~DirectStorageQueue
/// \details DirectStorageQueue::~DirectStorageQueue
/// \details Must wait for all pending requests to become completed and eventually switch to blank state
/// \details DirectStorageFactory owns all instances of DirectStorageQueue and must be notified on destruction
//////////////////////////////////////////////////////////////////////////
DirectStorageQueue::~DirectStorageQueue()
{
	assert(m_refCount == 0);
	if (m_entries)					// queue has not already been closed
	{
		// must wait until all pending requests have completed
		while (NumberFreeSlots() != m_description.Capacity)
		{
			Sleep(DirectStorageFactory::c_managementThreadSleepTimeMS);
		}
	}

	// Notify factory this queue instance has been destroyed
	DirectStorageFactory::GetInstance()->RemoveQueue(this);
	CloseHandle(m_errorEvent);
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageQueue
/// \details DirectStorageQueue::EnqueueRequest
/// \details Instantiation of IDStorageQueueWin32::EnqueueRequest function
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::EnqueueRequest(const DSTORAGE_REQUEST* request)
{
	if (!m_entries)					// Queue has been closed
		return;

	if (request == nullptr)
		return;

	// Queue is currently full, spin until there is a free slot for the new request
	while (!m_entries[m_nextRequestEnqueued].IsBlank())
	{
		SwitchToThread();
	}

	// The queue has hit the threshold of pending requests for auto submission
	if (NumberPending() >= (m_description.Capacity / 2))
	{
		Submit();
	}

	{
		std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);

		m_entries[m_nextRequestEnqueued].SwitchState(State::STATE_PENDING);
		m_entries[m_nextRequestEnqueued].entryType = QueueEntry::EntryType::REQUEST_ENTRY;
		m_entries[m_nextRequestEnqueued].stagingBuffer = nullptr;
		m_entries[m_nextRequestEnqueued].stagingBuffer2 = nullptr;

		memcpy(&(m_entries[m_nextRequestEnqueued].request), request, sizeof(DSTORAGE_REQUEST));

		if (request->Options.SourceType == DSTORAGE_REQUEST_SOURCE_FILE)
		{
			if (request->File)   // Check the read location against the cached extents of the file
			{
				DirectStorageFile* file = static_cast<DirectStorageFile*> (request->File);
				uint64_t fileSize = DirectStorageFactory::GetInstance()->GetFileSize(file->GetRawHandle());
				if (request->FileOffset >= fileSize)
					MarkRequestError(m_nextRequestEnqueued, E_DSTORAGE_END_OF_FILE, false);
				if ((request->FileOffset + request->SourceSize) < request->FileOffset)					// overflow
					MarkRequestError(m_nextRequestEnqueued, E_DSTORAGE_END_OF_FILE, false);
				if ((request->FileOffset + request->SourceSize) > fileSize)
					MarkRequestError(m_nextRequestEnqueued, E_DSTORAGE_END_OF_FILE, false);
			}
			else				// no file was used for this request which is automatically an error condition
			{
				MarkRequestError(m_nextRequestEnqueued, E_DSTORAGE_FILE_NOT_OPEN, false);
			}
		}
		IncrementNextRequestEnqueued();
	}
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageQueue
/// \details DirectStorageQueue::EnqueueStatus
/// \details Instantiation of IDStorageQueueWin32::EnqueueStatus function
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::EnqueueStatus(IDStorageStatusArrayWin32* statusArray, UINT32 index)
{
	if (!m_entries)					// Queue has been closed
		return;

	// Queue is currently full, spin until there is a free slot for the new request
	while (!m_entries[m_nextRequestEnqueued].IsBlank())
	{
		SwitchToThread();
	}

	// The queue has hit the threshold of pending requests for auto submission
	if (NumberPending() >= m_description.Capacity / 2)
	{
		Submit();
	}

	{
		std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);

		m_entries[m_nextRequestEnqueued].SwitchState(State::STATE_PENDING);
		m_entries[m_nextRequestEnqueued].entryType = QueueEntry::EntryType::STATUS_ENTRY;
		m_entries[m_nextRequestEnqueued].statusArray = static_cast<DirectStorageStatusArray*> (statusArray);
		m_entries[m_nextRequestEnqueued].statusIndex = index;
		m_entries[m_nextRequestEnqueued].statusArray->MarkState(index, false, S_OK);
		IncrementNextRequestEnqueued();
	}
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageQueue
/// \details DirectStorageQueue::EnqueueSignal
/// \details Instantiation of IDStorageQueueWin32::EnqueueSignal function
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::EnqueueSignal(ID3D12Fence* fence, UINT64 value)
{
	if (!m_entries)					// Queue has been closed
		return;

	// Queue is currently full, spin until there is a free slot for the new request
	while (!m_entries[m_nextRequestEnqueued].IsBlank())
	{
		SwitchToThread();
	}

	// The queue has hit the threshold of pending requests for auto submission
	if (NumberPending() >= m_description.Capacity / 2)
	{
		Submit();
	}

	{
		std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);

		m_entries[m_nextRequestEnqueued].SwitchState(State::STATE_PENDING);
		m_entries[m_nextRequestEnqueued].entryType = QueueEntry::EntryType::FENCE_ENTRY;
		m_entries[m_nextRequestEnqueued].fence = fence;
		m_entries[m_nextRequestEnqueued].fenceValue = value;
		IncrementNextRequestEnqueued();
	}
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageQueue
/// \details DirectStorageQueue::Submit
/// \details Instantiation of IDStorageQueueWin32::Submit function
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::Submit()
{
	if (!m_entries)					// Queue has been closed
		return;

	std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);

	// Iterate over all new requests since last submission and convert them to submitted if they are currently pending
	// Requests may already be in an error or a cancelled state, do not convert those to submitted
	while (m_nextRequestSubmitted != m_nextRequestEnqueued)
	{
		if (m_entries[m_nextRequestSubmitted].IsPending())
		{
			if (m_entries[m_nextRequestSubmitted].IsMemoryDecompression())
				m_entries[m_nextRequestSubmitted].SwitchState(State::STATE_READY_DECOMPRESS);
			else
				m_entries[m_nextRequestSubmitted].SwitchState(State::STATE_SUBMITTED);
		}
		IncrementNextRequestSubmitted();
	}
	SignalStatusOrFence();
	DirectStorageFactory::GetInstance()->KickManagementThread();
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageQueue
/// \details DirectStorageQueue::CancelRequestsWithTag
/// \details Instantiation of IDStorageQueueWin32::CancelRequestsWithTag function
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::CancelRequestsWithTag(UINT64 mask, UINT64 value)
{
	if (!m_entries)					// Queue has been closed
		return;

	bool didCancel = false;
	std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
	// TODO: Consider adjusting loop to only go over pending and submitted entries as opposed to entire queue based on optimization pass
	for (uint32_t i = 0; i < m_description.Capacity; i++)
	{
		if (m_entries[i].entryType != QueueEntry::EntryType::REQUEST_ENTRY)
			continue;
		// Only pending and submitted requests can be canceled
		// Other requests are either completed, currently being processed by the hardware, or an error
		if (m_entries[i].IsCancellable())
		{
			if ((m_entries[i].request.CancellationTag & mask) == value)
			{
				m_entries[i].SwitchState(State::STATE_CANCELLED);
				didCancel = true;
			}
		}
	}
	if (didCancel)
		SignalStatusOrFence();
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageQueue
/// \details DirectStorageQueue::Close
/// \details Instantiation of IDStorageQueueWin32::Close function
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::Close()
{
	if (!m_entries)					// Queue is already closed
		return;

	// must wait until all pending requests have completed
	std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
	while (NumberFreeSlots() != m_description.Capacity)
	{
		Sleep(DirectStorageFactory::c_managementThreadSleepTimeMS);
	}
	m_entries.reset();
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageQueue
/// \details DirectStorageQueue::RetrieveErrorRecord
/// \details Instantiation of IDStorageQueueWin32::RetrieveErrorRecord function
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::RetrieveErrorRecord(_Out_ DSTORAGE_ERROR_RECORD* record)
{
	if (!m_entries)					// Queue has been closed
		return;

	if (!record)
		return;

	std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
	memcpy(record, &m_errorRecord, sizeof(m_errorRecord));
	memset(&m_errorRecord, 0, sizeof(m_errorRecord));
}

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageQueue
/// \details DirectStorageQueue::Query
/// \details Instantiation of IDStorageQueueWin32::Query function
/// \details Fills info with DSTORAGE_QUEUE_DESC used to create the queue as well as number of slots
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::Query(_Out_ DSTORAGE_QUEUE_INFO* info)
{
	if (!info)
		return;
	info->Desc = m_description;
	info->EmptySlotCount = static_cast<UINT16> (NumberFreeSlots());
	info->RequestCountUntilAutoSubmit = static_cast<UINT16> (NumberPending()) - (m_description.Capacity / 2);
}

//////////////////////////////////////////////////////////////////////////
/// \brief DSCallbackFunction
/// \details DirectStorageQueue::DSCallbackFunction
/// \details Callback function called when the factory has finished a read request
/// \details The userData parameter will be the index in the queue that matches the request
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::DSCallbackFunction(HRESULT errorResult, uintptr_t userData)
{
	std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
	assert(userData < m_nextRequestSubmitted);
	assert(m_entries[userData].IsReading());

	if (FAILED(errorResult))				// Read request failed for some reason, save the HRESULT
	{
		DirectStorageFactory::GetInstance()->FreeStagingMemory(m_entries[userData].stagingBuffer);
		MarkRequestError(userData, errorResult, true);
	}
	else                                    // Read was successful, copy the data from the staging buffer to the destination buffer
	{
		if (m_entries[userData].IsCompressed())
		{
			m_entries[userData].SwitchState(State::STATE_READY_DECOMPRESS);
		}
		else if (m_entries[userData].stagingBufferUsed)
		{
			char* realSrcAddress = static_cast<char*> (m_entries[userData].stagingBuffer);

			// Adjust for unaligned read request by the title, request was converted to aligned for Win32 during submission
			uint64_t realFileOffset = m_entries[userData].request.FileOffset;
			realFileOffset &= ~(c_fileAlignment - 1ULL);
			realSrcAddress += (m_entries[userData].request.FileOffset - realFileOffset);
			memcpy(m_entries[userData].request.Destination, realSrcAddress, m_entries[userData].request.DestinationSize);
			DirectStorageFactory::GetInstance()->FreeStagingMemory(m_entries[userData].stagingBuffer);
			m_entries[userData].stagingBuffer = nullptr;
			m_entries[userData].stagingBufferUsed = false;
			MarkRequestCompleted(userData);
		}
		else    // request was 4k aligned so we didn't allocate a staging buffer and read straight into the destination
		{
			MarkRequestCompleted(userData);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
/// \brief SubmitNextRequest
/// \details DirectStorageQueue::SubmitNextRequest
/// \details Called by the factory to submit one request to Win32 for reading
/// \details The factory chooses the queue based on priority
//////////////////////////////////////////////////////////////////////////
DirectStorageQueue::SubmissionResult DirectStorageQueue::SubmitNextRequest()
{
	if (!m_entries)					// Queue has been closed
		return SubmissionResult::RESULT_NOTHING_TO_SUBMIT;

	if (m_nextRequestRead >= m_nextRequestSubmitted)
		return SubmissionResult::RESULT_NOTHING_TO_SUBMIT;

	std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);

	HRESULT readError = E_FAIL;
	while (!SUCCEEDED(readError))
	{
		// skip over requests already marked as an error as well as any status or fence requests
		while (!m_entries[m_nextRequestRead].IsRequestValidForReading())
		{
			IncrementNextRequestRead();
			if (m_nextRequestRead == m_nextRequestSubmitted)
				return SubmissionResult::RESULT_NOTHING_TO_SUBMIT;
		}

		assert(m_entries[m_nextRequestRead].IsRequestEntry());
		assert(m_entries[m_nextRequestRead].IsRequestValidForReading());

		// Convert titles possible unaligned read request to an aligned read request
		// TODO: Consider removing the staging buffer if the original request was aligned
		uint64_t realFileOffset = m_entries[m_nextRequestRead].request.FileOffset;
		realFileOffset &= ~(c_fileAlignment - 1ULL);

		uint32_t realBytesToRead = m_entries[m_nextRequestRead].request.SourceSize;
		uint64_t readOffset = m_entries[m_nextRequestRead].request.FileOffset - realFileOffset;
		assert(readOffset < c_fileAlignment);
		realBytesToRead += static_cast<uint32_t> (readOffset);			// add on the extra bits for the start of the block
		realBytesToRead += (c_fileAlignment - 1);						// round up to the next size of alignment
		realBytesToRead &= ~(c_fileAlignment - 1);

		if ((realFileOffset != m_entries[m_nextRequestRead].request.FileOffset) || (realBytesToRead != m_entries[m_nextRequestRead].request.SourceSize))
		{
			m_entries[m_nextRequestRead].stagingBuffer = DirectStorageFactory::GetInstance()->AllocateStagingMemory(realBytesToRead);
			// The staging buffer is exhausted, in this case the factory will back off on submitted new read requests until some have completed
			// This queue will not lose its place in line based on priority
			if (m_entries[m_nextRequestRead].stagingBuffer == nullptr)
				return SubmissionResult::RESULT_WAITING_ON_MEMORY;
			m_entries[m_nextRequestRead].stagingBufferUsed = true;
		}
		else
		{
			m_entries[m_nextRequestRead].stagingBuffer = m_entries[m_nextRequestRead].request.Destination;
			m_entries[m_nextRequestRead].stagingBufferUsed = false;
		}

		if ((m_entries[m_nextRequestRead].request.Options.ZlibDecompress) && (m_entries[m_nextRequestRead].request.Options.BcpackMode != DSTORAGE_BCPACK_MODE_NONE))
		{
			m_entries[m_nextRequestRead].stagingBuffer2 = DirectStorageFactory::GetInstance()->AllocateStagingMemory(m_entries[m_nextRequestRead].request.IntermediateSize);
			if (m_entries[m_nextRequestRead].stagingBuffer2 == nullptr)
			{
				DirectStorageFactory::GetInstance()->FreeStagingMemory(m_entries[m_nextRequestRead].stagingBuffer);
				m_entries[m_nextRequestRead].stagingBuffer = nullptr;
				return SubmissionResult::RESULT_WAITING_ON_MEMORY;
			}
		}
		else
		{
			m_entries[m_nextRequestRead].stagingBuffer2 = nullptr;
		}

		m_entries[m_nextRequestRead].stagingBufferSize = realBytesToRead;

		HANDLE file = (static_cast<DirectStorageFile*> (m_entries[m_nextRequestRead].request.File))->GetRawHandle();

		// Note: All read requests are initially marked as pending by the underlying file system even if Win32 returns immediately with the data
		// This allows a common path for processing completed requests
		readError = DirectStorageFactory::GetInstance()->AsyncRead(file, m_entries[m_nextRequestRead].stagingBuffer, realBytesToRead, realFileOffset, m_nextRequestRead, std::bind(&DirectStorageQueue::DSCallbackFunction, this, std::placeholders::_1, std::placeholders::_2));
		if (FAILED(readError))
		{
			MarkRequestError(m_nextRequestRead, readError, true);
		}
		else
		{
			m_entries[m_nextRequestRead].SwitchState(State::STATE_READING);
		}

		IncrementNextRequestRead();
	}

	return SubmissionResult::RESULT_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
/// \brief AnyPendingRequestsForRead
/// \details DirectStorageQueue::AnyPendingRequestsForRead
/// \details Quick check to see if SubmitNextRequest should be called
/// \details Explicitly does not check for only status/fence object left in the queue
/// \details This will be caught naturally at the next request to perform a read
//////////////////////////////////////////////////////////////////////////
bool DirectStorageQueue::AnyRequestsWaitingForRead()
{
	if (!m_entries)					// Queue has been closed
		return false;

	//std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
	return m_nextRequestRead < m_nextRequestSubmitted;
}

//////////////////////////////////////////////////////////////////////////
/// \brief AnyPendingRequestsForRead
/// \details DirectStorageQueue::AnyPendingRequestsForRead
/// \details Quick check to see if SubmitNextRequest should be called
/// \details Explicitly does not check for only status/fence object left in the queue
/// \details This will be caught naturally at the next request to perform a read
//////////////////////////////////////////////////////////////////////////
bool DirectStorageQueue::AnyRequestsWaitingForDecompression()
{
	if (!m_entries)					// Queue has been closed
		return false;

	//std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
	for (uint64_t curEntry = m_nextRequestNotComplete; curEntry < m_nextRequestRead; ++curEntry)
	{
		if (m_entries[curEntry].IsReadyDecompression())
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
/// \brief MarkRequestCompleted
/// \details DirectStorageQueue::MarkRequestCompleted
/// \details Handle updating a request to completing including signaling a status/fence
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::MarkRequestCompleted(size_t index)
{
	if (!m_entries)					// Queue has been closed
		return;

	//std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
	m_entries[index].SwitchState(State::STATE_FINISHED);
	SignalStatusOrFence();
}

//////////////////////////////////////////////////////////////////////////
/// \brief MarkRequestError
/// \details DirectStorageQueue::MarkRequestError
/// \details Handle updating a request to an error
/// \details Updates m_errorRecord if this is the first error since the last RetrieveErrorRecord
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::MarkRequestError(size_t index, HRESULT errorResult, bool signalStatusOrFence)
{
	if (!m_entries)					// Queue has been closed
		return;

	{
		std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
		assert(m_entries[index].IsSubmitted());
		m_entries[index].SwitchState(State::STATE_ERROR);
		m_errorRecord.FailureCount++;
		if (m_errorRecord.FailureCount == 1)
		{
			HANDLE file = static_cast<DirectStorageFile*> (m_entries[index].request.File);
			const std::wstring& fileName = DirectStorageFactory::GetInstance()->GetFileName(file);
			m_errorRecord.FirstFailure.HResult = errorResult;
			wcsncpy_s(m_errorRecord.FirstFailure.Filename, MAX_PATH, fileName.c_str(), fileName.size());
			m_errorRecord.FirstFailure.Options = m_entries[index].request.Options;
			m_errorRecord.FirstFailure.Destination = m_entries[index].request.Destination;
			m_errorRecord.FirstFailure.DestinationSize = m_entries[index].request.DestinationSize;
			m_errorRecord.FirstFailure.FileOffset = m_entries[index].request.FileOffset;
			m_errorRecord.FirstFailure.SourceSize = m_entries[index].request.SourceSize;
			m_errorRecord.FirstFailure.CancellationTag = m_entries[index].request.CancellationTag;
		}
		if (m_firstErrorSinceStatusSignal == S_OK)
			m_firstErrorSinceStatusSignal = errorResult;
	}
	if (signalStatusOrFence)		// error might be set when the request is enqueued which means it's pointless to signal because no signal/fence has been submitted yet
		SignalStatusOrFence();
	SetEvent(m_errorEvent);
}

//////////////////////////////////////////////////////////////////////////
/// \brief SignalStatusOrFence
/// \details DirectStorageQueue::SignalStatusOrFence
/// \details Scans requests since the last status/fence to the next status/fence
/// \details If all previous requests are completed/error then signal the status/fence
//////////////////////////////////////////////////////////////////////////
void DirectStorageQueue::SignalStatusOrFence()
{
	if (!m_entries)					// Queue has been closed
		return;

	std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);

	while ((m_nextRequestNotComplete != m_nextRequestSubmitted) && m_entries[m_nextRequestNotComplete].IsRequestComplete())
	{
		if (m_entries[m_nextRequestNotComplete].IsStatusEntry())
		{
			m_entries[m_nextRequestNotComplete].statusArray->MarkState(m_entries[m_nextRequestNotComplete].statusIndex, true, m_firstErrorSinceStatusSignal);
			m_firstErrorSinceStatusSignal = S_OK;
		}
		else if (m_entries[m_nextRequestNotComplete].IsFenceEntry())
		{
			m_entries[m_nextRequestNotComplete].fence->Signal(m_entries[m_nextRequestNotComplete].fenceValue);
		}
		m_entries[m_nextRequestNotComplete].SwitchState(State::STATE_BLANK);
		IncrementNextRequestNotComplete();
	}
}

//////////////////////////////////////////////////////////////////////////
/// \brief NumberFreeSlots
/// \details DirectStorageQueue::NumberFreeSlots
/// \details Determine the number of open slots for new requests
//////////////////////////////////////////////////////////////////////////
size_t DirectStorageQueue::NumberFreeSlots() const
{
	if (!m_entries)					// Queue has been closed
		return 0;

	//std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
	if (m_nextRequestEnqueued == m_nextRequestNotComplete)
		return m_description.Capacity;
	return m_nextRequestEnqueued - m_nextRequestNotComplete;
}

//////////////////////////////////////////////////////////////////////////
/// \brief NumberPending
/// \details DirectStorageQueue::NumberPending
/// \details How many requests have been enqueued but not submitted
//////////////////////////////////////////////////////////////////////////
size_t DirectStorageQueue::NumberPending() const
{
	if (!m_entries)					// Queue has been closed
		return 0;

	//std::lock_guard<ATG::CriticalSectionLockable> queueLock(m_criticalSection);
	return m_nextRequestEnqueued - m_nextRequestSubmitted;
}

//////////////////////////////////////////////////////////////////////////
/// \brief PerformDecompression
/// \details DirectStorageQueue::PerformDecompression
/// \details Will check and perform any pending decompression requests
/// \details Will keep decompressing until total duration >= maxExecutionTime
/// \returns How much time it spent decompressing data
//////////////////////////////////////////////////////////////////////////
DirectStorageQueue::DecompressionResult DirectStorageQueue::DecompressNextRequest()
{
	m_criticalSection.lock();
	bool didDecompression = false;
	uint64_t curEntry = m_nextRequestNotComplete;
	while ((curEntry != m_nextRequestRead) && !didDecompression)
	{
		if (m_entries[curEntry].IsReadyDecompression())
		{
			m_entries[curEntry].SwitchState(DirectStorageQueue::State::STATE_DECOMPRESSING);
			m_criticalSection.unlock();
			char* realSrcAddress = nullptr;
			if (m_entries[curEntry].IsMemoryDecompression())
			{
				realSrcAddress = static_cast<char*> (m_entries[curEntry].request.Source);
			}
			else
			{
				realSrcAddress = static_cast<char*> (m_entries[curEntry].stagingBuffer);

				// Adjust for unaligned read request by the title, request was converted to aligned for Win32 during submission
				// TODO: Decompression needs to be hooked up here
				uint64_t realFileOffset = m_entries[curEntry].request.FileOffset;
				realFileOffset &= ~(c_fileAlignment - 1ULL);
				realSrcAddress += (m_entries[curEntry].request.FileOffset - realFileOffset);
			}

			void* zlibDestBuffer = m_entries[curEntry].request.Destination;

			// BCPack support is currently not implemented
#if 0
			void* bcPackSrcBuffer = realSrcAddress;
			if ((m_entries[curEntry].request.Options.ZlibDecompress) && (m_entries[curEntry].request.Options.BcpackMode != DSTORAGE_BCPACK_MODE_NONE))
			{
				zlibDestBuffer = m_entries[curEntry].stagingBuffer2;
				bcPackSrcBuffer = m_entries[curEntry].stagingBuffer2;
			}
#endif
			if (m_entries[curEntry].request.Options.ZlibDecompress)
			{
				std::ignore = LZInflate(zlibDestBuffer, m_entries[curEntry].request.DestinationSize, realSrcAddress, m_entries[curEntry].request.SourceSize);
			}

			// BCPack support is currently not implemented
#if 0
			if (m_entries[curEntry].request.Options.BcpackMode != DSTORAGE_BCPACK_MODE_NONE)
			{
				uint32_t mode = m_entries[curEntry].request.Options.BcpackMode;
				mode -= 1;
				mode *= 2;
				BCPack::Unpack((BCPack::Format)mode, m_entries[curEntry].request.Destination, m_entries[curEntry].request.DestinationSize, bcPackSrcBuffer, bcPackedSize);
			}
#else
			if (m_entries[curEntry].request.Options.BcpackMode != DSTORAGE_BCPACK_MODE_NONE)
			{
				MarkRequestError(curEntry, E_NOTIMPL, true);
			}
#endif
			didDecompression = true;

			DirectStorageFactory::GetInstance()->FreeStagingMemory(m_entries[curEntry].stagingBuffer);
			DirectStorageFactory::GetInstance()->FreeStagingMemory(m_entries[curEntry].stagingBuffer2);
			m_criticalSection.lock();
			if (!m_entries[curEntry].IsError())
				MarkRequestCompleted(curEntry);
		}
		++curEntry;
	}

	m_criticalSection.unlock();
	return didDecompression ? DecompressionResult::RESULT_SUCCESS : DecompressionResult::RESULT_NOTHING_TO_DECOMPRESS;
}

size_t DirectStorageQueue::LZInflate(void* output, size_t outputSize, const void* input, size_t length)
{
	// Initialize inflate
	z_stream strm = {};
	strm.data_type = Z_BINARY;
	int err = inflateInit(&strm);

	strm.total_in = strm.avail_in = (uInt)length;
	strm.next_in = (Bytef*)input;
	strm.avail_out = (uInt)outputSize;
	strm.next_out = (Bytef*)output;

	// Inflate the input buffer in one step
	err = inflate(&strm, Z_FINISH);
	if (err != Z_STREAM_END)
	{
		throw std::exception("Internal ZLIB error");
	}

	inflateEnd(&strm);

	return strm.total_out;
}
