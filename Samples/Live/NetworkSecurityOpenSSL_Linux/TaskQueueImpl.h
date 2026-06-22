// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "AtomicVector.h"
#include "LocklessQueue.h"
#include "StaticArray.h"
#include "ThreadPool.h"
#include "WaitTimer.h"

namespace ApiDiag
{
    void GlobalAddRef();
    void GlobalRelease();
}

template <ApiId iid, typename TInterface>
class Api : public TInterface
{
public:

    virtual ~Api() 
    {
    }
    
    uint32_t __stdcall AddRef()
    {
        ApiDiag::GlobalAddRef();
        return m_refs++;
    }

    uint32_t __stdcall Release()
    {
        ApiDiag::GlobalRelease();
        uint32_t refs = --m_refs;

        // Note: rundown may addref/release as it
        // progresses, so we guard agaisnt redundant
        // deletes with m_deleting.

        if (refs == 0 && m_deleting.test_and_set() == false)
        {
            RundownObject();
            delete this;
        }
        return refs;
    }

    HRESULT __stdcall QueryApi(ApiId id, void** ptr)
    {
        if (ptr == nullptr)
        {
            return E_POINTER;
        }

        *ptr = QueryApiImpl(id);

        if ((*ptr) != nullptr)
        {
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

protected:

    virtual void* QueryApiImpl(ApiId id) 
    {
        if (id == ApiId::Identity || id == iid)
        {
            return static_cast<TInterface*>(this);
        }
        return nullptr;
    }
    
    // Called when the object is "likely" about to be
    // deleted.  There is no guarantee that an object
    // will be deleted after this call.
    virtual void RundownObject()
    {
    }
    
private:
    std::atomic<uint32_t> m_refs{ 0 };
    std::atomic_flag m_deleting;
};

static uint32_t const SUBMIT_CALLBACK_MAX = 32;

class SubmitCallback
{
public:

    SubmitCallback(_In_ XTaskQueueHandle queue);

    HRESULT Register(_In_opt_ void* context, _In_ XTaskQueueMonitorCallback* callback, _Out_ XTaskQueueRegistrationToken* token);
    void Unregister(_In_ XTaskQueueRegistrationToken token);
    void Invoke(_In_ XTaskQueuePort port);

private:

    struct CallbackRegistration
    {
        uint64_t Token;
        void* Context;
        XTaskQueueMonitorCallback* Callback;
    };

    std::atomic<uint64_t> m_nextToken{ 0 };
    std::mutex m_lock;
    CallbackRegistration m_buffer1[SUBMIT_CALLBACK_MAX];
    CallbackRegistration m_buffer2[SUBMIT_CALLBACK_MAX];
    CallbackRegistration* m_buffers[2]= { m_buffer1, m_buffer2 };
    std::atomic<uint32_t> m_indexAndRef { 0 };
    XTaskQueueHandle m_queue;
};

#define PORT_WAIT_MAX 60
#define PORT_EVENT_MAX (PORT_WAIT_MAX + 1)
#define QUEUE_WAIT_MAX (PORT_WAIT_MAX * 2)

class QueueWaitRegistry
{
public:

    HRESULT Register(
        _In_ XTaskQueuePort port,
        _In_ const XTaskQueueRegistrationToken& portToken,
        _Out_ XTaskQueueRegistrationToken* token);

    std::pair<XTaskQueuePort, XTaskQueueRegistrationToken> Unregister(
        _In_ const XTaskQueueRegistrationToken& token);

private:

    struct WaitRegistration
    {
        uint64_t Token;
        uint64_t PortToken;
        XTaskQueuePort Port;
    };

    std::atomic<uint64_t> m_nextToken{ 0 };
    StaticArray<WaitRegistration, QUEUE_WAIT_MAX> m_callbacks;
    std::mutex m_lock;
};

class TaskQueuePortImpl: public Api<ApiId::TaskQueuePort, ITaskQueuePort>
{
public:

    TaskQueuePortImpl();
    virtual ~TaskQueuePortImpl();

    HRESULT Initialize(
        _In_ XTaskQueueDispatchMode mode);

    XTaskQueuePortHandle __stdcall GetHandle() { return &m_header; }

    HRESULT __stdcall QueueItem(
        _In_ ITaskQueuePortContext* portContext,
        _In_ uint32_t waitMs,
        _In_opt_ void* callbackContext,
        _In_ XTaskQueueCallback* callback);

    HRESULT __stdcall RegisterWaitHandle(
        _In_ ITaskQueuePortContext* portContext,
        _In_ HANDLE waitHandle,
        _In_opt_ void* callbackContext,
        _In_ XTaskQueueCallback* callback,
        _Out_ XTaskQueueRegistrationToken* token);

    void __stdcall UnregisterWaitHandle(
        _In_ XTaskQueueRegistrationToken token);

    HRESULT __stdcall PrepareTerminate(
        _In_ ITaskQueuePortContext* portContext,
        _In_ void* callbackContext,
        _In_ XTaskQueueTerminatedCallback* callback,
        _Out_ void** token);

    void __stdcall CancelTermination(
        _In_ void* token);

    void __stdcall Terminate(
        _In_ void* token);

    virtual HRESULT __stdcall Attach(
        _In_ ITaskQueuePortContext* portContext);

    void __stdcall Detach(
        _In_ ITaskQueuePortContext* portContext);
    
    bool __stdcall DrainOneItem();

    bool __stdcall Wait(
        _In_ ITaskQueuePortContext* portContext,
        _In_ uint32_t timeout);

    bool __stdcall IsEmpty();

    HRESULT __stdcall SuspendTermination(
        _In_ ITaskQueuePortContext* portContext);

    void __stdcall ResumeTermination(
        _In_ ITaskQueuePortContext* portContext);

private:

    struct WaitRegistration;

    struct QueueEntry
    {
        ITaskQueuePortContext* portContext;
        void* callbackContext;
        XTaskQueueCallback* callback;
        WaitRegistration* waitRegistration;
        uint64_t enqueueTime;
        uint64_t id;
    };

    struct TerminationEntry
    {
        ITaskQueuePortContext* portContext;
        void* callbackContext;
        XTaskQueueTerminatedCallback* callback;
        std::uint64_t node;
    };

#ifdef _WIN32
    struct WaitRegistration
    {
        uint64_t token;
        HANDLE waitHandle;
        PTP_WAIT threadpoolWait;
        TaskQueuePortImpl* port;
        QueueEntry queueEntry;
        std::atomic_flag appended;
        std::atomic<uint32_t> refs;
        bool deleted;
    };
#endif

    XTaskQueuePortObject m_header = { };
    XTaskQueueDispatchMode m_dispatchMode = XTaskQueueDispatchMode::Manual;
    AtomicVector<ITaskQueuePortContext*> m_attachedContexts;
    std::atomic<uint32_t> m_processingCallback{ 0 };
    std::mutex m_lock;
    std::unique_ptr<LocklessQueue<QueueEntry>> m_queueList;
    std::unique_ptr<LocklessQueue<QueueEntry>> m_pendingList;
    std::unique_ptr<LocklessQueue<TerminationEntry*>> m_terminationList;
    std::unique_ptr<LocklessQueue<TerminationEntry*>> m_pendingTerminationList;
    OS::WaitTimer m_timer;
    OS::ThreadPool m_threadPool;
    std::atomic<uint64_t> m_timerDue = { UINT64_MAX };
    std::atomic<uint64_t> m_nextId = { 0 };

#ifdef _WIN32
    StaticArray<WaitRegistration*, PORT_WAIT_MAX> m_waits;
    StaticArray<HANDLE, PORT_EVENT_MAX> m_events;
    uint64_t m_nextWaitToken = 0;
#else
    std::condition_variable_any m_event;
#endif

    HRESULT VerifyNotTerminated(_In_ ITaskQueuePortContext* portContext);
    
    bool IsCallCanceled(_In_ const QueueEntry& entry);

    // Appends the given entry to the active queue.  The entry should already
    // be add-refd.
    bool AppendEntry(
        _In_ const QueueEntry& entry,
        _In_opt_ uint64_t node = 0,
        _In_ bool signal = true);

    void CancelPendingEntries(
        _In_ ITaskQueuePortContext* portContext,
        _In_ bool appendToQueue);

    static void EraseQueue(
        _In_opt_ LocklessQueue<QueueEntry>* queue);

    bool ScheduleNextPendingCallback(
        _In_ uint64_t dueTime,
        _Out_ QueueEntry& dueEntry,
        _Out_ uint64_t& dueEntryNode);

    void SubmitPendingCallback();

    void SignalTerminations();
    void ScheduleTermination(_In_ TerminationEntry* term);

    void SignalQueue();

    void ProcessThreadPoolCallback(_In_ OS::ThreadPoolActionComplete& complete);

#ifdef _WIN32
    HRESULT InitializeWaitRegistration(
        _In_ WaitRegistration* waitReg);

    bool AppendWaitRegistrationEntry(
        _In_ WaitRegistration* waitReg,
        _In_ bool signal = true);

    bool ReleaseWaitRegistration(
        _In_ WaitRegistration* waitReg);

    void ProcessWaitCallback(
        _In_ WaitRegistration* waitReg);

    static void CALLBACK WaitCallback(
        _In_ PTP_CALLBACK_INSTANCE instance,
        _Inout_opt_ void* context,
        _Inout_ PTP_WAIT wait,
        _In_ TP_WAIT_RESULT waitResult);
#endif
};

class TaskQueuePortContextImpl : public ITaskQueuePortContext
{
public:
    
    TaskQueuePortContextImpl(
        _In_ ITaskQueue* queue,
        _In_ XTaskQueuePort type,
        _In_ SubmitCallback* submitCallback);
    
    uint32_t __stdcall AddRef() override;
    uint32_t __stdcall Release() override;
    HRESULT __stdcall QueryApi(_In_ ApiId id, _Out_ void** ptr) override;

    XTaskQueuePort __stdcall GetType() override;
    TaskQueuePortStatus __stdcall GetStatus() override;
    ITaskQueue* __stdcall GetQueue() override;
    ITaskQueuePort* __stdcall GetPort() override;
    
    bool __stdcall TrySetStatus(
        _In_ TaskQueuePortStatus expectedStatus,
        _In_ TaskQueuePortStatus status) override;
    
    void __stdcall SetStatus(
        _In_ TaskQueuePortStatus status) override;

    void __stdcall ItemQueued() override;

    bool __stdcall AddSuspend() override;
    bool __stdcall RemoveSuspend() override;

    referenced_ptr<ITaskQueuePort> Port;
    referenced_ptr<ITaskQueue> Source;

private:
    
    ITaskQueue* m_queue = nullptr;
    XTaskQueuePort m_type = XTaskQueuePort::Work;
    SubmitCallback* m_submitCallback = nullptr;
    std::atomic<TaskQueuePortStatus> m_status = { TaskQueuePortStatus::Active };
    std::atomic<uint32_t> m_suspendCount = { 0 };
};

class TaskQueueImpl : public Api<ApiId::TaskQueue, ITaskQueue>
{
public:

    TaskQueueImpl();
    virtual ~TaskQueueImpl();

    HRESULT Initialize(
        _In_ XTaskQueueDispatchMode workMode,
        _In_ XTaskQueueDispatchMode completionMode,
        _In_ bool allowTermination,
        _In_ bool allowClose);
    
    HRESULT Initialize(
        _In_ XTaskQueuePortHandle workPort,
        _In_ XTaskQueuePortHandle completionPort);
    
    XTaskQueueHandle __stdcall GetHandle() override { return &m_header; }

    HRESULT __stdcall GetPortContext(
        _In_ XTaskQueuePort port,
        _Out_ ITaskQueuePortContext** portContext) override;
    
    HRESULT __stdcall RegisterWaitHandle(
        _In_ XTaskQueuePort port,
        _In_ HANDLE waitHandle,
        _In_opt_ void* callbackContext,
        _In_ XTaskQueueCallback* callback,
        _Out_ XTaskQueueRegistrationToken* token) override;

    void __stdcall UnregisterWaitHandle(
        _In_ XTaskQueueRegistrationToken token) override;

    HRESULT __stdcall RegisterSubmitCallback(
        _In_opt_ void* context,
        _In_ XTaskQueueMonitorCallback* callback,
        _Out_ XTaskQueueRegistrationToken* token) override;
    
    void __stdcall UnregisterSubmitCallback(
        _In_ XTaskQueueRegistrationToken token) override;

    bool __stdcall CanTerminate() override;
    bool __stdcall CanClose() override;

    HRESULT __stdcall Terminate(
        _In_ bool wait, 
        _In_opt_ void* callbackContext, 
        _In_opt_ XTaskQueueTerminatedCallback* callback)override ;

protected:

    void RundownObject() override;

private:

    static void CALLBACK OnTerminationCallback(_In_ void* context);

private:

    enum class TerminationLevel
    {
        None,
        Work,
        Completion
    };
    
    struct TerminationEntry
    {
        TaskQueueImpl* owner;
        TerminationLevel level;
        void* completionPortToken;
        void* context;
        XTaskQueueTerminatedCallback* callback;
    };

    struct TerminationData
    {
        bool allowed;
        bool terminated;
        std::mutex lock;
        std::condition_variable cv;
    };
    
    XTaskQueueObject m_header = { };
    SubmitCallback m_callbackSubmitted;
    QueueWaitRegistry m_waitRegistry;
    TerminationData m_termination;
    TaskQueuePortContextImpl m_work;
    TaskQueuePortContextImpl m_completion;
    bool m_allowClose;
};

inline ITaskQueue* GetQueue(XTaskQueueHandle handle)
{
    if (handle->m_signature != TASK_QUEUE_SIGNATURE)
    {
        ASSERT("Invalid XTaskQueueHandle");
        return nullptr;
    }
    
    return handle->m_queue;
}
