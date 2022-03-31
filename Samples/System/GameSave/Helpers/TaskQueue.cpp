//--------------------------------------------------------------------------------------
// TaskQueue.cpp
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "ScopedLockWrappers.h"
#include "TaskQueue.h"

class SingleCoreAsyncTaskQueue
{
    enum class State {
        NotInitialized,
        Running,
        Terminating,
        Terminated
    };

    std::atomic<State> state;
    HANDLE completionThread;
    HANDLE workThread;
    XTaskQueueHandle taskQueue;
    uint32_t affinitizedCore;

public:
    // Constructor
    SingleCoreAsyncTaskQueue(size_t core);

    // Move & copy constructors
    SingleCoreAsyncTaskQueue(const SingleCoreAsyncTaskQueue& copyFrom) = delete;
    SingleCoreAsyncTaskQueue& operator=(const SingleCoreAsyncTaskQueue& copyFrom) = delete;
    SingleCoreAsyncTaskQueue(SingleCoreAsyncTaskQueue&& moveFrom) = delete;
    SingleCoreAsyncTaskQueue& operator=(SingleCoreAsyncTaskQueue&& moveFrom) = delete;

    // Destructor
    ~SingleCoreAsyncTaskQueue();

    // Starts the threads.
    void Start();

    // Terminates this queue.
    void TerminateQueue();

    // Waits for the thread to end.
    void Wait();

    // Returns the underlying task queue handle.
    XTaskQueueHandle GetAsyncTaskQueue() const noexcept
    {
        return taskQueue;
    }

private:
    unsigned int RunWork();
    unsigned int RunCompletion();

    static unsigned int __stdcall StaticWorkThreadProc(void* context);
    static unsigned int __stdcall StaticCompletionThreadProc(void* context);
    static void CALLBACK OnTaskQueueTerminated(void* context);
};

namespace {
    SRWLOCK s_threadQueueLock = SRWLOCK_INIT;
    std::atomic<uint64_t> s_ThreadInitCallCount(0ULL);
    std::unique_ptr<SingleCoreAsyncTaskQueue> s_AsyncWorkerThreads[ATG::MAX_THREAD_CORES];
}

namespace ATG
{
    // Creates an Async Task Queue on all of the CPU cores, processing its callbacks and work on the same core.

    void InitializeThreadQueues()
    {
        ScopedExclusiveLock exclusive(s_threadQueueLock);
        uint64_t prevCount = s_ThreadInitCallCount.fetch_add(1ULL);
        if (prevCount == 0) // We were the first!
        {
            for (size_t i = 0; i < ATG::MAX_THREAD_CORES; ++i)
            {
                s_AsyncWorkerThreads[i] = std::make_unique<SingleCoreAsyncTaskQueue>(i);
            }

            for (auto& asyncWorkerThread : s_AsyncWorkerThreads)
            {
                asyncWorkerThread->Start();
            }
        }
    }

    // Gets the Async Task Queue for a specific core. Does not duplicate the handle. If you need a duplicate, create
    // one yourself using the returned value. Do not close the returned value yourself.

    XTaskQueueHandle GetThreadQueueForCore(uint16_t coreIndex)
    {
        ATG::ScopedSharedLock shared(s_threadQueueLock);
        assert(s_ThreadInitCallCount.load() > 0 && "Not initialized yet");
        assert(s_AsyncWorkerThreads[coreIndex] && "Index contents were null");
        return s_AsyncWorkerThreads[coreIndex]->GetAsyncTaskQueue();
    }

    // Terminates all of the task queues created by InitializeThreadQueues, and waits for the threads to cleanly
    // shut down.

    void ShutdownThreadQueues()
    {
        ScopedExclusiveLock exclusive(s_threadQueueLock);
        uint64_t prevCount = s_ThreadInitCallCount.fetch_sub(1ULL);

        if (prevCount == 1) // Now 0, so no-one should own an outstanding reference.
        {
            for (auto& asyncWorkerThread : s_AsyncWorkerThreads)
            {
                asyncWorkerThread->TerminateQueue();
            }

            for (auto& asyncWorkerThread : s_AsyncWorkerThreads)
            {
                asyncWorkerThread->Wait();
                asyncWorkerThread.reset(nullptr);
            }
        }
    }
}

SingleCoreAsyncTaskQueue::SingleCoreAsyncTaskQueue(size_t core) :
    state(State::NotInitialized),
    completionThread(nullptr),
    workThread(nullptr),
    taskQueue(nullptr),
    affinitizedCore(0xFFFFFFFF)
{
    affinitizedCore = (uint32_t)core;

    HRESULT hr = XTaskQueueCreate(XTaskQueueDispatchMode::Manual, XTaskQueueDispatchMode::Manual, &taskQueue);
    DX::ThrowIfFailed(hr);

    // NOTE: We don't use std::thread, as std::thread's can't be created suspended. This means you can't affinitize
    // them to another core without disturbing another core first. Instead, we use Win32 so that we can:
    //   - Create the thread suspended.
    //   - Set its thread affinity to the appropriate core.
    //   - Resume the thread so that it starts to run.

    uint32_t threadAffinity = 1u << core;
    void* context = (void*)this;

    completionThread = (HANDLE)_beginthreadex(nullptr, 0, &StaticCompletionThreadProc, context, CREATE_SUSPENDED, nullptr);
    if (!completionThread)
    {
        DX::ThrowLastError();
    }

    wchar_t temp[100];
    swprintf_s(temp, L"SingleCoreAsyncTaskQueue completion thread [0x%04x on core %u]", GetThreadId(completionThread), (uint32_t)affinitizedCore);
    ::SetThreadDescription(completionThread, temp);

    workThread = (HANDLE)_beginthreadex(nullptr, 0, &StaticWorkThreadProc, context, CREATE_SUSPENDED, nullptr);

    if (!workThread)
    {
        DX::ThrowLastError();
    }

    swprintf_s(temp, L"SingleCoreAsyncTaskQueue worker thread [0x%04x, on core %u]", GetThreadId(workThread), (uint32_t)affinitizedCore);
    ::SetThreadDescription(workThread, temp);

    ::SetThreadAffinityMask(completionThread, threadAffinity);
    ::SetThreadAffinityMask(workThread, threadAffinity);
}

SingleCoreAsyncTaskQueue::~SingleCoreAsyncTaskQueue()
{
    if (taskQueue != nullptr)
    {
        assert(state.load(std::memory_order_seq_cst) == State::Terminated
            && "Attempting to destroy task queue before it has been properly terminated");

        XTaskQueueCloseHandle(taskQueue);
        taskQueue = nullptr;
    }

    if (workThread != nullptr)
    {
        ::CloseHandle(workThread);
        workThread = nullptr;
    }

    if (completionThread != nullptr)
    {
        ::CloseHandle(completionThread);
        completionThread = nullptr;
    }
}

void SingleCoreAsyncTaskQueue::Start()
{
    assert(workThread != nullptr && completionThread != nullptr && state.load() == State::NotInitialized);
    ::ResumeThread(workThread);
    ::ResumeThread(completionThread);

    // Transition from NotInitialized --> Running
    State expected = State::NotInitialized;
    if (!state.compare_exchange_strong(expected, State::Running))
    {
#ifdef _DEBUG
        assert(expected == State::NotInitialized && "Thread wasn't in uninitialized state");
#else
        __assume(0);
#endif
    }
}

void SingleCoreAsyncTaskQueue::TerminateQueue()
{
    //NOTE: This function could be called from any thread.
    State localstate = state.load();

    assert(state != State::NotInitialized && "Terminate called, but queue has not been initialized");

    // If we're already terminating, return.

    if (localstate == State::Terminated || localstate == State::Terminating)
        return;

    // Transition from Running --> Terminating.
    State expectedState = State::Running;
    if (!state.compare_exchange_strong(expectedState, State::Terminating))
    {
#ifdef _DEBUG
        assert(expectedState == State::Running && "Unexpected state.");
#else
        __assume(0);
#endif
    }

    // Shut down the task queue.

    XTaskQueueTerminate(taskQueue, false, this, OnTaskQueueTerminated);

    //NOTE: the callback will be run on the Completion thread.
}

void CALLBACK SingleCoreAsyncTaskQueue::OnTaskQueueTerminated(void* context)
{
    auto queue = static_cast<SingleCoreAsyncTaskQueue*>(context);

    // Transition from Terminating --> Terminated.
    State expectedState = State::Terminating;
    if (!queue->state.compare_exchange_strong(expectedState, State::Terminated))
    {
#ifdef _DEBUG
        assert(expectedState == State::Terminating && "We didn't transition from Terminating to Terminated - this shouldn't happen.");
#else
        __assume(0);
#endif
    }
}

unsigned int __stdcall SingleCoreAsyncTaskQueue::StaticWorkThreadProc(void* context)
{
    SingleCoreAsyncTaskQueue* instance = reinterpret_cast<SingleCoreAsyncTaskQueue*>(context);
    return instance->RunWork();
}

unsigned int SingleCoreAsyncTaskQueue::RunWork()
{
    while (XTaskQueueDispatch(taskQueue, XTaskQueuePort::Work, INFINITE));
    return 0;
}

unsigned int __stdcall SingleCoreAsyncTaskQueue::StaticCompletionThreadProc(void* context)
{
    SingleCoreAsyncTaskQueue* instance = reinterpret_cast<SingleCoreAsyncTaskQueue*>(context);
    return instance->RunCompletion();
}

unsigned int SingleCoreAsyncTaskQueue::RunCompletion()
{
    while (XTaskQueueDispatch(taskQueue, XTaskQueuePort::Completion, INFINITE));
    return 0;
}

void SingleCoreAsyncTaskQueue::Wait()
{
    assert(state.load() != State::NotInitialized && "Not initialized yet");

    //NOTE: It's valid to Wait before Terminate is called, as long as you Terminate from another thread at some point.

    HANDLE handles[] = {
       completionThread,
       workThread
    };

    DWORD result = ::WaitForMultipleObjectsEx(2, handles, TRUE, INFINITE, FALSE);

#ifdef _DEBUG
    assert(result >= WAIT_OBJECT_0 && result <= (WAIT_OBJECT_0 + 1) && "Unexpected result from WaitForMultipleObjectsEx");
#else
    std::ignore = result;
#endif
}
