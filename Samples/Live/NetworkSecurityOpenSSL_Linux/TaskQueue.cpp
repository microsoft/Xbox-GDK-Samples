// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include "pch.h"
#include "XTaskQueue.h"
#include "referenced_ptr.h"
#include "TaskQueueP.h"
#include "TaskQueueImpl.h"

//
// Note:  ApiDiag is only used for reference count validation during
//        unit tests.  Otherwise, g_globalApiRefs is unused.
//
namespace ApiDiag
{
    std::atomic<uint32_t> g_globalApiRefs { 0 };
    
    void GlobalAddRef()
    {
        g_globalApiRefs++;
    }
    
    void GlobalRelease()
    {
        g_globalApiRefs--;
    }
}

namespace ProcessGlobals
{
    const XTaskQueueHandle g_invalidQueueHandle = (XTaskQueueHandle)(-1);
    std::atomic<XTaskQueueHandle> g_defaultProcessQueue = { g_invalidQueueHandle };
    std::atomic<XTaskQueueHandle> g_processQueue = { g_invalidQueueHandle };
}

//
// SubmitCallback
//

SubmitCallback::SubmitCallback(
    _In_ XTaskQueueHandle queue)
    : m_queue(queue)
{
    memset(m_buffer1, 0, sizeof(m_buffer1));
    memset(m_buffer2, 0, sizeof(m_buffer2));
}

HRESULT SubmitCallback::Register(_In_opt_ void* context, _In_ XTaskQueueMonitorCallback* callback, _Out_ XTaskQueueRegistrationToken* token)
{
    RETURN_HR_IF(E_POINTER, callback == nullptr || token == nullptr);

    token->token = 0;

    std::lock_guard<std::mutex> lock(m_lock);
    uint32_t bufferReadIdx = (m_indexAndRef & 0x80000000 ? 1 : 0);
    uint32_t bufferWriteIdx = 1 - bufferReadIdx;

    for(uint32_t idx = 0; idx < ARRAYSIZE(m_buffer1); idx++)
    {
        if (token->token == 0 && m_buffers[bufferReadIdx][idx].Callback == nullptr)
        {
            token->token = ++m_nextToken;
            m_buffers[bufferWriteIdx][idx].Token = token->token;
            m_buffers[bufferWriteIdx][idx].Context = context;
            m_buffers[bufferWriteIdx][idx].Callback = callback;
        }
        else
        {
            m_buffers[bufferWriteIdx][idx] = m_buffers[bufferReadIdx][idx];
        }
    }

    RETURN_HR_IF(E_OUTOFMEMORY, token->token == 0);

    // Now spin wait to swap the active buffer.
    uint32_t expected = bufferReadIdx << 31;
    uint32_t desired = bufferWriteIdx << 31;

    for (;;)
    {
        uint32_t expectedSwap = expected;
        if (m_indexAndRef.compare_exchange_weak(expectedSwap, desired))
        {
            break;
        }
    }
    
    return S_OK;
}

void SubmitCallback::Unregister(_In_ XTaskQueueRegistrationToken token)
{
    std::lock_guard<std::mutex> lock(m_lock);
    uint32_t bufferReadIdx = (m_indexAndRef & 0x80000000 ? 1 : 0);
    uint32_t bufferWriteIdx = 1 - bufferReadIdx;

    for(uint32_t idx = 0; idx < ARRAYSIZE(m_buffer1); idx++)
    {
        if (m_buffers[bufferReadIdx][idx].Token == token.token)
        {
            m_buffers[bufferWriteIdx][idx].Callback = nullptr;
        }
        else
        {
            m_buffers[bufferWriteIdx][idx] = m_buffers[bufferReadIdx][idx];
        }
    }

    // Now spin wait to swap the active buffer.
    uint32_t expected = bufferReadIdx << 31;
    uint32_t desired = bufferWriteIdx << 31;

    for (;;)
    {
        uint32_t expectedSwap = expected;
        if (m_indexAndRef.compare_exchange_weak(expectedSwap, desired))
        {
            break;
        }
    }
}

void SubmitCallback::Invoke(_In_ XTaskQueuePort port)
{
    uint32_t indexAndRef = ++m_indexAndRef;
    uint32_t bufferIdx = (indexAndRef & 0x80000000 ? 1 : 0);

    for(uint32_t idx = 0; idx < ARRAYSIZE(m_buffer1); idx++)
    {
        if (m_buffers[bufferIdx][idx].Callback != nullptr)
        {
            m_buffers[bufferIdx][idx].Callback(
                m_buffers[bufferIdx][idx].Context,
                m_queue,
                port);
        }
    }

    m_indexAndRef--;
}

//
// QueueWaitRegistry
//

HRESULT QueueWaitRegistry::Register(
    _In_ XTaskQueuePort port,
    _In_ const XTaskQueueRegistrationToken& portToken,
    _Out_ XTaskQueueRegistrationToken* token)
{
    RETURN_HR_IF(E_OUTOFMEMORY, m_callbacks.capacity() == 0);

    std::lock_guard<std::mutex> lock(m_lock);

    WaitRegistration reg = { };
    reg.Port = port;
    reg.Token = ++m_nextToken;
    reg.PortToken = portToken.token;
    token->token = reg.Token;
    m_callbacks.append(reg);

    return S_OK;
}

std::pair<XTaskQueuePort, XTaskQueueRegistrationToken> QueueWaitRegistry::Unregister(
    _In_ const XTaskQueueRegistrationToken& token)
{
    XTaskQueuePort port = XTaskQueuePort::Work;
    XTaskQueueRegistrationToken portToken;
    portToken.token = 0;

    std::lock_guard<std::mutex> lock(m_lock);

    for(uint32_t idx = 0; idx < m_callbacks.count(); idx++)
    {
        if (m_callbacks[idx].Token == token.token)
        {
            port = m_callbacks[idx].Port;
            portToken.token = m_callbacks[idx].PortToken;
            m_callbacks.removeAt(idx);
            break;
        }
    }

    return std::pair<XTaskQueuePort, XTaskQueueRegistrationToken>(port, portToken);
}

//
// TaskQueuePortImpl
//

TaskQueuePortImpl::TaskQueuePortImpl()
    : Api()
{
    m_header.m_signature = TASK_QUEUE_PORT_SIGNATURE;
    m_header.m_port = this;
    m_header.m_queue = nullptr;
}

TaskQueuePortImpl::~TaskQueuePortImpl()
{
    m_timer.Terminate();

    EraseQueue(m_queueList.get());
    EraseQueue(m_pendingList.get());

#ifdef _WIN32
    StaticArray<WaitRegistration*, PORT_WAIT_MAX> waits;

    // We have no control over when these event callbacks
    // fire, so we must guard during destruction.
    {
        std::lock_guard<std::mutex> lock(m_lock);
        waits = m_waits;
        m_waits.clear();
    }

    if (waits.count() != 0)
    {
        for (uint32_t idx = 0; idx < waits.count(); idx++)
        {
            if (waits[idx]->threadpoolWait != nullptr)
            {
                SetThreadpoolWait(waits[idx]->threadpoolWait, nullptr, nullptr);
                WaitForThreadpoolWaitCallbacks(waits[idx]->threadpoolWait, TRUE);
                CloseThreadpoolWait(waits[idx]->threadpoolWait);
            }

            // Note: queue entries that are parked on the wait registration
            // don't take a reference on the port context, so don't release here.
            delete waits[idx];
        }
    }

#endif

    m_threadPool.Terminate();

#ifdef _WIN32
    if (m_events.count() != 0)
    {
        // Other events in here are owned by the caller.
        CloseHandle(m_events[0]);
    }
#endif

    // Destroy these lists in order, since the pending lsit shares
    // the node heap with the queue list.
    
    m_pendingList.reset();
    m_queueList.reset();
}

HRESULT TaskQueuePortImpl::Initialize(
    _In_ XTaskQueueDispatchMode mode)
{
    m_dispatchMode = mode;

    m_queueList.reset(new (std::nothrow) LocklessQueue<QueueEntry>);
    RETURN_IF_NULL_ALLOC(m_queueList);

    // Lockless lists can share their underlying heaps.  This both reduces
    // memory costs and allows node addresses from one list to be transferred
    // to another.  We use this frequently when transferring data across
    // lists to eliminate the possibility of allocation failures.

    m_pendingList.reset(new (std::nothrow) LocklessQueue<QueueEntry>(*m_queueList.get()));
    RETURN_IF_NULL_ALLOC(m_pendingList);

    // There should be very few simultaneous termination requests, so we can set the
    // default heap size to the minimum.

    m_terminationList.reset(new (std::nothrow) LocklessQueue<TerminationEntry*>(0));
    RETURN_IF_NULL_ALLOC(m_terminationList);

    m_pendingTerminationList.reset(new (std::nothrow) LocklessQueue<TerminationEntry*>(*m_terminationList.get()));
    RETURN_IF_NULL_ALLOC(m_pendingTerminationList);

    RETURN_IF_FAILED(m_timer.Initialize(this, [](void* context)
    {
        TaskQueuePortImpl* pthis = static_cast<TaskQueuePortImpl*>(context);
        pthis->SubmitPendingCallback();
    }));

#ifdef _WIN32
    HANDLE evt = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    RETURN_LAST_ERROR_IF_NULL(evt);
    m_events.append(evt);
#endif

    switch (mode)
    {
    case XTaskQueueDispatchMode::Manual:
        // nothing
        break;

    case XTaskQueueDispatchMode::ThreadPool:
    case XTaskQueueDispatchMode::SerializedThreadPool:
        RETURN_IF_FAILED(m_threadPool.Initialize(this, [](void* context, OS::ThreadPoolActionComplete& complete)
        {
            TaskQueuePortImpl* pthis = static_cast<TaskQueuePortImpl*>(context);
            pthis->ProcessThreadPoolCallback(complete);
        }));
        break;
          
    case XTaskQueueDispatchMode::Immediate:
        // nothing
        break;
    }

    return S_OK;
}

HRESULT __stdcall TaskQueuePortImpl::QueueItem(
    _In_ ITaskQueuePortContext* portContext,
    _In_ uint32_t waitMs,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback)
{
    RETURN_IF_FAILED(VerifyNotTerminated(portContext));

    QueueEntry entry;
    referenced_ptr<ITaskQueuePortContext> portContextHolder(portContext);

    entry.portContext = portContext;
    entry.callback = callback;
    entry.callbackContext = callbackContext;
    entry.waitRegistration = nullptr;
    entry.id = m_nextId++;

    if (waitMs == 0)
    {
        entry.enqueueTime = 0;
        RETURN_HR_IF(E_OUTOFMEMORY, !AppendEntry(entry));
    }
    else
    {
        entry.enqueueTime = m_timer.GetAbsoluteTime(waitMs);
        RETURN_HR_IF(E_OUTOFMEMORY, !m_pendingList->push_back(entry));

        // If the entry's enqueue time is < our current time,
        // update the timer.
        while (true)
        {
            uint64_t due = m_timerDue;
            if (entry.enqueueTime < due)
            {
                if (m_timerDue.compare_exchange_weak(due, entry.enqueueTime))
                {
                    m_timer.Start(entry.enqueueTime);
                    break;
                }
            }
            else if (m_timerDue.compare_exchange_weak(due, due))
            {
                break;
            }
        }
    }

    // QueueEntry now owns the ref.
    portContextHolder.release();
    return S_OK;
}

HRESULT __stdcall TaskQueuePortImpl::RegisterWaitHandle(
    _In_ ITaskQueuePortContext* portContext,
    _In_ HANDLE waitHandle,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token)
{
    RETURN_HR_IF(E_INVALIDARG, callback == nullptr || waitHandle == nullptr || token == nullptr);
    RETURN_HR_IF(E_POINTER, token == nullptr);
    RETURN_IF_FAILED(VerifyNotTerminated(portContext));

#ifdef _WIN32
    std::unique_ptr<WaitRegistration> waitReg(new (std::nothrow) WaitRegistration);
    RETURN_IF_NULL_ALLOC(waitReg);

    // Entries in waitReg get a port context, but it is not
    // addref'd because a web registration does not keep the
    // queue alive.  When a wait registration queue entry is
    // added to the queue the port context will be addref'd.
    waitReg->queueEntry.portContext = portContext;
    waitReg->queueEntry.callback = callback;
    waitReg->queueEntry.callbackContext = callbackContext;
    waitReg->queueEntry.waitRegistration = waitReg.get();
    waitReg->queueEntry.id = m_nextId++;

    waitReg->waitHandle = waitHandle;
    waitReg->token = ++m_nextWaitToken;
    waitReg->port = this;
    waitReg->threadpoolWait = nullptr;
    waitReg->appended.clear();
    waitReg->refs = 1;
    waitReg->deleted = false;

    std::lock_guard<std::mutex> lock(m_lock);
    RETURN_HR_IF(E_OUTOFMEMORY, m_events.capacity() == 0);
    RETURN_HR_IF(E_OUTOFMEMORY, m_waits.capacity() == 0);

    RETURN_IF_FAILED(InitializeWaitRegistration(waitReg.get()));

    m_events.append(waitHandle);
    m_waits.append(waitReg.get());

    token->token = waitReg->token;

    waitReg.release();

    // The queue may be waiting on our handle array, but
    // we've just changed the array so the wait needs to
    // fetch it again.
    SignalQueue();

    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

void __stdcall TaskQueuePortImpl::UnregisterWaitHandle(
    _In_ XTaskQueueRegistrationToken token)
{
#ifdef _WIN32
    WaitRegistration* toDelete = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        for(uint32_t idx = 0; idx < m_waits.count(); idx++)
        {
            if (m_waits[idx]->token == token.token)
            {
                toDelete = m_waits[idx];
                toDelete->deleted = true;
                m_waits.removeAt(idx);
                break;
            }
        }

        if (toDelete != nullptr)
        {
            // Remove the handle from our event list
            for (uint32_t idx = 1; idx < m_events.count(); idx++)
            {
                if (m_events[idx] == toDelete->waitHandle)
                {
                    m_events.removeAt(idx);
                    break;
                }
            }
        }
    }

    if (toDelete != nullptr)
    {
        SetThreadpoolWait(toDelete->threadpoolWait, nullptr, nullptr);
        WaitForThreadpoolWaitCallbacks(toDelete->threadpoolWait, TRUE);
        CloseThreadpoolWait(toDelete->threadpoolWait);
        ReleaseWaitRegistration(toDelete);
    }

    // The queue may be waiting on our handle array, but
    // we've just changed the array so the wait needs to
    // fetch it again.
    SignalQueue();

#else
    UNREFERENCED_PARAMETER(token);
#endif
}

HRESULT __stdcall TaskQueuePortImpl::PrepareTerminate(
    _In_ ITaskQueuePortContext* portContext,
    _In_ void* callbackContext,
    _In_ XTaskQueueTerminatedCallback* callback,
    _Out_ void** token)
{
    RETURN_HR_IF(E_POINTER, token == nullptr);
    RETURN_HR_IF(E_INVALIDARG, callback == nullptr);

    std::unique_ptr<TerminationEntry> term(new (std::nothrow) TerminationEntry);
    RETURN_IF_NULL_ALLOC(term);

    RETURN_HR_IF(E_OUTOFMEMORY, !m_terminationList->reserve_node(term->node));

    term->callbackContext = callbackContext;
    term->callback = callback;
    term->portContext = portContext;

    // Mark the port as canceled, but don't overwrite
    // terminating or terminated status.
    portContext->TrySetStatus(TaskQueuePortStatus::Active, TaskQueuePortStatus::Canceled);
    *token = term.release();

    return S_OK;
}

void __stdcall TaskQueuePortImpl::CancelTermination(
    _In_ void* token)
{
    TerminationEntry* term = static_cast<TerminationEntry*>(token);
    term->portContext->TrySetStatus(TaskQueuePortStatus::Canceled, TaskQueuePortStatus::Active);

    if (term->node != 0)
    {
        m_terminationList->free_node(term->node);
    }

    delete term;
}

void __stdcall TaskQueuePortImpl::Terminate(
    _In_ void* token)
{
    TerminationEntry* term = static_cast<TerminationEntry*>(token);
    referenced_ptr<ITaskQueuePortContext> cxt(term->portContext);

    // Prevent anything else from coming into the queue
    cxt->SetStatus(TaskQueuePortStatus::Terminated);

    CancelPendingEntries(cxt.get(), true);

    // Are there existing suspends?  AddSuspend returns
    // true if this is the first suspend added.
    if (cxt->AddSuspend())
    {
        ScheduleTermination(term);
    }
    else
    {
        m_pendingTerminationList->push_back(term, term->node);
        term->node = 0;
    }

    // Balance our add.  Note we must use ResumeTermination
    // here so we schedule the termination if this is the
    // last remove.
    ResumeTermination(cxt.get());
}

HRESULT __stdcall TaskQueuePortImpl::Attach(
    _In_ ITaskQueuePortContext* portContext)
{
    return m_attachedContexts.Add(portContext);
}

void __stdcall TaskQueuePortImpl::Detach(
    _In_ ITaskQueuePortContext* portContext)
{
    CancelPendingEntries(portContext, false);
    m_attachedContexts.Remove([portContext](ITaskQueuePortContext* compare)
    {
        return portContext == compare;
    });
}

bool __stdcall TaskQueuePortImpl::DrainOneItem()
{
    m_processingCallback++;
    QueueEntry entry;
    bool popped = false;

    if (m_queueList->pop_front(entry))
    {
        popped = true;
        entry.callback(entry.callbackContext, IsCallCanceled(entry));
        m_processingCallback--;

#ifdef _WIN32
        // If this entry has a wait registration, it needs
        // to be reinitialized as we mark it to only execute
        // once. When the queue entry was aded the wait registration
        // was addref'd, so release it. If not destroyed it
        // should be reinitialized.
        if (entry.waitRegistration != nullptr)
        {
            std::lock_guard<std::mutex> lock(m_lock);            
            if (!ReleaseWaitRegistration(entry.waitRegistration))
            {
                InitializeWaitRegistration(entry.waitRegistration);
            }            
        }

#endif
        entry.portContext->Release();
    }
    else
    {
        m_processingCallback--;
    }

    if (m_queueList->empty())
    {
        SignalQueue();
        SignalTerminations();
    }

    return popped;
}

bool __stdcall TaskQueuePortImpl::Wait(
    _In_ ITaskQueuePortContext* portContext,
    _In_ uint32_t timeout)
{
#ifdef _WIN32
    while(true)
    {
        StaticArray<HANDLE, PORT_EVENT_MAX> events;

        {
            std::lock_guard<std::mutex> lock(m_lock);
            events = m_events;
        }

        DWORD waitResult = WaitForMultipleObjects(events.count(), events.data(), FALSE, timeout);

        if (waitResult > WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + events.count())
        {
            // One of our waiters was signaled.  Find it, and then process it.
            std::lock_guard<std::mutex> lock(m_lock);
            for (uint32_t idx = 0; idx < m_waits.count(); idx++)
            {
                if (m_waits[idx]->waitHandle == events[waitResult - WAIT_OBJECT_0])
                {
                    if (!AppendWaitRegistrationEntry(m_waits[idx]))
                    {
                        // If we fail adding to the queue, re-initialize our wait
                        LOG_IF_FAILED(InitializeWaitRegistration(m_waits[idx]));
                    }
                    break;
                }
            }
        }
        else if (waitResult == WAIT_OBJECT_0)
        {
            // We are using event 0 like a condition variable.  It's
            // auto reset, so if nothing is in the queue we continue
            // waiting.
            if (portContext->GetStatus() == TaskQueuePortStatus::Terminated || !m_queueList->empty())
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

#else
    while (m_queueList->empty() && portContext->GetStatus() != TaskQueuePortStatus::Terminated)
    {
        std::unique_lock<std::mutex> lock(m_lock);
        if (m_event.wait_for(lock, std::chrono::milliseconds(timeout)) == std::cv_status::timeout)
        {
            break;
        }
    }
#endif

    return !m_queueList->empty() || !m_terminationList->empty();
}

bool __stdcall TaskQueuePortImpl::IsEmpty()
{
    bool empty =
        (m_queueList->empty()) &&
        (m_pendingList->empty()) &&
        (m_processingCallback == 0);

    return empty;
}

HRESULT __stdcall TaskQueuePortImpl::SuspendTermination(
    _In_ ITaskQueuePortContext* portContext
    )
{
    // This is necessary to ensure there is no race.
    portContext->AddSuspend();
    HRESULT hr = VerifyNotTerminated(portContext);
    
    if (FAILED(hr))
    {
        portContext->RemoveSuspend();
        RETURN_HR(hr);
    }

    return S_OK;
}

void __stdcall TaskQueuePortImpl::ResumeTermination(
    _In_ ITaskQueuePortContext* portContext)
{
    if (portContext->RemoveSuspend())
    {
        // Removed the last external callback.  Look for
        // parked terminations and reschedule them.

        m_pendingTerminationList->remove_if([&](auto& entry, auto address)
        {
            if (entry->portContext == portContext)
            {
                // This entry is for the port that's resuming,
                // we can schedule it.
                entry->node = address;
                ScheduleTermination(entry);
                return true;
            }

            return false;
        });
    }
}

HRESULT TaskQueuePortImpl::VerifyNotTerminated(
    _In_ ITaskQueuePortContext* portContext)
{
    // N.B.  This looks wrong but it's not.  We only error adding new items
    // if we are terminating or terminated.  If we're just canceled we allow
    // new items in but we invoke them with the canceled flag set to true.
    RETURN_HR_IF(E_ABORT, portContext->GetStatus() > TaskQueuePortStatus::Canceled);
    return S_OK;
}

bool TaskQueuePortImpl::IsCallCanceled(_In_ const QueueEntry& entry)
{
    return entry.portContext->GetStatus() != TaskQueuePortStatus::Active;
}

// Appends the given entry to the active queue.  The entry should already
// be add-refd. This will return false on failure.
bool TaskQueuePortImpl::AppendEntry(
    _In_ const QueueEntry& entry,
    _In_opt_ uint64_t node,
    _In_ bool signal)
{
    if (node != 0)
    {
        m_queueList->push_back(entry, node);
    }
    else if (!m_queueList->push_back(entry))
    {
        return false;
    }

    if (signal)
    {
        SignalQueue();
    }

    switch (m_dispatchMode)
    {
    case XTaskQueueDispatchMode::Manual:
        // nothing
        break;

    case XTaskQueueDispatchMode::SerializedThreadPool:
    case XTaskQueueDispatchMode::ThreadPool:
        m_threadPool.Submit();
        break;

    case XTaskQueueDispatchMode::Immediate:
        // We will handle this after we invoke
        // callback submitted.
        break;
    }

    m_attachedContexts.Visit([](ITaskQueuePortContext* portContext)
    {
        portContext->ItemQueued();
    });
    
    if (m_dispatchMode == XTaskQueueDispatchMode::Immediate)
    {
        DrainOneItem();
    }

    return true;
}

void TaskQueuePortImpl::CancelPendingEntries(
    _In_ ITaskQueuePortContext* portContext,
    _In_ bool appendToQueue)
{
    // Stop wait timer and promote pending callbacks that are used
    // by the queue that invoked this termination. Other callbacks
    // are placed back on the pending list.
    
    m_timer.Cancel();
    m_timerDue = UINT64_MAX;

    m_pendingList->remove_if([&](auto& entry, auto address)
    {
        if (entry.portContext == portContext)
        {
            if (!appendToQueue || !AppendEntry(entry, address))
            {
                entry.portContext->Release();
                m_pendingList->free_node(address);
            }

            return true;
        }

        return false;
    });

    SubmitPendingCallback();
    
#ifdef _WIN32
    
    // Abort any registered waits and promote their entries too.
    // Wait registration is not lock free.
    std::unique_lock<std::mutex> lock(m_lock);
    
    for (uint32_t index = m_waits.count(); index > 0; index--)
    {
        uint32_t idx = index - 1;
        if (m_waits[idx]->queueEntry.portContext == portContext)
        {
            CloseThreadpoolWait(m_waits[idx]->threadpoolWait);
            m_waits[idx]->queueEntry.waitRegistration = nullptr;

            if (appendToQueue)
            {
                AppendWaitRegistrationEntry(m_waits[idx]);
            }
            
            delete m_waits[idx];
            m_waits.removeAt(idx);
        }
    }
    lock.unlock();
    
#endif
}

void TaskQueuePortImpl::EraseQueue(
    _In_opt_ LocklessQueue<QueueEntry>* queue)
{
    if (queue != nullptr)
    {
        QueueEntry entry;
        
        while(queue->pop_front(entry))
        {
            entry.portContext->Release();
        }
    }
}

// Examines the pending callback list, optionally popping the entry off the
// list that matches m_timerDue, and schedules the timer for the next entry.
bool TaskQueuePortImpl::ScheduleNextPendingCallback(
    _In_ uint64_t dueTime,
    _Out_ QueueEntry& dueEntry,
    _Out_ uint64_t& dueEntryNode)
{
    QueueEntry nextItem = {};
    bool hasDueEntry = false;
    bool hasNextItem = false;

    dueEntryNode = 0;

    m_pendingList->remove_if([&](auto& entry, auto address)
    {
        if (!hasDueEntry && entry.enqueueTime == dueTime)
        {
            dueEntry = entry;
            dueEntryNode = address;
            hasDueEntry = true;
            return true;
        }
        else if (!hasNextItem || nextItem.enqueueTime > entry.enqueueTime)
        {
            nextItem = entry;
            hasNextItem = true;
        }

        return false;
    });

    if (hasNextItem)
    {
        while (true)
        {
            if (m_timerDue.compare_exchange_weak(dueTime, nextItem.enqueueTime))
            {
                m_timer.Start(nextItem.enqueueTime);
                break;
            }

            dueTime = m_timerDue.load();

            if (dueTime <= nextItem.enqueueTime)
            {
                break;
            }
        }
    }
    else
    {
        uint64_t noDueTime = UINT64_MAX;
        if (m_timerDue.compare_exchange_strong(dueTime, noDueTime))
        {
            m_timer.Cancel();
        }
    }

    return hasDueEntry;
}

void TaskQueuePortImpl::SubmitPendingCallback()
{
    QueueEntry dueEntry;
    uint64_t dueEntryNode;
    
    if (ScheduleNextPendingCallback(m_timerDue.load(), dueEntry, dueEntryNode))
    {
        if (!AppendEntry(dueEntry, dueEntryNode))
        {
            dueEntry.portContext->Release();
            m_queueList->free_node(dueEntryNode);
        }
    }
}

// Called from thread pool callback
void TaskQueuePortImpl::ProcessThreadPoolCallback(_In_ OS::ThreadPoolActionComplete& complete)
{
    referenced_ptr<ITaskQueuePort> ref(this);
    uint32_t wasProcessing = m_processingCallback++;
    if (m_dispatchMode == XTaskQueueDispatchMode::SerializedThreadPool)
    {
        if (wasProcessing == 0)
        {
            while (DrainOneItem());
        }
    }
    else
    {
        DrainOneItem();
    }
    m_processingCallback--;

    // Important that this comes before our release (which is implicit
    // in the raii referenced_ptr wrapper).
    complete();
}

void TaskQueuePortImpl::SignalQueue()
{
#ifdef _WIN32
    SetEvent(m_events[0]);
#else
    m_event.notify_all();
#endif
}

void TaskQueuePortImpl::SignalTerminations()
{
    m_terminationList->remove_if([this](auto& entry, auto address)
    {
        if (entry->portContext->GetStatus() == TaskQueuePortStatus::Terminated)
        {
            entry->callback(entry->callbackContext);
            m_terminationList->free_node(address);
            delete entry;
            return true;
        }

        return false;
    });
}

void TaskQueuePortImpl::ScheduleTermination(
    _In_ TerminationEntry* term)
{
    // Insert the termination callback into the queue.  Even if the
    // main queue is empty, we still signal it and run through
    // a cycle.  This ensures we flush the queue out with no 
    // races and that the termination callback happens on the right
    // thread.

    // This never fails because we preallocate the
    // list node.
    m_terminationList->push_back(term, term->node);
    term->node = 0; // now owned by the list

    // We will not signal until we are marked as terminated. The queue could
    // still be moving while we are running this terminate call.

    SignalQueue();

    // We must ensure we poke the queue threads in case there's
    // nothing submitted
    switch (m_dispatchMode)
    {
    case XTaskQueueDispatchMode::SerializedThreadPool:
    case XTaskQueueDispatchMode::ThreadPool:
        m_threadPool.Submit();
        break;

    default:
        break;
    }

    m_attachedContexts.Visit([](ITaskQueuePortContext* portContext)
    {
        portContext->ItemQueued();
    });
    
    if (m_dispatchMode == XTaskQueueDispatchMode::Immediate)
    {
        DrainOneItem();
    }
}

#ifdef _WIN32
void CALLBACK TaskQueuePortImpl::WaitCallback(
    _In_ PTP_CALLBACK_INSTANCE instance,
    _Inout_opt_ void* context,
    _Inout_ PTP_WAIT wait,
    _In_ TP_WAIT_RESULT waitResult)
{
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(wait);

    if (waitResult == WAIT_OBJECT_0 && context != nullptr)
    {
        WaitRegistration* waitReg = static_cast<WaitRegistration*>(context);
        waitReg->port->ProcessWaitCallback(waitReg);
    }
}

HRESULT TaskQueuePortImpl::InitializeWaitRegistration(
    _In_ WaitRegistration* waitReg)
{
    waitReg->appended.clear();

    if (waitReg->threadpoolWait == nullptr)
    {
        waitReg->threadpoolWait = CreateThreadpoolWait(WaitCallback, waitReg, nullptr);
        RETURN_LAST_ERROR_IF_NULL(waitReg->threadpoolWait);
    }

    SetThreadpoolWait(waitReg->threadpoolWait, waitReg->waitHandle, nullptr);

    return S_OK;
}

// Appends the queue entry of the wait registration to the queue
// if it has not already been done. This can addref the queue
// entry. Returns true if it appended or found nothing to do.
// Returns false if it needed to append but failed. On failure this
// correctly releases the queue entry.
bool TaskQueuePortImpl::AppendWaitRegistrationEntry(
    _In_ WaitRegistration* waitReg,
    _In_ bool signal)
{
    // Prepare the queue entry for insert
    QueueEntry entry = waitReg->queueEntry;
    bool success = true;

    if (waitReg->appended.test_and_set() == false)
    {
        entry.portContext->AddRef();
        waitReg->refs++;

        success = AppendEntry(entry, 0, signal);
        if (!success)
        {
            entry.portContext->Release();
            waitReg->refs--;
        }
    }

    return success;
}

bool TaskQueuePortImpl::ReleaseWaitRegistration(
    _In_ WaitRegistration* waitReg)
{
    bool deleted = waitReg->deleted;

    if (--waitReg->refs == 0)
    {
        delete waitReg;
        return true;
    }

    // We return true for the final release if the reg
    // is being deleted. This prevents a race during
    // reinit.
    return deleted;
}

// Called from thread pool when a wait is satisfied
void TaskQueuePortImpl::ProcessWaitCallback(
    _In_ WaitRegistration* waitReg)
{
    if (!AppendWaitRegistrationEntry(waitReg))
    {
        LOG_IF_FAILED(InitializeWaitRegistration(waitReg));
    }
}

#endif

//
// TaskQueuePortContextImpl
//

TaskQueuePortContextImpl::TaskQueuePortContextImpl(
    _In_ ITaskQueue* queue,
    _In_ XTaskQueuePort type,
    _In_ SubmitCallback* submitCallback) :
    m_queue(queue),
    m_type(type),
    m_submitCallback(submitCallback)
{
}
    
uint32_t __stdcall TaskQueuePortContextImpl::AddRef()
{
    return m_queue->AddRef();
}

uint32_t __stdcall TaskQueuePortContextImpl::Release()
{
    return m_queue->Release();
}

HRESULT __stdcall TaskQueuePortContextImpl::QueryApi(_In_ ApiId id, _Out_ void** ptr)
{
    if (ptr == nullptr)
    {
        return E_POINTER;
    }

    if (id == ApiId::Identity || id == ApiId::TaskQueuePortContext)
    {
        *ptr = static_cast<ITaskQueuePortContext*>(this);
        AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}
    
XTaskQueuePort __stdcall TaskQueuePortContextImpl::GetType()
{
    return m_type;
}

TaskQueuePortStatus __stdcall TaskQueuePortContextImpl::GetStatus()
{
    return m_status;
}

ITaskQueue* __stdcall TaskQueuePortContextImpl::GetQueue()
{
    return m_queue;
}

ITaskQueuePort* __stdcall TaskQueuePortContextImpl::GetPort()
{
    return Port.get();
}

bool __stdcall TaskQueuePortContextImpl::TrySetStatus(
    _In_ TaskQueuePortStatus expectedStatus,
    _In_ TaskQueuePortStatus status)
{
    return m_status.compare_exchange_strong(expectedStatus, status);
}
    
void __stdcall TaskQueuePortContextImpl::SetStatus(
    _In_ TaskQueuePortStatus status)
{
    m_status = status;
}

void __stdcall TaskQueuePortContextImpl::ItemQueued()
{
    m_submitCallback->Invoke(m_type);
}

bool __stdcall TaskQueuePortContextImpl::AddSuspend()
{
    return (m_suspendCount.fetch_add(1) == 0);
}

bool __stdcall TaskQueuePortContextImpl::RemoveSuspend()
{
    for(;;)
    {
        uint32_t current = m_suspendCount.load();

        // These should always be balanced and there is no
        // valid case where this should happen, even
        // for multiple threads racing.

        if (current == 0)
        {
            ASSERT(false);
            return true;
        }

        if (m_suspendCount.compare_exchange_weak(current, current -1))
        {
            return current == 1;
        }
    }
}

//
// TaskQueueImpl
//

TaskQueueImpl::TaskQueueImpl() :
    Api(),
    m_callbackSubmitted(&m_header),
    m_work(this, XTaskQueuePort::Work, &m_callbackSubmitted),
    m_completion(this, XTaskQueuePort::Completion, &m_callbackSubmitted),
    m_allowClose(true)
{
    m_header.m_signature = TASK_QUEUE_SIGNATURE;
    m_header.m_queue = this;

    m_termination.allowed = true;
    m_termination.terminated = false;
}

TaskQueueImpl::~TaskQueueImpl()
{
    // Zero the header so we get an early warning of using
    // a released object.
    m_header = {};
}

HRESULT TaskQueueImpl::Initialize(
    _In_ XTaskQueueDispatchMode workMode,
    _In_ XTaskQueueDispatchMode completionMode,
    _In_ bool allowTermination,
    _In_ bool allowClose)
{
    m_termination.allowed = allowTermination;
    m_allowClose = allowClose;

    referenced_ptr<TaskQueuePortImpl> work(new (std::nothrow) TaskQueuePortImpl);
    RETURN_IF_NULL_ALLOC(work);
    RETURN_IF_FAILED(work->Initialize(workMode));

    referenced_ptr<TaskQueuePortImpl> completion(new (std::nothrow) TaskQueuePortImpl);
    RETURN_IF_NULL_ALLOC(completion);
    RETURN_IF_FAILED(completion->Initialize(completionMode));
    
    work->GetHandle()->m_queue = this;
    completion->GetHandle()->m_queue = this;
    
    RETURN_IF_FAILED(work->QueryApi(ApiId::TaskQueuePort, (void**)&m_work.Port));
    RETURN_IF_FAILED(completion->QueryApi(ApiId::TaskQueuePort, (void**)&m_completion.Port));

    RETURN_IF_FAILED(m_work.Port->Attach(&m_work));
    RETURN_IF_FAILED(m_completion.Port->Attach(&m_completion));

    if (!allowClose)
    {
        // This queue will never be closed.  Subtract off the
        // global api refs so tests don't think there's a leak.  We
        // need to subtract off our own ref and the refs of each of our ports.
        ApiDiag::g_globalApiRefs -= 3;
    }

    return S_OK;
}

HRESULT TaskQueueImpl::Initialize(
    _In_ XTaskQueuePortHandle workPort,
    _In_ XTaskQueuePortHandle completionPort)
{
    RETURN_HR_IF(E_INVALIDARG, workPort== nullptr || workPort->m_signature != TASK_QUEUE_PORT_SIGNATURE);
    RETURN_HR_IF(E_INVALIDARG, completionPort == nullptr || completionPort->m_signature != TASK_QUEUE_PORT_SIGNATURE);
    
    m_work.Port = referenced_ptr<ITaskQueuePort>(workPort->m_port);
    m_completion.Port = referenced_ptr<ITaskQueuePort>(completionPort->m_port);
    m_work.Source = referenced_ptr<ITaskQueue>(workPort->m_queue);
    m_completion.Source = referenced_ptr<ITaskQueue>(completionPort->m_queue);

    m_termination.allowed = true;
    m_allowClose = true;
    
    RETURN_IF_FAILED(m_work.Port->Attach(&m_work));
    RETURN_IF_FAILED(m_completion.Port->Attach(&m_completion));

    return S_OK;
}

HRESULT __stdcall TaskQueueImpl::GetPortContext(
    _In_ XTaskQueuePort port,
    _Out_ ITaskQueuePortContext** portContext)
{
    RETURN_HR_IF(E_POINTER, portContext == nullptr);
    
    switch(port)
    {
    case XTaskQueuePort::Work:
        *portContext = &m_work;
        m_work.AddRef();
        break;
        
    case XTaskQueuePort::Completion:
        *portContext = &m_completion;
        m_completion.AddRef();
        break;
        
    default:
        RETURN_HR(E_INVALIDARG);
    }
    
    return S_OK;
}

HRESULT __stdcall TaskQueueImpl::RegisterWaitHandle(
    _In_ XTaskQueuePort port,
    _In_ HANDLE waitHandle,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token)
{
    RETURN_HR_IF(E_POINTER, callback == nullptr || token == nullptr);

    XTaskQueueRegistrationToken portToken;
    referenced_ptr<ITaskQueuePortContext> portContext;

    RETURN_IF_FAILED(GetPortContext(port, portContext.address_of()));
    RETURN_IF_FAILED(portContext->GetPort()->RegisterWaitHandle(
        portContext.get(),
        waitHandle,
        callbackContext,
        callback,
        &portToken));

    HRESULT hr = m_waitRegistry.Register(port, portToken, token);
    if (FAILED(hr))
    {
        portContext->GetPort()->UnregisterWaitHandle(portToken);
    }
    RETURN_IF_FAILED(hr);

    return S_OK;
}

void __stdcall TaskQueueImpl::UnregisterWaitHandle(
    _In_ XTaskQueueRegistrationToken token)
{
    std::pair<XTaskQueuePort, XTaskQueueRegistrationToken> pair = m_waitRegistry.Unregister(token);
    if (pair.second.token != 0)
    {
        referenced_ptr<ITaskQueuePortContext> portContext;
        if (SUCCEEDED(GetPortContext(pair.first, portContext.address_of())))
        {
            portContext->GetPort()->UnregisterWaitHandle(pair.second);
        }
    }
}

HRESULT __stdcall TaskQueueImpl::RegisterSubmitCallback(
    _In_opt_ void* context,
    _In_ XTaskQueueMonitorCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token)
{
    return m_callbackSubmitted.Register(context, callback, token);
}

void __stdcall TaskQueueImpl::UnregisterSubmitCallback(
    _In_ XTaskQueueRegistrationToken token)
{
    m_callbackSubmitted.Unregister(token);
}

bool __stdcall TaskQueueImpl::CanTerminate()
{
    return m_termination.allowed;
}

bool __stdcall TaskQueueImpl::CanClose()
{
    return m_allowClose;
}

HRESULT __stdcall TaskQueueImpl::Terminate(
    _In_ bool wait, 
    _In_opt_ void* callbackContext, 
    _In_opt_ XTaskQueueTerminatedCallback* callback)
{
    RETURN_HR_IF(E_ACCESSDENIED, !m_termination.allowed);

    std::unique_ptr<TerminationEntry> entry(new (std::nothrow) TerminationEntry);
    RETURN_IF_NULL_ALLOC(entry);

    entry->owner = this;
    entry->level = TerminationLevel::Work;
    entry->context = callbackContext;
    entry->callback = callback;

    void* workToken;
    
    RETURN_IF_FAILED(m_work.Port->PrepareTerminate(&m_work, entry.get(), OnTerminationCallback, &workToken));

    HRESULT hr = m_completion.Port->PrepareTerminate(&m_completion, entry.get(), OnTerminationCallback, &entry->completionPortToken);
    if (FAILED(hr))
    {
        m_work.Port->CancelTermination(workToken);
        RETURN_HR(hr);
    }

    // At this point both ports have been marked for termination and have pre-alocated any state they
    // need, so we can proceed with the actual termination.
    
    entry.release();

    // Addref ourself to ensure we don't lose the queue to a close
    // while termination is in flight.  This is released on OnTerminationCallback

    AddRef();

    m_work.Port->Terminate(workToken);
    
    if (wait)
    {
        std::unique_lock<std::mutex> lock(m_termination.lock);
        while(!m_termination.terminated)
        {
            m_termination.cv.wait(lock);
        }
    }

    return S_OK;
}

void TaskQueueImpl::RundownObject()
{
    m_work.SetStatus(TaskQueuePortStatus::Terminated);
    m_completion.SetStatus(TaskQueuePortStatus::Terminated);
    m_work.Port->Detach(&m_work);
    m_completion.Port->Detach(&m_completion);
}

void TaskQueueImpl::OnTerminationCallback(_In_ void* context)
{
    TerminationEntry* entry = static_cast<TerminationEntry*>(context);
    switch(entry->level)
    {
        case TerminationLevel::Work:
            entry->level = TerminationLevel::Completion;
            entry->owner->m_completion.Port->Terminate(entry->completionPortToken);
            break;

        case TerminationLevel::Completion:
            if (entry->callback != nullptr)
            {
                entry->callback(entry->context);
            }

            {
                std::unique_lock<std::mutex> lock(entry->owner->m_termination.lock);
                entry->owner->m_termination.terminated = true;
                entry->owner->m_termination.cv.notify_all();
            }

            entry->owner->Release();
            delete entry;
            break;

        default:
            ASSERT(false);
    }
}

///////////////////
// XTaskQueue.h APIs
///////////////////

//
// Creates a Task Queue, which can be used to queue
// different calls together.
//
STDAPI XTaskQueueCreate(
    _In_ XTaskQueueDispatchMode workDispatchMode,
    _In_ XTaskQueueDispatchMode completionDispatchMode,
    _Out_ XTaskQueueHandle* queue
    ) noexcept
{
    referenced_ptr<TaskQueueImpl> aq(new (std::nothrow) TaskQueueImpl);
    RETURN_IF_NULL_ALLOC(aq);
    RETURN_IF_FAILED(aq->Initialize(
        workDispatchMode, 
        completionDispatchMode, 
        true, /* can terminate */ 
        true /* can close */));
    *queue = aq.release()->GetHandle();
    return S_OK;
}

/// <summary>
/// Returns the task queue port handle for the given
/// port. Task queue port handles are owned by the
/// task queue and do not have to be closed. They are
/// used when creating composite task queues.
/// </summary>
/// <param name='queue'>The task queue to get the port from.</param>
/// <param name='port'>The port to get.</param>
STDAPI XTaskQueueGetPort(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port,
    _Out_ XTaskQueuePortHandle* portHandle
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq == nullptr)
    {
        RETURN_HR(E_INVALIDARG);
    }
    
    referenced_ptr<ITaskQueuePortContext> portContext;
    RETURN_IF_FAILED(aq->GetPortContext(port, portContext.address_of()));
    
    *portHandle = portContext->GetPort()->GetHandle();
    
    return S_OK;
}

/// <summary>
/// Creates a task queue composed of ports of other
/// task queue ports. A composite task queue will duplicate
/// the handles of queues that own the provided ports.
/// </summary>
/// <param name='workPort'>The port to use for queuing work callbacks.</param>
/// <param name='completionPort'>The port to use for queuing completion callbacks.</param>
STDAPI XTaskQueueCreateComposite(
     _In_ XTaskQueuePortHandle workPort,
     _In_ XTaskQueuePortHandle completionPort,
     _Out_ XTaskQueueHandle* queue
     ) noexcept
{
    referenced_ptr<TaskQueueImpl> aq(new (std::nothrow) TaskQueueImpl);
    RETURN_IF_NULL_ALLOC(aq);
    RETURN_IF_FAILED(aq->Initialize(workPort, completionPort));
    *queue = aq.release()->GetHandle();
    return S_OK;
}

//
// Processes items in the task queue of the given type. If an item
// is processed this will return TRUE. If there are no items to process
// this returns FALSE.  You can pass a timeout, which will cause
// XTaskQueueDispatch to wait for something to arrive in the queue.
//
STDAPI_(bool) XTaskQueueDispatch(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port,
    _In_ uint32_t timeoutInMs
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq == nullptr)
    {
        return false;
    }
    
    referenced_ptr<ITaskQueuePortContext> portContext;
    if (FAILED(aq->GetPortContext(port, portContext.address_of())))
    {
        return false;
    }
        
    bool found = portContext->GetPort()->DrainOneItem();
    if (!found && timeoutInMs != 0)
    {
        found = portContext->GetPort()->Wait(portContext.get(), timeoutInMs);
        if (found) portContext->GetPort()->DrainOneItem();
    }

    return found;
}

//
// Returns TRUE if there is no outstanding work in this
// queue.  Note this API is only used for testing and
// is not exposed outside the library.
//
STDAPI_(bool) XTaskQueueIsEmpty(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq == nullptr)
    {
        return false;
    }
    
    referenced_ptr<ITaskQueuePortContext> portContext;
    if (FAILED(aq->GetPortContext(port, portContext.address_of())))
    {
        return false;
    }

    return portContext->GetPort()->IsEmpty();
}

//
// Closes the task queue.  A queue can only be closed if it
// is not in use by a task or is empty.  If not true, the queue
// will be marked for closure and closed when it can. 
//
STDAPI_(void) XTaskQueueCloseHandle(
    _In_ XTaskQueueHandle queue
    ) noexcept
{
    ITaskQueue* aq = GetQueue(queue);
    if (aq != nullptr && aq->CanClose())
    {
        aq->Release();
    }
}

//
// Terminates a task queue by canceling all pending items and
// preventing new items from being queued.  Once a queue is terminated
// its handle can be closed.
//
STDAPI XTaskQueueTerminate(
    _In_ XTaskQueueHandle queue, 
    _In_ bool wait, 
    _In_opt_ void* callbackContext, 
    _In_opt_ XTaskQueueTerminatedCallback* callback
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    return aq->Terminate(wait, callbackContext, callback);
}

//
// Submits either a work or completion callback immediately.
//
STDAPI XTaskQueueSubmitCallback(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback
    ) noexcept
{
    return XTaskQueueSubmitDelayedCallback(queue, port, 0, callbackContext, callback);
}

//
// Submits either a work or completion callback.
//
STDAPI XTaskQueueSubmitDelayedCallback(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port,
    _In_ uint32_t delayMs,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    RETURN_HR_IF(E_INVALIDARG, aq == nullptr);

    referenced_ptr<ITaskQueuePortContext> portContext;
    RETURN_IF_FAILED(aq->GetPortContext(port, portContext.address_of()));

    RETURN_IF_FAILED(portContext->GetPort()->QueueItem(portContext.get(), delayMs, callbackContext, callback));
    return S_OK;
}

//
// Registers a wait handle with the task queue.  When the wait handle
// is satisfied the task queue will invoke the given callback. This
// provides an efficient way to add items to a task queue in 
// response to handles becoming signaled.
//
STDAPI XTaskQueueRegisterWaiter(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port,
    _In_ HANDLE waitHandle,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    RETURN_HR_IF(E_INVALIDARG, aq == nullptr);
    RETURN_IF_FAILED(aq->RegisterWaitHandle(port, waitHandle, callbackContext, callback, token));
    return S_OK;
}

//
// Unregisters a previously registered task queue waiter.
//
STDAPI_(void) XTaskQueueUnregisterWaiter(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueueRegistrationToken token
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq != nullptr)
    {
        aq->UnregisterWaitHandle(token);
    }
}

//
// Increments the refcount on the queue
//
STDAPI XTaskQueueDuplicateHandle(
    _In_ XTaskQueueHandle queueHandle,
    _Out_ XTaskQueueHandle* duplicatedHandle
    ) noexcept
{
    RETURN_HR_IF(E_POINTER, duplicatedHandle == nullptr);

    auto queue = GetQueue(queueHandle);
    RETURN_HR_IF(E_INVALIDARG, queue == nullptr);

    // For queues that cannot be closed, don't add ref, since
    // closing does nothing.
    if (queue->CanClose())
    {
        queue->AddRef();
    }

    *duplicatedHandle = queueHandle;

    return S_OK;
}

//
// Registers a callback that will be called when a new callback
// is submitted. The callback will be directly invoked when
// the call is submitted.
//
STDAPI XTaskQueueRegisterMonitor(
    _In_ XTaskQueueHandle queue,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueMonitorCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    RETURN_HR_IF(E_INVALIDARG, aq == nullptr);
    RETURN_IF_FAILED(aq->RegisterSubmitCallback(callbackContext, callback, token));
    return S_OK;
}

//
// Unregisters a previously added callback.
//
STDAPI_(void) XTaskQueueUnregisterMonitor(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueueRegistrationToken token
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq != nullptr)
    {
        aq->UnregisterSubmitCallback(token);
    }
}

//
// Returns a handle to the process task queue, or nullptr if there is no
// process task queue.  By default, there is a default process task queue
// that uses the thread pool for both work and completion ports.
//
STDAPI_(bool) XTaskQueueGetCurrentProcessTaskQueue(
    _Out_ XTaskQueueHandle* queue
    ) noexcept
{
    XTaskQueueHandle processQueue = ProcessGlobals::g_processQueue;
    if (processQueue == ProcessGlobals::g_invalidQueueHandle)
    {
        XTaskQueueHandle defaultProcessQueue = ProcessGlobals::g_defaultProcessQueue;
        if (defaultProcessQueue == ProcessGlobals::g_invalidQueueHandle)
        {
            // The default process queue hasn't been created yet.  Create it locally
            // then swap it into the atomic.

            referenced_ptr<TaskQueueImpl> aq(new (std::nothrow) TaskQueueImpl);
            if (aq != nullptr && SUCCEEDED(aq->Initialize(
                XTaskQueueDispatchMode::ThreadPool,
                XTaskQueueDispatchMode::ThreadPool,
                false, /* can terminate */
                false /* can close */)))
            {
                XTaskQueueHandle expected = ProcessGlobals::g_invalidQueueHandle;
                if (ProcessGlobals::g_defaultProcessQueue.compare_exchange_strong(
                    expected,
                    aq->GetHandle()))
                {
                    // We successfully set the default handle.
                    aq.release();
                }
            }

            defaultProcessQueue = ProcessGlobals::g_defaultProcessQueue;
        }

        processQueue = defaultProcessQueue;
    }

    if (processQueue == ProcessGlobals::g_invalidQueueHandle)
    {
        processQueue = nullptr;
    }

    if (processQueue != nullptr)
    {
        *queue = processQueue;

        // The default process queue does not addref or release.
        if (processQueue->m_queue->CanClose())
        {
            processQueue->m_queue->AddRef();
        }
    }
    else
    {
        *queue = nullptr;
    }

    return processQueue != nullptr;
}

//
// Sets the given task queue as the process wide task queue.  The
// queue can be set to nullptr, in which case XTaskQueueGetCurrentProcessTaskQueue will
// also return nullptr. The provided queue will have its handle duplicated
// and any existing process task queue will have its handle closed.
//
STDAPI_(void) XTaskQueueSetCurrentProcessTaskQueue(
    _In_ XTaskQueueHandle queue
    ) noexcept
{
    XTaskQueueHandle newQueue = nullptr;
    if (queue != nullptr)
    {
        XTaskQueueDuplicateHandle(queue, &newQueue);
    }

    XTaskQueueHandle previous = ProcessGlobals::g_processQueue.exchange(newQueue);
    if (previous != nullptr && previous != ProcessGlobals::g_invalidQueueHandle)
    {
        XTaskQueueCloseHandle(previous);
    }
}

//
// Submits a callback that will be invoked asynchronously via some external
// system.  This is used to prevent queue termination while other work
// is happening.
//
STDAPI XTaskQueueSuspendTermination(
    _In_ XTaskQueueHandle queue
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    RETURN_HR_IF(E_INVALIDARG, aq == nullptr);

    referenced_ptr<ITaskQueuePortContext> portContext;
    RETURN_IF_FAILED(aq->GetPortContext(XTaskQueuePort::Work, portContext.address_of()));

    RETURN_IF_FAILED(portContext->GetPort()->SuspendTermination(portContext.get()));
    return S_OK;
}

//
// Tells the queue that a previously submitted external callback
// has been dispatched.
//
STDAPI_(void) XTaskQueueResumeTermination(
    _In_ XTaskQueueHandle queue
    ) noexcept
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq == nullptr)
    {
        return;
    }
    
    referenced_ptr<ITaskQueuePortContext> portContext;
    if (FAILED(aq->GetPortContext(XTaskQueuePort::Work, portContext.address_of())))
    {
        return;
    }
        
    portContext->GetPort()->ResumeTermination(portContext.get());
}
