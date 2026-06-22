// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
// This is the "private" api for a task queue.  It is an immutable definition used to help define
// the implementation of a queue so that multiple implementations can co-habitate and share the same
// flat public API.
#pragma once

enum class ApiId
{
    Identity,
    TaskQueuePort,
    TaskQueue,
    TaskQueuePortContext
};

// Lightweight version of IUnknown
struct IApi
{
    virtual uint32_t __stdcall AddRef() = 0;
    virtual uint32_t __stdcall Release() = 0;
    virtual HRESULT __stdcall QueryApi(_In_ ApiId id, _Out_ void** ptr) = 0;
};

struct ITaskQueue;
struct ITaskQueuePort;
struct ITaskQueuePortContext;

// A queue port is either the work or completion component of
// a task queue
struct ITaskQueuePort: IApi
{
    virtual XTaskQueuePortHandle __stdcall GetHandle() = 0;

    virtual HRESULT __stdcall QueueItem(
        _In_ ITaskQueuePortContext* portContext,
        _In_ uint32_t waitMs,
        _In_opt_ void* callbackContext,
        _In_ XTaskQueueCallback* callback) = 0;

    virtual HRESULT __stdcall RegisterWaitHandle(
        _In_ ITaskQueuePortContext* portContext,
        _In_ HANDLE waitHandle,
        _In_opt_ void* callbackContext,
        _In_ XTaskQueueCallback* callback,
        _Out_ XTaskQueueRegistrationToken* token) = 0;

    virtual void __stdcall UnregisterWaitHandle(
        _In_ XTaskQueueRegistrationToken token) = 0;

    virtual HRESULT __stdcall PrepareTerminate(
        _In_ ITaskQueuePortContext* portContext,
        _In_ void* callbackContext,
        _In_ XTaskQueueTerminatedCallback* callback,
        _Out_ void** token) = 0;

    virtual void __stdcall CancelTermination(
        _In_ void* token) = 0;

    virtual void __stdcall Terminate(
        _In_ void* token) = 0;

    virtual HRESULT __stdcall Attach(
        _In_ ITaskQueuePortContext* portContext) = 0;
    
    virtual void __stdcall Detach(
        _In_ ITaskQueuePortContext* portContext) = 0;

    virtual bool __stdcall DrainOneItem() = 0;

    virtual bool __stdcall Wait(
        _In_ ITaskQueuePortContext* portContext,
        _In_ uint32_t timeout) = 0;

    virtual bool __stdcall IsEmpty() = 0;

    virtual HRESULT __stdcall SuspendTermination(
        _In_ ITaskQueuePortContext* portContext) = 0;

    virtual void __stdcall ResumeTermination(
        _In_ ITaskQueuePortContext* portContext) = 0;
};

// The status of a port on the queue.
enum class TaskQueuePortStatus
{
    Active,
    Canceled,
    Terminated
};

// A task queue port context contains queue-specific data about a port.
// Interface pointer return values from this interface are not
// add-ref'd.
struct ITaskQueuePortContext : IApi
{
    virtual XTaskQueuePort __stdcall GetType() = 0;
    virtual TaskQueuePortStatus __stdcall GetStatus() = 0;
    virtual ITaskQueue* __stdcall GetQueue() = 0;
    virtual ITaskQueuePort* __stdcall GetPort() = 0;
    
    virtual bool __stdcall TrySetStatus(
        _In_ TaskQueuePortStatus expectedStatus,
        _In_ TaskQueuePortStatus status) = 0;
    
    virtual void __stdcall SetStatus(
        _In_ TaskQueuePortStatus status) = 0;

    virtual void __stdcall ItemQueued() = 0;

    virtual bool __stdcall AddSuspend() = 0;
    virtual bool __stdcall RemoveSuspend() = 0;
};

// The task queue.  The public flat API is built entirely on
// this primitive.
struct ITaskQueue : IApi
{
    virtual XTaskQueueHandle __stdcall GetHandle() = 0;
    
    virtual HRESULT __stdcall GetPortContext(
        _In_ XTaskQueuePort port,
        _Out_ ITaskQueuePortContext** portContext) = 0;
    
    virtual HRESULT __stdcall RegisterWaitHandle(
        _In_ XTaskQueuePort port,
        _In_ HANDLE waitHandle,
        _In_opt_ void* callbackContext,
        _In_ XTaskQueueCallback* callback,
        _Out_ XTaskQueueRegistrationToken* token) = 0;

    virtual void __stdcall UnregisterWaitHandle(
        _In_ XTaskQueueRegistrationToken token) = 0;

    virtual HRESULT __stdcall RegisterSubmitCallback(
        _In_opt_ void* context,
        _In_ XTaskQueueMonitorCallback* callback,
        _Out_ XTaskQueueRegistrationToken* token) = 0;

    virtual void __stdcall UnregisterSubmitCallback(
        _In_ XTaskQueueRegistrationToken token) = 0;

    virtual bool __stdcall CanTerminate() = 0;
    virtual bool __stdcall CanClose() = 0;

    virtual HRESULT __stdcall Terminate(
        _In_ bool wait, 
        _In_opt_ void* callbackContext, 
        _In_opt_ XTaskQueueTerminatedCallback* callback) = 0;         
};

// Defines the structure that backs a XTaskQueueHandle.  This structure can never
// change, nor can its signature.
struct XTaskQueueObject
{
    uint32_t m_signature;
    ITaskQueue* m_queue;
};

// Ditto for backing XTaskQueuePortHandle
struct XTaskQueuePortObject
{
    uint32_t m_signature;
    ITaskQueuePort* m_port;
    ITaskQueue* m_queue;
};

static uint32_t const TASK_QUEUE_SIGNATURE = 0x41515545;
static uint32_t const TASK_QUEUE_PORT_SIGNATURE = 0x41515553;
