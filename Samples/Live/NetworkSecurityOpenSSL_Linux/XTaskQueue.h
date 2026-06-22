// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if !defined(__cplusplus)
    #error C++11 required
#endif

#pragma once
#include <stdint.h>

extern "C"
{

/// <summary>
/// A task queue contains and dispatches callbacks. A task queue has two ports:  a worker port and
/// a completion port.  Each port can have different rules for how queued calls
/// are dispatched.
/// </summary>
typedef struct XTaskQueueObject* XTaskQueueHandle;

/// <summary>
/// A task queue port represents one port of a task queue.  A port's lifetime is controlled
/// by its owning task queue.
/// </summary>
typedef struct XTaskQueuePortObject* XTaskQueuePortHandle;

/// <summary>
/// Describes how task queue callbacks are processed.
/// </summary>
enum class XTaskQueueDispatchMode : uint32_t
{
    /// <summary>
    /// Callbacks are invoked manually by XTaskQueueDispatch
    /// </summary>
    Manual,

    /// <summary>
    /// Callbacks are queued to the system thread pool and will
    /// be processed in order by the thread pool across multiple thread
    /// pool threads.
    /// </summary>
    ThreadPool,
    
    /// <summary>
    /// Callbacks are queued to the system thread pool and
    /// will be processed one at a time.
    /// </summary>
    SerializedThreadPool,
    
    /// <summary>
    /// Callbacks are not queued at all but are dispatched
    /// immediately by the thread that submits them.
    /// </summary>
    Immediate
};

/// <summary>
/// Declares which port of a task queue to dispatch or submit
/// callbacks to.
/// </summary>
enum class XTaskQueuePort : uint32_t
{
    /// <summary>
    /// Work callbacks
    /// </summary>
    Work,

    /// <summary>
    /// Completion callbacks after work is done
    /// </summary>
    Completion
};

/// <summary>
/// A token returned when registering a callback to identify the registration. This token
/// is later used to unregister the callback.
/// </summary>
struct XTaskQueueRegistrationToken
{
    uint64_t token;
};

/// <summary>
/// A callback that is invoked by the task queue.
/// </summary>
/// <param name='context'>A context pointer that was passed during XTaskQueueSubmitCallback.</param>
/// <param name='canceled'>If true, callbacks are being canceled because the queue is terminating.</param>
/// <seealso cref='XTaskQueueSubmitCallback' />
typedef void CALLBACK XTaskQueueCallback(_In_opt_ void* context, _In_ bool canceled);

/// <summary>
/// A callback that is invoked by the task queue whenever an item is submitted for execution.
/// </summary>
/// <param name='context'>A context pointer that was passed during XTaskQueueRegisterMonitor.</param>
/// <param name='queue'>The task queue where the callback was submitted.</param>
/// <param name='port'>The port the callback was submitted to.</param>
/// <seealso cref='XTaskQueueRegisterMonitor' />
typedef void CALLBACK XTaskQueueMonitorCallback(_In_opt_ void* context, _In_ XTaskQueueHandle queue, _In_ XTaskQueuePort port);

/// <summary>
/// A callback that is invoked when a task queue is terminated.
/// </summary>
/// <param name='context'>A context pointer that was passed during XTaskQueueTerminate.</param>
typedef void CALLBACK XTaskQueueTerminatedCallback(_In_opt_ void* context);

/// <summary>
/// Creates a Task Queue, which can be used to queue
/// and dispatch calls.  Task Queues are
/// reference counted objects.  Release the reference
/// by calling XTaskQueueCloseHandle.
/// </summary>
/// <param name='workDispatchMode'>The dispatch mode for the "work" port of the queue.</param>
/// <param name='completionDispatchMode'>The dispatch mode for the "completion" port of the queue.</param>
/// <param name='queue'>The newly created queue.</param>
STDAPI XTaskQueueCreate(
    _In_ XTaskQueueDispatchMode workDispatchMode,
    _In_ XTaskQueueDispatchMode completionDispatchMode,
    _Out_ XTaskQueueHandle* queue
    ) noexcept;

/// <summary>
/// Creates a task queue composed of ports of other
/// task queues. A composite task queue will duplicate
/// the handles of queues that own the provided ports.
/// </summary>
/// <param name='workPort'>The port to use for queuing work callbacks.</param>
/// <param name='completionPort'>The port to use for queuing completion callbacks.</param>
STDAPI XTaskQueueCreateComposite(
    _In_ XTaskQueuePortHandle workPort,
    _In_ XTaskQueuePortHandle completionPort,
    _Out_ XTaskQueueHandle* queue
    ) noexcept;

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
    ) noexcept;
    
/// <summary>
/// Duplicates the XTaskQueueHandle object.  Use XTaskQueueCloseHandle to close it.
/// </summary>
/// <param name='queueHandle'>The queue to reference.</param>
/// <param name='duplicatedHandle'>The duplicated queue handle.</param>
STDAPI XTaskQueueDuplicateHandle(
    _In_ XTaskQueueHandle queueHandle,
    _Out_ XTaskQueueHandle* duplicatedHandle
    ) noexcept;

/// <summary>
/// Processes an item in the task queue for the given port. If an item
/// is processed this will return true. If there are no items to process
/// this returns false.  You can pass a timeout, which will cause
/// XTaskQueueDispatch to wait for something to arrive in the queue.
/// </summary>
/// <param name='queue'>The queue to dispatch work on.</param>
/// <param name='port'>The port to dispatch.</param>
/// <param name='timeoutInMs'>The number of ms to wait for work to arrive before returning false.</param>
STDAPI_(bool) XTaskQueueDispatch(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port,
    _In_ uint32_t timeoutInMs
    ) noexcept;

/// <summary>
/// Closes the task queue.  The queue is not actually destroyed until
/// all handles are closed and there are no outstanding callbacks 
/// enqueued.
/// </summary>
/// <param name='queue'>The queue to close.</param>
STDAPI_(void) XTaskQueueCloseHandle(
    _In_ XTaskQueueHandle queue
    ) noexcept;

/// <summary>
/// Terminates a task queue by canceling all pending items and
/// preventing new items from being queued.  Once a queue is terminated
/// its handle can be closed. New items cannot be enqueued to a task
/// queue that has been terminated.
/// </summary>
/// <param name='queue'>The queue to terminate.</param>
/// <param name='wait'>True to wait for the termination to complete.</param>
/// <param name='callbackContext'>An optional context pointer to pass to the callback.</param>
/// <param name='callback'>An optional callback that will be called when the queue has terminated.</param>
STDAPI XTaskQueueTerminate(
    _In_ XTaskQueueHandle queue, 
    _In_ bool wait, 
    _In_opt_ void* callbackContext, 
    _In_opt_ XTaskQueueTerminatedCallback* callback
    ) noexcept;

/// <summary>
/// Submits a callback to the queue for the given port.
/// </summary>
/// <param name='queue'>The queue to submit the callback to.</param>
/// <param name='port'>The port to submit the callback to.</param>
/// <param name='callbackContext'>An optional context pointer that will be passed to the callback.</param>
/// <param name='callback'>A pointer to the callback function.</param>
STDAPI XTaskQueueSubmitCallback(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback
    ) noexcept;

/// <summary>
/// Submits a callback to the queue for the given port.  The callback will be added
/// to the queue after delayMs milliseconds.
/// </summary>
/// <param name='queue'>The queue to submit the callback to.</param>
/// <param name='port'>The port to submit the callback to.</param>
/// <param name='delayMs'>The number of milliseconds to delay before submitting the callback to the queue.</param>
/// <param name='callbackContext'>An optional context pointer that will be passed to the callback.</param>
/// <param name='callback'>A pointer to the callback function.</param>
STDAPI XTaskQueueSubmitDelayedCallback(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port,
    _In_ uint32_t delayMs,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback
    ) noexcept;

/// <summary>
/// Registers a wait handle with the task queue.  When the wait handle
/// is satisfied the task queue will invoke the given callback. This
/// provides an efficient way to add items to a task queue in 
/// response to handles becoming signaled.
/// </summary>
/// <param name='queue'>The queue to submit the callback to.</param>
/// <param name='port'>The port to invoke the callback on.</param>
/// <param name='waitHandle'>The handle to monitor.</param>
/// <param name='callbackContext'>An optional context pointer that will be passed to the callback.</param>
/// <param name='callback'>A pointer to the callback function.</param>
/// <param name='token'>A registration token.</param>
STDAPI XTaskQueueRegisterWaiter(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port,
    _In_ HANDLE waitHandle,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token
    ) noexcept;

/// <summary>
/// Unregisters a previously registered task queue waiter.
/// </summary>
/// <param name='queue'>The queue to remove the waiter from.</param>
/// <param name='token'>The token returned from XTaskQueueRegisterWaiter.</param>
STDAPI_(void) XTaskQueueUnregisterWaiter(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueueRegistrationToken token
    ) noexcept;

/// <summary>
/// Registers a callback that will be invoked whenever a callback
/// is submitted to this queue.
/// </summary>
/// <param name='queue'>The queue to register the submit callback to.</param>
/// <param name='callbackContext'>An optional context pointer to be passed to the callback.</param>
/// <param name='callback'>A callback that will be invoked when a new callback is submitted to the queue.</param>
/// <param name='token'>A token used in a later call to XTaskQueueUnregisterMonitor to remove the callback.</param>
STDAPI XTaskQueueRegisterMonitor(
    _In_ XTaskQueueHandle queue,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueMonitorCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token
    ) noexcept;

/// <summary>
/// Unregisters a previously registered callback. This blocks if there are outstanding montior
/// callbacks.
/// </summary>
/// <param name='queue'>The queue to remove the submit callback from.</param>
/// <param name='token'>The token returned from XTaskQueueRegisterMonitor.</param>
STDAPI_(void) XTaskQueueUnregisterMonitor(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueueRegistrationToken token
    ) noexcept;

/// <summary>
/// Obtains a handle to the process task queue, or nullptr if there is no
/// process task queue.  By default, there is a process task queue
/// that uses the thread pool for both work and completion ports. You
/// can replace the default process task queue by calling 
/// XTaskQueueSetCurrentProcessTaskQueue, and you can prevent callers using
/// the process task queue by calling XTaskQueueSetCurrentProcessTaskQueue 
/// with a null queue parameter.
///
/// This API returns true if there is a process task queue available.
/// You are responsible for calling XTaskQueueCloseHandle on the handle
/// returned from this API.
/// </summary>
STDAPI_(bool) XTaskQueueGetCurrentProcessTaskQueue(_Out_ XTaskQueueHandle* queue) noexcept;

/// <summary>
/// Sets the given task queue as the process wide task queue.  The
/// queue can be set to nullptr, in which case XTaskQueueGetCurrentProcessTaskQueue will
/// also return nullptr. The provided queue will have its handle duplicated
/// and any existing process task queue will have its handle closed.
/// </summary>
/// <param name='queue'>The queue to set up as the default task queue for the procces.</param>
STDAPI_(void) XTaskQueueSetCurrentProcessTaskQueue(
    _In_ XTaskQueueHandle queue
    ) noexcept;

} // extern "C"
