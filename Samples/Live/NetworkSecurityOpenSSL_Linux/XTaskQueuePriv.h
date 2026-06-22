// Copyright(c) Microsoft Corporation. All rights reserved.
//
// These APIs should be reserved for driving unit test harnesses.

#pragma once

#include "XTaskQueue.h"

/// <summary>
/// Returns TRUE if there is no outstanding work in this
/// queue for the given callback port.
/// </summary>
/// <param name='queue'>The queue to check.</param>
/// <param name='port'>The port to check.</param>
STDAPI_(bool) XTaskQueueIsEmpty(
    _In_ XTaskQueueHandle queue,
    _In_ XTaskQueuePort port
    ) noexcept;

/// <summary>
/// Suspends terminations on the task queue.  May return an error if
/// the queue is already terminated.
/// </summary>
/// <param name='queue'>The queue to suspend terminations.</param>
STDAPI XTaskQueueSuspendTermination(
    _In_ XTaskQueueHandle queue
    ) noexcept;

/// <summary>
/// Resumes the ability to terminate the task queue. If a termination was
/// attempted it will be continued.
/// </summary>
/// <param name='queue'>The queue resume terminations.</param>
STDAPI_(void) XTaskQueueResumeTermination(
    _In_ XTaskQueueHandle queue
    ) noexcept;
