// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"
#include "XTaskQueuePriv.h"
#include "XAsyncProvider.h"

#define ASYNC_BLOCK_SIG         0x41535942 // ASYB
#define ASYNC_BLOCK_RESULT_SIG  0x41535242 // ASRB
#define ASYNC_STATE_SIG         0x41535445 // ASTE

// Used by unit tests to verify we cleanup memory correctly.
std::atomic<uint32_t> s_AsyncLibGlobalStateCount{ 0 };

// Set externally to enable pumping waits.
bool s_AsyncLibEnablePumpingWait = false;

enum class ProviderCleanupLocation
{
    Destructor,     // Cleanup provider in destructor
    AfterDoWork,    // Cleanup provider after DoWork
    InCancel,       // XAsyncCancel code is calling into provider
    CleanedUp       // Provider clean up has been called
};

// Note that there are two XAsyncBlock structures in play.
// There is the pointer allocated and passed to us by the user
// and there is our own local copy.  We pass the local copy to
// async provider callbacks, and pass the user pointer to all
// user-visible callbacks.  The AsyncBlockInternalGuard class keeps
// the state of these two classes in sync under a lock.  This allows
// us to provide our own local task queue for use by the provider
// without the confusion of offering two queue pointers or modifying
// a structure the user passed to us.

struct AsyncState
{
    uint32_t signature = ASYNC_STATE_SIG;
    std::atomic<uint32_t> refs{ 1 };
    std::atomic<ProviderCleanupLocation> providerCleanup{ ProviderCleanupLocation::Destructor };
    std::atomic<bool> workScheduled{ false };
    bool valid = true;
    XAsyncProvider* provider = nullptr;
    XAsyncProviderData providerData{ };
    XAsyncBlock providerAsyncBlock { };
    XAsyncBlock* userAsyncBlock = nullptr;
    XTaskQueueHandle queue = nullptr;
    std::mutex waitMutex;
    std::condition_variable waitCondition;
    bool waitSatisfied = false;

#if _WIN32
    HANDLE waitEvent = nullptr;
#endif

    const void* identity = nullptr;
    const char* identityName = nullptr;

    void* operator new(size_t size, size_t additional)
    {
        return ::operator new(size + additional);
    }

    void operator delete(void* ptr)
    {
        ::operator delete(ptr);
    }

    AsyncState() noexcept
    {
        ++s_AsyncLibGlobalStateCount;
    }

    void AddRef() noexcept
    {
        ++refs;
    }

    void Release() noexcept
    {
        if (--refs == 0)
        {
            delete this;
        }
    }

private:

    ~AsyncState() noexcept
    {
        if (provider != nullptr)
        {
            // Have we already called cleanup?
            ProviderCleanupLocation loc = providerCleanup.exchange(ProviderCleanupLocation::CleanedUp);
            if (loc != ProviderCleanupLocation::CleanedUp)
            {
                (void)provider(XAsyncOp::Cleanup, &providerData);
            }
        }

        if (queue != nullptr)
        {
            XTaskQueueCloseHandle(queue);
        }

#if _WIN32
        if (waitEvent != nullptr)
        {
            CloseHandle(waitEvent);
        }
#endif

        --s_AsyncLibGlobalStateCount;
    }
};

struct AsyncBlockInternal
{
    AsyncState* state = nullptr;
    HRESULT status = E_PENDING;
    DWORD signature = ASYNC_BLOCK_SIG;
    std::atomic_flag lock = ATOMIC_FLAG_INIT;
};
static_assert(sizeof(AsyncBlockInternal) <= sizeof(XAsyncBlock::internal),
    "Unexpected size for AsyncBlockInternal");
static_assert(std::alignment_of<AsyncBlockInternal>::value == std::alignment_of<void*>::value,
    "Unexpected alignment for AsyncBlockInternal");
static_assert(std::is_trivially_destructible<AsyncBlockInternal>::value,
    "Unexpected nontrivial destructor for AsyncBlockInternal");

class AsyncStateRef
{
public:
    AsyncStateRef() noexcept
        : m_state{ nullptr }
    {}
    explicit AsyncStateRef(AsyncState* state) noexcept
        : m_state{ state }
    {
        if (m_state)
        {
            m_state->AddRef();
        }
    }
    AsyncStateRef(const AsyncStateRef&) = delete;
    AsyncStateRef(AsyncStateRef&& other) noexcept
        : m_state{ other.m_state }
    {
        other.m_state = nullptr;
    }
    AsyncStateRef& operator=(const AsyncStateRef&) = delete;
    AsyncStateRef& operator=(AsyncStateRef&& other) noexcept
    {
        if (&other != this) { std::swap(m_state, other.m_state); }
        return *this;
    }
    ~AsyncStateRef() noexcept
    {
        if (m_state)
        {
            m_state->Release();
        }
    }
    AsyncState* operator->() const noexcept
    {
        return m_state;
    }
    bool operator==(std::nullptr_t) const noexcept
    {
        return m_state == nullptr;
    }
    bool operator!=(std::nullptr_t) const noexcept
    {
        return m_state != nullptr;
    }
    void Attach(AsyncState* state) noexcept
    {
        ASSERT(!m_state);
        m_state = state;
    }
    AsyncState* Detach() noexcept
    {
        AsyncState* p = m_state;
        m_state = nullptr;
        return p;
    }

    AsyncState* Get() const { return m_state; }

private:
    AsyncState* m_state;
};

class AsyncBlockInternalGuard
{
public:
    AsyncBlockInternalGuard(_Inout_ XAsyncBlock* asyncBlock) noexcept :
        m_internal(DoLock(asyncBlock))
    {
        m_locked = m_internal != nullptr;

        if (!m_locked)
        {
            // We never locked because the block contains an invalid signature.  We still
            // need the block for access though (although that access will be read only).
            m_internal = reinterpret_cast<AsyncBlockInternal*>(asyncBlock->internal);
        }

        if (m_internal->state != nullptr)
        {
            m_userInternal = reinterpret_cast<AsyncBlockInternal*>(m_internal->state->userAsyncBlock->internal);
        }
        else
        {
            m_userInternal = m_internal;
        }

        // If user internal != internal, we grab its lock.  Note that
        // lock ordering here is critical.  It must always be 
        // state lock, then user lock.  If state is not available, then
        // it is just user lock.

        if (m_userInternal != m_internal)
        {
            while (m_userInternal->lock.test_and_set()) {}
        }
    }

    ~AsyncBlockInternalGuard() noexcept
    {
        if (m_locked)
        {
            m_internal->lock.clear();
            if (m_userInternal != m_internal)
            {
                m_userInternal->lock.clear();
            }
        }
    }

    AsyncStateRef GetState() const noexcept
    {
        AsyncStateRef state{ m_internal->state };

        if (state != nullptr && state->signature != ASYNC_STATE_SIG)
        {
            ASSERT(false);
            return AsyncStateRef{};
        }

        return state;
    }

    AsyncStateRef ExtractState(_In_ bool resultsRetrieved = false) const noexcept
    {
        AsyncStateRef state{ m_internal->state };
        m_internal->state = nullptr;
        m_userInternal->state = nullptr;

        // When XAsyncGetResults is called, it extracts state
        // with resultsRetrieved set to true, which places a
        // different signature into the async block. This is used
        // later as a marker to prevent duplicate calls to 
        // XAsyncGetResults.

        if (resultsRetrieved)
        {
            m_internal->signature = ASYNC_BLOCK_RESULT_SIG;
            m_userInternal->signature = ASYNC_BLOCK_RESULT_SIG;
        }
        else
        {
            m_internal->signature = 0;
            m_userInternal->signature = 0;
        }

        if (state != nullptr && state->signature != ASYNC_STATE_SIG)
        {
            ASSERT(false);
            return AsyncStateRef{};
        }

        return state;
    }

    HRESULT GetStatus() const noexcept
    {
        return m_internal->status;
    }

    bool GetResultsRetrieved()
    {
        return m_internal->signature == ASYNC_BLOCK_RESULT_SIG;
    }

    bool TrySetTerminalStatus(HRESULT status) noexcept
    {
        ASSERT(m_locked || m_internal->status != E_PENDING);
        if (m_locked && m_internal->status == E_PENDING)
        {
            ASSERT(m_userInternal->status == E_PENDING);
            m_userInternal->status = status;
            m_internal->status = status;

            return true;
        }
        else
        {
            return false;
        }
    }

private:
    AsyncBlockInternal * m_internal;
    AsyncBlockInternal * m_userInternal;
    bool m_locked = false;

    // Locks the correct async block and returns a pointer to the one
    // we locked.
    static AsyncBlockInternal* DoLock(_In_ XAsyncBlock* asyncBlock)
    {
        AsyncBlockInternal* lockedResult = reinterpret_cast<AsyncBlockInternal*>(asyncBlock->internal);

        ASSERT(lockedResult);

        // If the signature of this block is wrong, that means that it's not currently
        // in play. We can't lock because the lock flag could be invalid, and we can't
        // fix up the lock flag without introducing a race condition with other potential
        // lock calls.  All calls further down guard against an invalid state, so all
        // we do in this case is set the state to null.  We return null as an indicator
        // that we didn't lock.

        if (lockedResult->signature != ASYNC_BLOCK_SIG)
        {
            lockedResult->state = nullptr;
            return nullptr;
        }

        while (lockedResult->lock.test_and_set()) {}

        // We've locked the async block. We only ever want to keep a lock on one block
        // to prevent deadlocks caused by lock ordering.  If the state is still valid
        // on this block, we ensure the async block we're locking is the permanent one
        // associated with the async state.  

        if (lockedResult->state != nullptr && asyncBlock != &lockedResult->state->providerAsyncBlock)
        {
            // Grab a state ref here because releasing the lock can allow
            // the state to be cleared / released.
            AsyncStateRef state(lockedResult->state);
            lockedResult->lock.clear();

            // Now lock the async block on the state struct
            AsyncBlockInternal* stateAsyncBlockInternal = reinterpret_cast<AsyncBlockInternal*>(state->providerAsyncBlock.internal);
            while (stateAsyncBlockInternal->lock.test_and_set()) {}

            // We locked the right object, but we need to check here to see if we
            // lost the state after clearing the lock above.  If we did, then this
            // pointer is likely going to destruct as soon as we release
            // our state ref.  We should throw it away and grab the user block
            // again.

            if (stateAsyncBlockInternal->state == nullptr)
            {
                stateAsyncBlockInternal->lock.clear();
                while (lockedResult->lock.test_and_set()) {}
            }
            else
            {
                lockedResult = stateAsyncBlockInternal;
            }
        }

        return lockedResult;
    }
};

static void CALLBACK CompletionCallback(_In_ void* context, _In_ bool canceled);
static void CALLBACK WorkerCallback(_In_ void* context, _In_ bool canceled);
static void SignalWait(_In_ AsyncStateRef const& state);
static HRESULT SignalCompletion(_In_ AsyncStateRef const& state);
static HRESULT AllocStateNoCompletion(_Inout_ XAsyncBlock* asyncBlock, _Inout_ AsyncBlockInternal* internal, _In_ size_t contextSize);
static HRESULT AllocState(_Inout_ XAsyncBlock* asyncBlock, _In_ size_t contextSize);
static void CleanupState(_Inout_ AsyncStateRef&& state);
static void CleanupProviderForLocation(_Inout_ AsyncStateRef& state, _In_ ProviderCleanupLocation location);
static bool TrySetProviderCleanup(_Inout_ AsyncStateRef& state, _In_ ProviderCleanupLocation location);
static void RevertProviderCleanup(_Inout_ AsyncStateRef& state, _In_ ProviderCleanupLocation expected);

static HRESULT AllocStateNoCompletion(_Inout_ XAsyncBlock* asyncBlock, _Inout_ AsyncBlockInternal* internal, _In_ size_t contextSize)
{
    AsyncStateRef state;
    state.Attach(new (contextSize) AsyncState);
    RETURN_IF_NULL_ALLOC(state);

    if (contextSize != 0)
    {
        // User allocated additional context data.  This was allocated as extra bytes at the end of 
        // async state.
        state->providerData.context = (state.Get() + 1);
    }
    
    XTaskQueueHandle queue = asyncBlock->queue;
    if (queue == nullptr)
    {
        RETURN_HR_IF(E_NO_TASK_QUEUE, XTaskQueueGetCurrentProcessTaskQueue(&state->queue) == false);
    }
    else
    {
        RETURN_IF_FAILED(XTaskQueueDuplicateHandle(queue, &state->queue));
    }

    state->userAsyncBlock = asyncBlock;
    state->providerData.async = &state->providerAsyncBlock;

#if _WIN32
    state->waitEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    RETURN_LAST_ERROR_IF(state->waitEvent == nullptr);
#endif

    // Note: needs to be the last failable thing we do.
    RETURN_IF_FAILED(XTaskQueueSuspendTermination(state->queue));

    internal->state = state.Detach();

    // Duplicate the async block we've just configured
    internal->state->providerAsyncBlock = *asyncBlock;
    internal->state->providerAsyncBlock.queue = internal->state->queue;

    return S_OK;
}

static HRESULT AllocState(_Inout_ XAsyncBlock* asyncBlock, _In_ size_t contextSize)
{
    // If the async block is already associated with another
    // call, fail.

    // We need to guard against use of an active async block.  We don't want
    // to rely on the caller zeroing memory so we check a signature
    // DWORD. This signature is cleared when the block can be reused.
    auto internal = reinterpret_cast<AsyncBlockInternal*>(asyncBlock->internal);
    if (internal->signature == ASYNC_BLOCK_SIG)
    {
        RETURN_HR(E_INVALIDARG);
    }

    // This could be a reused async block from a prior
    // call, so zero everything.
    for (auto i = 0u; i < sizeof(asyncBlock->internal); ++i)
    {
        asyncBlock->internal[i] = 0;
    }

    // Construction is inherently single threaded
    // (there is nothing we can do if the client tries to use the same
    // XAsyncBlock in 2 calls at the same time)
    internal = new (asyncBlock->internal) AsyncBlockInternal{};

    HRESULT hr = AllocStateNoCompletion(asyncBlock, internal, contextSize);

    if (FAILED(hr))
    {
        internal->signature = 0;
        internal->status = hr;
    }

    RETURN_IF_FAILED(hr);
    return S_OK;
}

static void CleanupState(_Inout_ AsyncStateRef&& state)
{
    if (state != nullptr)
    {
        // Should only cleanup state after calling ExtractState to clear it.
        ASSERT((reinterpret_cast<AsyncBlockInternal*>(state->providerData.async->internal))->state == nullptr);

        state->valid = false;
        state->Release();
    }
}

static void CleanupProviderForLocation(_Inout_ AsyncStateRef& state, _In_ ProviderCleanupLocation location)
{
    if (state->providerCleanup.compare_exchange_strong(location, ProviderCleanupLocation::CleanedUp))
    {
        (void)state->provider(XAsyncOp::Cleanup, &state->providerData);
    }
}

static bool TrySetProviderCleanup(_Inout_ AsyncStateRef& state, _In_ ProviderCleanupLocation location)
{
    ProviderCleanupLocation expected = ProviderCleanupLocation::Destructor;
    return state->providerCleanup.compare_exchange_strong(expected, location);
}

static void RevertProviderCleanup(_Inout_ AsyncStateRef& state, _In_ ProviderCleanupLocation expected)
{
    (void)state->providerCleanup.compare_exchange_strong(expected, ProviderCleanupLocation::Destructor);
}

static HRESULT SignalCompletion(_In_ AsyncStateRef const& state)
{
    HRESULT hr = S_OK;

    if (state->providerData.async->callback != nullptr)
    {
        AsyncStateRef callbackState(state.Get());
        hr = XTaskQueueSubmitCallback(
            state->queue,
            XTaskQueuePort::Completion,
            callbackState.Get(),
            CompletionCallback);

        if (SUCCEEDED(hr))
        {
            callbackState.Detach();
        }
    }
    else
    {
        SignalWait(state);
    }

    return hr;
}

static void SignalWait(_In_ AsyncStateRef const& state)
{
    bool newlySatisfied;
    {
        std::lock_guard<std::mutex> lock(state->waitMutex);
        newlySatisfied = !state->waitSatisfied;
        state->waitSatisfied = true;
        state->waitCondition.notify_all();
    }
#if _WIN32
    SetEvent(state->waitEvent);
#endif

    // We should only come in here once, but we don't want
    // to underflow task queue resumes and we already know
    // from above if we're first marking this wait as
    // satisfied, so use it.

    ASSERT(newlySatisfied);
    if (newlySatisfied)
    {
        XTaskQueueResumeTermination(state->queue);
    }
}

static void CALLBACK CompletionCallback(
    _In_ void* context,
    _In_ bool canceled)
{
    // We must still let canceled callbacks through to clean
    // up user data.
    UNREFERENCED_PARAMETER(canceled);

    AsyncStateRef state;
    state.Attach(static_cast<AsyncState*>(context));

    // We always pass the user async block into the 
    // callback, but we don't trust it -- we check
    // the callback field on our internal copy.
    XAsyncBlock* asyncBlock = state->userAsyncBlock;
    if (state->providerAsyncBlock.callback != nullptr)
    {
        state->providerAsyncBlock.callback(asyncBlock);
    }

    SignalWait(state);
}

static void CALLBACK WorkerCallback(
    _In_ void* context,
    _In_ bool canceled)
{
    AsyncStateRef state;
    state.Attach(static_cast<AsyncState*>(context));
    state->workScheduled = false;

    if (!state->valid)
    {
        return;
    }

    // If the queue is canceling callbacks, simply cancel this work. Since no
    // new work for this call will be scheduled, if the call didn't cancel
    // immediately do it ourselves.

    if (canceled)
    {
        XAsyncCancel(state->userAsyncBlock);
        HRESULT callStatus;
        {
            AsyncBlockInternalGuard internal{ state->userAsyncBlock };
            callStatus = internal.GetStatus();
        }

        if (callStatus != E_ABORT)
        {
            XAsyncComplete(state->userAsyncBlock, E_ABORT, 0);
        }
    }
    else
    {
        HRESULT result = state->provider(XAsyncOp::DoWork, &state->providerData);

        // Work routine can return E_PENDING if there is more work to do.  Otherwise
        // it either needs to be a failure or it should have called XAsyncComplete, which
        // would have set a new value into the status.

        if (result != E_PENDING)
        {
            if (SUCCEEDED(result))
            {
                result = E_UNEXPECTED;
            }

            XAsyncComplete(&state->providerAsyncBlock, result, 0);
        }
    }

    // If the result of this call caused a completion with no payload, XAsyncComplete
    // will change the provider cleanup to be "AfterWork", which is here.  Cleanup
    // the provider if we need to.
    CleanupProviderForLocation(state, ProviderCleanupLocation::AfterDoWork);
}

//
// Public APIs
//

/// <summary>
/// Returns the status of the asynchronous operation, optionally waiting
/// for it to complete.  Once complete, you may call XAsyncGetResult if
/// the async call has a resulting data payload. If it doesn't, calling
/// XAsyncGetResult is unneeded.
/// </summary>
STDAPI XAsyncGetStatus(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_ bool wait
    ) noexcept
{
    HRESULT result = E_PENDING;
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        result = internal.GetStatus();
        state = internal.GetState();
    }

    // If we are being asked to wait, always check the wait state before
    // looking at the hresult.  Our wait waits until the completion runs
    // so we may need to wait past when the status is set.

    if (wait)
    {
        if (state == nullptr)
        {
            ASSERT(result != E_PENDING);
            RETURN_HR_IF(E_INVALIDARG, result == E_PENDING);
        }
        else
        {
// This codebase compiles for multiple platforms on GitHub.  This is only
// supported on win32 desktop platforms.  It is enabled for Windows builds.
#if _WIN32
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

            // If we're being asked to wait from a STA thread, use a CoWait to ensure we
            // don't totally gum up the thread.  Then fall back to our normal condition
            // variable check (both are signaled during completion).

            if (s_AsyncLibEnablePumpingWait)
            {
                APTTYPE aptType;
                APTTYPEQUALIFIER aptQualifier;
                if (SUCCEEDED(CoGetApartmentType(&aptType, &aptQualifier)) && aptType != APTTYPE_MTA && aptType != APTTYPE_NA)
                {
                    DWORD idx;
                    (void)CoWaitForMultipleHandles(COWAIT_DEFAULT, INFINITE, 1, &state->waitEvent, &idx);
                }
            }
#endif
#endif
            {
                std::unique_lock<std::mutex> lock(state->waitMutex);

                if (!state->waitSatisfied)
                {
                    AsyncState* s = state.Get();
                    state->waitCondition.wait(lock, [s] { return s->waitSatisfied; });
                }
            }

            {
                AsyncBlockInternalGuard internal{ asyncBlock };
                result = internal.GetStatus();
            }        
        }
    }

    return result;
}

/// <summary>
/// Returns the required size of the buffer to pass to XAsyncGetResult.
/// </summary>
STDAPI XAsyncGetResultSize(
    _Inout_ XAsyncBlock* asyncBlock,
    _Out_ size_t* bufferSize
    ) noexcept
{
    HRESULT result = E_PENDING;
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        result = internal.GetStatus();
        state = internal.GetState();
    }

    *bufferSize = state == nullptr ? 0 : state->providerData.bufferSize;

    return result;
}

/// <summary>
/// Tries to cancel an asynchronous operation. If canceled the status result will return E_ABORT
/// and the completion callback will be invoked.
/// </summary>
STDAPI_(void) XAsyncCancel(
    _Inout_ XAsyncBlock* asyncBlock
    ) noexcept
{
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        state = internal.GetState();
    }

    if (state != nullptr)
    {
        // In case of cancel, failure, or success with no payload we will
        // agressively clean up the provider at the end of DoWork. This can race
        // with a cancel call. To prevent this we mark the provider cleanup as
        // "in cancel", which prevents switching it to the aggressive DoWork
        // cleanup.  We switch out of "in cancel" when done.  In the worst case this
        // will defer provider cleanup to the state destructor, which is the natural
        // place for it anyway.  Anything else here is just an optimization to get the
        // provider cleaned up sooner (the destructor location may be delayed until the
        // completion callback fires, since it's hanging on to a state object ref).

        if (TrySetProviderCleanup(state, ProviderCleanupLocation::InCancel))
        {
            (void)state->provider(XAsyncOp::Cancel, &state->providerData);
            RevertProviderCleanup(state, ProviderCleanupLocation::InCancel);
        }
    }
}

/// <summary>
/// Runs the given callback asynchronously.
/// </summary>
STDAPI XAsyncRun(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_ XAsyncWork* work
    ) noexcept
{
    RETURN_IF_FAILED(XAsyncBegin(
        asyncBlock,
        reinterpret_cast<void*>(work),
        reinterpret_cast<void*>(XAsyncRun),
        __FUNCTION__,
        [](XAsyncOp op, const XAsyncProviderData* data)
    {
        switch (op)
        {
            case XAsyncOp::Begin:
                return XAsyncSchedule(data->async, 0);
                
            case XAsyncOp::DoWork:
                {
                    XAsyncWork* work = reinterpret_cast<XAsyncWork*>(data->context);
                    HRESULT hr = work(data->async);
                    XAsyncComplete(data->async, hr, 0);
                }
                break;

            case XAsyncOp::Cancel:
            case XAsyncOp::Cleanup:
            case XAsyncOp::GetResult:
                break;

        }

        return S_OK;
    }));

    return S_OK;
}

//
// XAsyncProvider APIs
//

/// <summary>
/// Initializes an async block for use.  Once begun calls such
/// as XAsyncGetStatus will provide meaningful data. It is assumed the
/// async work will begin on some system defined thread after this call
/// returns.
/// </summary>
STDAPI XAsyncBegin(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_opt_ void* context,
    _In_opt_ const void* identity,
    _In_opt_ const char* identityName,
    _In_ XAsyncProvider* provider
    ) noexcept
{
    RETURN_IF_FAILED(AllocState(asyncBlock, 0));

    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        state = internal.GetState();
    }

    state->provider = provider;
    state->providerData.context = context;
    state->identity = identity;
    state->identityName = identityName;

    // We've successfully setup the call.  Now kick off a
    // Begin opcode.  If this call fails, we use it to fail
    // the async call, instead of failing XAsyncBegin. This is
    // necessary to ensure that the async call state is properly
    // cleaned up, both for us and for the user call.

    HRESULT hr = provider(XAsyncOp::Begin, &state->providerData);
    if (FAILED(hr))
    {
        XAsyncComplete(asyncBlock, hr, 0);
    }

    return S_OK;
}

/// <summary>
/// Initializes an async block for use.  Once begun calls such
/// as XAsyncGetStatus will provide meaningful data. It is assumed the
/// async work will begin on some system defined thread after this call
/// returns. The token and function parameters can be used to help identify
/// mismatched Begin/GetResult calls.  The token is typically the function
/// pointer of the async API you are implementing, and the functionName parameter
/// is typically the __FUNCTION__ compiler macro.  
///
/// This variant of XAsyncBegin will allocate additional memory of size contextSize
/// and use this as the context pointer for async provider callbacks.  The memory
/// pointer is returned in 'context'.  The lifetime of this memory is managed
/// by the async library and will be freed automatically when the call 
/// completes.
/// </summary>
STDAPI XAsyncBeginAlloc(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_opt_ const void* identity,
    _In_opt_ const char* identityName,
    _In_ XAsyncProvider* provider,
    _In_ size_t contextSize,
    _In_ size_t parameterBlockSize,
    _In_opt_ void* parameterBlock
    ) noexcept
{
    RETURN_HR_IF(E_INVALIDARG, contextSize == 0);

    if (parameterBlockSize != 0)
    {
        RETURN_HR_IF(E_INVALIDARG, parameterBlock == nullptr || parameterBlockSize > contextSize);
    }
    else
    {
        RETURN_HR_IF(E_INVALIDARG, parameterBlock != nullptr);
    }

    RETURN_IF_FAILED(AllocState(asyncBlock, contextSize));

    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        state = internal.GetState();
    }

    // Alloc using a context size should always fail and not
    // try to send a completion.
    ASSERT(state != nullptr);

    state->provider = provider;
    state->identity = identity;
    state->identityName = identityName;

    ASSERT(state->providerData.context != nullptr);
    memset(state->providerData.context, 0, contextSize);

    if (parameterBlockSize != 0)
    {
        memcpy(state->providerData.context, parameterBlock, parameterBlockSize);
    }

    // We've successfully setup the call.  Now kick off a
    // Begin opcode.  If this call fails, we use it to fail
    // the async call, instead of failing XAsyncBegin. This is
    // necessary to ensure that the async call state is properly
    // cleaned up, both for us and for the user call.

    HRESULT hr = provider(XAsyncOp::Begin, &state->providerData);

    if (FAILED(hr))
    {
        XAsyncComplete(asyncBlock, hr, 0);
    }

    return S_OK;
}

/// <summary>
/// Schedules a callback to do async work.  Calling this is optional.  If the async work can be done
/// through a system - async mechanism like overlapped IO or async COM, there is no need to schedule
/// work.  If work should be scheduled after a delay, pass the number of ms XAsyncSchedule should wait
/// before it schedules work.
/// </summary>
STDAPI XAsyncSchedule(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_ uint32_t delayInMs
    ) noexcept
{
    HRESULT existingStatus;
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        state = internal.GetState();
        existingStatus = internal.GetStatus();
    }

    // If the call already failed, return that failure code.
    RETURN_HR_IF(existingStatus, FAILED(existingStatus) && existingStatus != E_PENDING);

    // If the call has completed, the state will be null.  
    RETURN_HR_IF(E_INVALIDARG, state == nullptr);

    bool priorScheduled = false;
    state->workScheduled.compare_exchange_strong(priorScheduled, true);

    if (priorScheduled)
    {
        RETURN_HR(E_UNEXPECTED);
    }

    RETURN_IF_FAILED(XTaskQueueSubmitDelayedCallback(
        state->queue,
        XTaskQueuePort::Work,
        delayInMs,
        state.Get(),
        WorkerCallback));

    // NOTE: The callback now owns the state ref.  It could have run
    // already, and state may be holding a dead pointer.  Regardless,
    // state should be detached here as it no longer owns
    // the ref.  
    state.Detach();
    return S_OK;
}

/// <summary>
/// Called when async work is completed and the results can be returned.
/// The caller should supply the resulting data payload size.  If the call
/// has no data payload, pass zero.
/// </summary
STDAPI_(void) XAsyncComplete(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_ HRESULT result,
    _In_ size_t requiredBufferSize
    ) noexcept
{
    // E_PENDING is special -- if you still have work to do don't complete.

    if (result == E_PENDING)
    {
        return;
    }

    bool completedNow = false;
    bool doCleanup = false;
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };

        completedNow = internal.TrySetTerminalStatus(result);

        // If the required buffer is zero, there is no payload and we need to
        // clean up now. Also clean up if the call failed.  The caller should
        // have passed zero in here in that case but be tolerant for cases like
        // sizeof().
        if ((requiredBufferSize == 0 || FAILED(result)) && completedNow)
        {
            // If we are going to cleanup steal the reference from the block.
            doCleanup = true;
            requiredBufferSize = 0;
            state = internal.ExtractState();
        }
        else
        {
            state = internal.GetState();
        }

        if (completedNow)
        {
            state->providerData.bufferSize = requiredBufferSize;
        }
    }

    // Only signal / adjust needed buffer size if we were first to complete.
    if (completedNow)
    {
        FAIL_FAST_IF_FAILED(SignalCompletion(state));
    }

    // At this point asyncBlock may be unsafe to touch. As we've cleaned up
    // state we will mark the state so that the DoWork callback calls
    // the Cleanup op on the provider.  This gets it cleaned up sooner
    // so it doesn't have to wait for the task queue to process it.

    if (doCleanup)
    {
        (void)TrySetProviderCleanup(state, ProviderCleanupLocation::AfterDoWork);
        CleanupState(std::move(state));
    }
}

/// <summary>
/// Returns the result data for the asynchronous operation.  After this call
/// the async block is completed and no longer associated with the
/// operation.
/// </summary>
STDAPI XAsyncGetResult(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_opt_ const void* identity,
    _In_ size_t bufferSize,
    _Out_writes_bytes_to_opt_(bufferSize, *bufferUsed) void* buffer,
    _Out_opt_ size_t* bufferUsed
    ) noexcept
{
    HRESULT result = E_PENDING;
    AsyncStateRef state;
    bool resultsAlreadyReturned;

    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        result = internal.GetStatus();
        state = internal.GetState();
        resultsAlreadyReturned = internal.GetResultsRetrieved();
    }

    if (SUCCEEDED(result))
    {
        // If the call was successful and we've already returned
        // results, fail now to prevent us interpreting a null
        // state object as a zero payload success.  Note if the
        // call had completed with zero payload it gets cleaned
        // up immediately and therefore a call to XAsyncGetResult
        // never extracts and sets the results as retrieved.  This
        // means you can safely call this multiple times for calls
        // that had no result payload, which is consistent with
        // how XAsyncGetStatus works.  We only want to guard against
        // the case where results were offered once, but can't be
        // offered again now that we've shut the call down.

        RETURN_HR_IF(E_ILLEGAL_METHOD_CALL, resultsAlreadyReturned);

        if (state == nullptr)
        {
            if (bufferUsed != nullptr)
            {
                *bufferUsed = 0;
            }
        }
        else if (identity != state->identity)
        {
            // Call/Result mismatch.  This XAsyncBlock was initiated by state->identityName
            char buf[100];
            int sprintfResult;
            if (state->identityName != nullptr)
            {
                sprintfResult = snprintf(
                    buf,
                    sizeof(buf),
                    "Call/Result mismatch.  This XAsyncBlock was initiated by '%s'.\r\n",
                    state->identityName);
            }
            else
            {
                sprintfResult = snprintf(buf, sizeof(buf), "Call/Result mismatch\r\n");
            }

            result = E_INVALIDARG;
            ASSERT(false);
            ASSERT(sprintfResult > 0);
        }
        else if (state->providerData.bufferSize == 0)
        {
            // Caller has not supplied a payload
            result = E_NOT_SUPPORTED;
        }
        else if (buffer == nullptr)
        {
            result = E_INVALIDARG;
        }
        else if (bufferSize < state->providerData.bufferSize)
        {
            result = E_NOT_SUFFICIENT_BUFFER;
        }
        else
        {
            if (bufferUsed != nullptr)
            {
                *bufferUsed = state->providerData.bufferSize;
            }

            state->providerData.bufferSize = bufferSize;
            state->providerData.buffer = buffer;
            result = state->provider(XAsyncOp::GetResult, &state->providerData);
        }
    }

    // Cleanup state if needed.
    if (result != E_PENDING && state != nullptr)
    {
        {
            AsyncBlockInternalGuard internal{ asyncBlock };
            internal.ExtractState(true);
        }

        CleanupState(std::move(state));
    }

    return result;
}
