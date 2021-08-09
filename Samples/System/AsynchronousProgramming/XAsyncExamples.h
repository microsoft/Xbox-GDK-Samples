//--------------------------------------------------------------------------------------
// XAsyncExamples.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <XAsync.h>
#include <XAsyncProvider.h>

#include "RDTSCPStopWatch.h"
#include "StopwatchProfiler.h"

class Sample;

namespace ATG
{

// This example class shows how to use GDK's XAsync and XTaskQueue functionality for asynchronous programming
class XAsyncExamples
{
public:

    XAsyncExamples(Sample* sample);
    ~XAsyncExamples();

    void Update(float dt);

    void Notify_TaskQueueMonitor(WPARAM wp, LPARAM lp);

protected:

    void CreateTaskQueues();
    void CreateThreads();

    void ShutdownTaskQueues();
    void ShutdownThreads();

    void ThreadProc_SingleCore(unsigned int index);
    void ThreadProc_ManualAndMessageLoop();

    void Monitor_ManualAndMessageLoop(XTaskQueueHandle queue, XTaskQueuePort port);

    static double GetTime();

public:

    // Test cases:

    // System Thread Pool Parallel Work + Manual Completion (Async+Manual)
    // - Show off with custom async methods in GDK style
    void StartTest_CustomGDKStyleAPIs();

    // Default Asynchronous Task Queue Work + Replacing default task queue to a synchronous execution with same calling pattern (Async+Async vs Sync+Sync)
    //  - Show off with synchronous calls and replacing the default process task queue
    void StartTest_SynchronousTaskQueue();

    // Manual Work, Manual Completion
    // - Show off with extra threads that share affinity off the first two threads. The manual completion is handled in the Windows Message Loop via a Queue Monitor.
    void StartTest_ManualAndMessageLoop();

    // Serialized Thread Pool Work (SerializedAsync+SerializedAsync)
    // Show off with async tasks that have dependencies on each other
    void StartTest_SerializedAsync();

    // Special: Parallel Work, Immediate completion (Manual+Immediate)
    // - Show off with ParallelFor and prefix sum
    void StartTest_ParallelFor();

    // Extra: Composite Queue, Duplicate Queue Handles, Waiters & delayed dispatch
    void StartTest_AdvancedUsage();

    // Canceling async requests.
    void StartTest_Canceling();

    // Calculate overhead of async functions and usage for different task queues and scenarios
    void StartTest_OverheadCalculations();

protected:

    enum Overheads
    {
        // Time spent between invocation and start of work callback
        Overhead_XAsyncRun_InvokeToWork,
        // Time spent between end of work callback and start of completion callback
        Overhead_XAsyncRun_WorkToCompletion,

        // Time spent between invocation and the start of the body of one of the threads
        Overhead_ParallelFor_InvokeToBody,
        // Time spent between invocation and the end of the function with an empty body
        Overhead_ParallelFor_InvokeToReturn,

        // Time spent in async provider callback per call on average, regardless of what the call enum is
        Overhead_GDKAsyncStyle_TimeInProviderAverage,
        // Time spent in async provider overall for a single async invocation
        Overhead_GDKAsyncStyle_TimeInProviderOverall,
        // Time spent between invocation and getting to the work callback
        Overhead_GDKAsyncStyle_InvokeToWork,
        // Time spent between the end of the work callback and the beginning of the completion callback
        Overhead_GDKAsyncStyle_WorkToCompletion,

        Overhead_Total
    };

    // Helper functions for overhead calculations to reduce code duplication
    void CalculateOverhead_XAsyncRun(XTaskQueueHandle taskQueue, StopwatchProfiler<Overhead_Total>& overheadProfiler);
    void CalculateOverhead_ParallelFor(StopwatchProfiler<Overhead_Total>& overheadProfiler);
    void CalculateOverhead_GDKAsyncStyle(XTaskQueueHandle taskQueue, StopwatchProfiler<Overhead_Total>& overheadProfiler);
    static void GetFormattedTimingInfo(double seconds, const char** outLabel, double* outValue);
    static void LogTiming(StopwatchProfiler<Overhead_Total>::TimingAccumulator* accumulator, const char* prefixLabel);

public:

    // Some basic slow single-thread algorithms
    static bool IsPrime(uint64_t num);
    static uint64_t NthPrime(uint64_t n);

    // Async versions in GDK API style of the above single-threaded algorithms
    static HRESULT NthPrimeAsync(uint64_t n, XAsyncBlock* async);
    static HRESULT NthPrimeAsyncResult(XAsyncBlock* async, uint64_t* result);

    // Cancelable async method. Method won't end unless it's canceled.
    static HRESULT CancelableInfiniteTaskAsync(XAsyncBlock* async);

    // ParallelFor function which runs from startIndex to endIndex and invokes the bodyFunction
    // in parallel for each index. The end index is one-past the end, so a synchronous loop would
    // look like this: for(unsigned int i = startIndex; i < endIndex; ++i) { bodyFunction(i); }
    static void ParallelFor(unsigned int startIndex, unsigned int endIndex, std::function<void(unsigned int)> bodyFunction);

    // Similar to ParallelFor, this function more generic in that it just runs N number of tasks in parallel and
    // requires you specify the task queue to use. Note that parallelization is actually based on the task queue specified.
    static void ParallelExecute(unsigned int numTasks, XTaskQueueHandle taskQueue, std::function<void(unsigned int)> bodyFunction);

    // Runs a task on the specified task queue with the specified callbacks. If nullptr is passed for the tasks queue, then the default
    // process test queue will be used.
    static void RunTask(XTaskQueueHandle taskQueue, std::function<void()> workCallback, std::function<void()> completionCallback);
    static void RunTask(XTaskQueueHandle taskQueue, std::function<void()> workCallback);

protected:

    bool                        m_testInProgress;
    XTaskQueueHandle            m_taskQueue_CustomGDKStyleAPIs;
    XTaskQueueHandle            m_taskQueue_ParallelFor;
    XTaskQueueHandle            m_taskQueue_SynchronousTaskQueue;
    XTaskQueueHandle            m_taskQueue_SerializedAsync;
    XTaskQueueHandle            m_taskQueue_ManualAndMessageLoop;

    XTaskQueueRegistrationToken m_taskQueueToken_ManualAndMessageloop;

    XTaskQueueHandle            m_cachedProcessDefaultTaskQueueHandle;

    unsigned int                m_numCores;
    std::vector<std::thread>    m_threads_SingleCore;
    std::vector<std::thread>    m_threads_ManualAndMessageLoop;
    HANDLE                      m_shutdownEvent;

    static XAsyncExamples*      s_singleton;
    static Sample*              s_sample;

    static RDTSCPStopWatch      s_stopWatch;
};

}
