//--------------------------------------------------------------------------------------
// TaskQueue.h
//
// Simple Per-Core Asynchronous Task Queue
//
// This code creates two affinitized threads per core (one for work, one for completions), and runs an Async Task Queue
// on it. This allows you to target callbacks and other work to occur on specific cores, and can be used as a starting
// point if you have more complex needs.
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#pragma once

namespace ATG
{
   // The maximum number of cores that will be used. Cores will be numbered from 0 to MAX_THREAD_CORES - 1.
   constexpr size_t MAX_THREAD_CORES = 7;

   // Creates an Async Task Queue on all of the CPU cores, processing its callbacks and work on the same core.
   void InitializeThreadQueues();

   // Gets the Async Task Queue for a specific core. Does not duplicate the handle. If you need a duplicate, create
   // one yourself using the returned value. Do not close the returned value yourself.
   XTaskQueueHandle GetThreadQueueForCore( uint16_t coreIndex );

   // Terminates all of the task queues created by InitializeThreadQueues, and waits for the threads to cleanly
   // shut down.
   void ShutdownThreadQueues();
}
