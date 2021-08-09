//--------------------------------------------------------------------------------------
// XAsyncExamples.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "XAsyncExamples.h"
#include "AsynchronousProgramming.h"

using namespace ATG;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmicrosoft-cast"
#endif

XAsyncExamples* XAsyncExamples::s_singleton = nullptr;
Sample* XAsyncExamples::s_sample = nullptr;
RDTSCPStopWatch XAsyncExamples::s_stopWatch;

XAsyncExamples::XAsyncExamples(Sample* sample)
    : m_testInProgress(false)
{
    if (s_singleton)
    {
        throw new std::exception("Only create 1 instance of this class");
    }
    s_singleton = this;
    s_sample = sample;

    s_stopWatch.Start();

    CreateTaskQueues();
    CreateThreads();
}

XAsyncExamples::~XAsyncExamples()
{
    ShutdownThreads();
    ShutdownTaskQueues();
}

void XAsyncExamples::Update(float /*dt*/)
{
    // This is called from the main thread

    // Dispatch events from the completion port of the task queue used in the CustomGDKStyleAPIs test
    XTaskQueueDispatch(m_taskQueue_CustomGDKStyleAPIs, XTaskQueuePort::Completion, 0);
}

void XAsyncExamples::Notify_TaskQueueMonitor(WPARAM /*wp*/, LPARAM /*lp*/)
{
    // This function is invoked by the message pump of the main window on whatever thread is handling it.
    XTaskQueueDispatch(m_taskQueue_ManualAndMessageLoop, XTaskQueuePort::Completion, 0);
}

void XAsyncExamples::CreateThreads()
{
    char buffer[256] = {};

    // Create shutdown event for all threads
    m_shutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    m_numCores = std::thread::hardware_concurrency();
    sprintf_s(buffer, 256, u8"Detected core count at [%d]", m_numCores);
    s_sample->Log(buffer);

    sprintf_s(buffer, 256, u8"Spinning up %d threads with their affinities set to a single core", m_numCores);
    s_sample->Log(buffer);
    m_threads_SingleCore.reserve(m_numCores);
    for(unsigned int i = 0; i < m_numCores; ++i)
    {
        m_threads_SingleCore.push_back(std::thread(
            [&, i]()
            {
                ThreadProc_SingleCore(i);
            }));
    }

    unsigned int numManualThreadsToSpin = m_numCores >= 4 ? m_numCores - 2 : 2;
    sprintf_s(buffer, 256, u8"Spinning up %d threads with their affinities set to all cores except 0 and 1", numManualThreadsToSpin);
    s_sample->Log(buffer);
    m_threads_ManualAndMessageLoop.reserve(numManualThreadsToSpin);
    for (unsigned int i = 0; i < numManualThreadsToSpin; ++i)
    {
        m_threads_ManualAndMessageLoop.push_back(std::thread(
            [&]()
            {
                ThreadProc_ManualAndMessageLoop();
            }));
    }
}

void XAsyncExamples::ShutdownThreads()
{
    SetEvent(m_shutdownEvent);
    for (auto& thread : m_threads_SingleCore)
    {
        thread.join();
    }
    for (auto& thread : m_threads_ManualAndMessageLoop)
    {
        thread.join();
    }
}

void XAsyncExamples::ThreadProc_SingleCore(unsigned int index)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"[ThreadProc_SingleCore] Thread Index %d Spawned!", index);
    s_sample->Log(buffer);

    // Set core affinity to this thread's index only
    DWORD_PTR result = SetThreadAffinityMask(GetCurrentThread(), 1ull << index);
    if (result == 0)
    {
        sprintf_s(buffer, 256, u8"[ThreadProc_SingleCore][%d]: Failed to set affinity mask", index);
        s_sample->Log(buffer);
    }

    while (WaitForSingleObject(m_shutdownEvent, 0) == WAIT_TIMEOUT)
    {
        // XTaskQueueDispatch is thread-safe for manual ports. The different threads dispatching will pull
        // work on a first-come, first-serve basis. Use a timeout so that the threads will sleep a little when there
        // is no work and not steal all system performance.
        XTaskQueueDispatch(m_taskQueue_ParallelFor, XTaskQueuePort::Work, 200);
    }

    sprintf_s(buffer, 256, u8"[ThreadProc_SingleCore][%d]: Exiting", index);
    s_sample->Log(buffer);
}

void XAsyncExamples::ThreadProc_ManualAndMessageLoop()
{
    char buffer[256] = {};
    s_sample->Log(u8"[ThreadProc_ManualAndMessageLoop] Thread Index Spawned!");

    // Set core affinity all threads except thread 0 and 1
    DWORD_PTR affinityMask = ~0x3ull;
    DWORD_PTR result = SetThreadAffinityMask(GetCurrentThread(), affinityMask);
    if (result == 0)
    {
        sprintf_s(buffer, 256, u8"[ThreadProc_ManualAndMessageLoop]: Failed to set affinity mask");
        s_sample->Log(buffer);
    }

    while (WaitForSingleObject(m_shutdownEvent, 0) == WAIT_TIMEOUT)
    {
        // XTaskQueueDispatch is thread-safe for manual ports. The different threads dispatching will pull
        // work on a first-come, first-serve basis. Use a timeout so that the threads will sleep a little when there
        // is no work and not steal all system performance.
        XTaskQueueDispatch(m_taskQueue_ManualAndMessageLoop, XTaskQueuePort::Work, 200);
    }

    sprintf_s(buffer, 256, u8"[ThreadProc_ManualAndMessageLoop]: Exiting");
    s_sample->Log(buffer);
}

void XAsyncExamples::Monitor_ManualAndMessageLoop(XTaskQueueHandle /*queue*/, XTaskQueuePort port)
{
    char buffer[256] = {};

    // Only run the completion work via the message pump. The work work will be run by the spawned threads for this test.
    if (port == XTaskQueuePort::Work)
    {
        sprintf_s(buffer, 256, u8"Monitor Callback invoked for XTaskQueuePort::Work. Spawned threads will process manual port automatically.");
        s_sample->Log(buffer);
    }
    else
    {
        sprintf_s(buffer, 256, u8"Monitor Callback invoked for XTaskQueuePort::Completion. Posting a WM_TASKQUEUEMONITOR for handling in the Windows message loop.");
        s_sample->Log(buffer);

        PostMessage(s_sample->GetHWND(), WM_TASKQUEUEMONITOR, 0, 0);
    }
}

double XAsyncExamples::GetTime()
{
    return s_stopWatch.GetCurrentSeconds();
}

void XAsyncExamples::GetFormattedTimingInfo(double seconds, const char** outLabel, double* outValue)
{
    assert(outLabel && outValue);

    // Microseconds:
    if (seconds < 0.001)
    {
        (*outValue) = seconds * 1000.0 * 1000.0;
        (*outLabel) = u8"us";
    }
    // Milliseconds
    else if (seconds < 1.0)
    {
        (*outValue) = seconds * 1000.0;
        (*outLabel) = u8"ms";
    }
    // Seconds
    else
    {
        (*outValue) = seconds;
        (*outLabel) = u8"s";
    }
}

void XAsyncExamples::LogTiming(StopwatchProfiler<Overhead_Total>::TimingAccumulator* accumulator, const char* prefixLabel)
{
    char buffer[512] = {};

    double minTime = 0.0;
    double maxTime = 0.0;
    double avgTime = 0.0;
    const char* minLabel = nullptr;
    const char* maxLabel = nullptr;
    const char* avgLabel = nullptr;

    GetFormattedTimingInfo(accumulator->m_lowestVal, &minLabel, &minTime);
    GetFormattedTimingInfo(accumulator->m_highestVal, &maxLabel, &maxTime);
    GetFormattedTimingInfo(accumulator->GetAverage(), &avgLabel, &avgTime);

    sprintf_s(buffer, 512, u8"%s: avg:%.3f%s min:%.3f%s max:%.3f%s", prefixLabel, avgTime, avgLabel, minTime, minLabel, maxTime, maxLabel);
    s_sample->Log(buffer);
}

void XAsyncExamples::CreateTaskQueues()
{
    // Cache the current process task queue. A ref to the retrieved task queue is automatically added.
    XTaskQueueGetCurrentProcessTaskQueue(&m_cachedProcessDefaultTaskQueueHandle);
    assert(m_cachedProcessDefaultTaskQueueHandle);

    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::Manual,
        &m_taskQueue_CustomGDKStyleAPIs));

    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::Manual,
        XTaskQueueDispatchMode::Immediate,
        &m_taskQueue_ParallelFor));

    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::Immediate,
        XTaskQueueDispatchMode::Immediate,
        &m_taskQueue_SynchronousTaskQueue));

    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::SerializedThreadPool,
        XTaskQueueDispatchMode::SerializedThreadPool,
        &m_taskQueue_SerializedAsync));

    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::Manual,
        XTaskQueueDispatchMode::Manual,
        &m_taskQueue_ManualAndMessageLoop));

    DX::ThrowIfFailed(XTaskQueueRegisterMonitor(
        m_taskQueue_ManualAndMessageLoop,
        this,
        [](void* context, XTaskQueueHandle queue, XTaskQueuePort port)
        {
            XAsyncExamples* examples = static_cast<XAsyncExamples*>(context);
            examples->Monitor_ManualAndMessageLoop(queue, port);
        },
        &m_taskQueueToken_ManualAndMessageloop));
}

void XAsyncExamples::ShutdownTaskQueues()
{
    // XTaskQueueTerminate adds to the terminated queue a task marker to know when the queue is empty.
    // As a result, you can't directly call XTaskQueueTerminate with wait=true on any task queue with a
    // manual port. Instead, you need to optionally use the terminate callbacks and keep pumping the manual
    // of the queue until the terminate finishes. When a queue is terminated, an infinite-dispatching call
    // will return, so we can use the behavior.

    XTaskQueueTerminate(m_taskQueue_CustomGDKStyleAPIs, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue_CustomGDKStyleAPIs, XTaskQueuePort::Completion, INFINITE);
    XTaskQueueCloseHandle(m_taskQueue_CustomGDKStyleAPIs);

    XTaskQueueTerminate(m_taskQueue_ParallelFor, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue_ParallelFor, XTaskQueuePort::Work, INFINITE);
    XTaskQueueCloseHandle(m_taskQueue_ParallelFor);

    XTaskQueueTerminate(m_taskQueue_SynchronousTaskQueue, true, nullptr, nullptr);
    XTaskQueueCloseHandle(m_taskQueue_SynchronousTaskQueue);

    XTaskQueueTerminate(m_taskQueue_SerializedAsync, true, nullptr, nullptr);
    XTaskQueueCloseHandle(m_taskQueue_SerializedAsync);

    XTaskQueueUnregisterMonitor(m_taskQueue_ManualAndMessageLoop, m_taskQueueToken_ManualAndMessageloop);
    XTaskQueueTerminate(m_taskQueue_ManualAndMessageLoop, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue_ManualAndMessageLoop, XTaskQueuePort::Work, INFINITE);
    XTaskQueueDispatch(m_taskQueue_ManualAndMessageLoop, XTaskQueuePort::Completion, INFINITE);
    XTaskQueueCloseHandle(m_taskQueue_ManualAndMessageLoop);

    // Release the handle we acquired to the default process task queue
    XTaskQueueTerminate(m_cachedProcessDefaultTaskQueueHandle, true, nullptr, nullptr);
    XTaskQueueCloseHandle(m_cachedProcessDefaultTaskQueueHandle);
    m_cachedProcessDefaultTaskQueueHandle = nullptr;
}

// This test demonstrates how to create and use GDK-API-style async invocation methods. See NthPrimeAsync and NthPrimeAsyncResult for
// the async plumbing implementation. The test function calls into NthPrimeAsync several times in the same manner as GDK Async functions.
// The work of calculating the primes is done via the system thread pool which takes tasks as they can. The completion callbacks will be handled
// by the main thread via the Update call of this class. Callbacks are handled in the order that they are queued to the task queue.
void XAsyncExamples::StartTest_CustomGDKStyleAPIs()
{
    if (m_testInProgress)
    {
        return;
    }
    m_testInProgress = true;

    s_sample->Log(u8"==========================================================================");
    s_sample->Log(u8"Beginning CustomGDKStyleAPIs test");

    // Begin several async invocations of NthPrime. Note that the XAsyncBlock cannot be shared between the different calls as it has internal
    // bookkeeping information. Using a lambda to reduce code duplication
    auto StartAsyncCall = [&](uint64_t n, volatile LONG* testCounter, LONG numTests)
    {
        struct CallData
        {
            uint64_t n;
            volatile LONG* testCounter;
            LONG numTests;
            XAsyncExamples* examples;
        };
        CallData* callData = new CallData{n, testCounter, numTests, this};

        XAsyncBlock* async = new XAsyncBlock{};
        ZeroMemory(async, sizeof(XAsyncBlock));
        async->queue = m_taskQueue_CustomGDKStyleAPIs;
        async->context = callData;
        async->callback = [](XAsyncBlock* async)
        {
            CallData* callData = static_cast<CallData*>(async->context);

            uint64_t nthPrime = 0;
            HRESULT hr = NthPrimeAsyncResult(async, &nthPrime);
            if (SUCCEEDED(hr))
            {
                char buffer[256] = {};
                sprintf_s(buffer, 256, u8"Async call to calculate the nth prime %llu finished with result %llu", callData->n, nthPrime);
                s_sample->Log(buffer);
            }
            else
            {
                char buffer[256] = {};
                sprintf_s(buffer, 256, u8"Async call to calculate the nth prime %llu failed with hr=%08X", callData->n, hr);
                s_sample->Log(buffer);
            }

            // Detect when test is done
            LONG counterValue = InterlockedIncrement(callData->testCounter);
            if (counterValue == callData->numTests)
            {
                s_sample->Log(u8"CustomGDKStyleAPIs test finished");
                s_sample->Log(u8"==========================================================================");

                callData->examples->m_testInProgress = false;
                delete callData->testCounter;
            }

            delete callData;
            delete async;
        };

        char buffer[256] = {};
        sprintf_s(buffer, 256, u8"Firing async call to calculate the nth prime %llu", n);
        s_sample->Log(buffer);
        DX::ThrowIfFailed(NthPrimeAsync(n, async));
    };

    volatile LONG* testCounter = new volatile LONG(0);
    const LONG numTests = 5;
    StartAsyncCall(12000, testCounter, numTests);
    StartAsyncCall(9000, testCounter, numTests);
    StartAsyncCall(7500, testCounter, numTests);
    StartAsyncCall(5000, testCounter, numTests);
    StartAsyncCall(3000, testCounter, numTests);
}

// This test shows how you can use different task queues to run asynchronous code synchronously. The
// whole test is run on a task as well to prevent the main process thread from locking up during the
// test. The process default task queue the the task queue used when nullptr is passed to the queue
// member in an XAsyncBlock. That task queue can be acquired, changed, and cleared as desired for
// your application.
void XAsyncExamples::StartTest_SynchronousTaskQueue()
{
    if (m_testInProgress)
    {
        return;
    }
    m_testInProgress = true;

    s_sample->Log(u8"==========================================================================");
    s_sample->Log(u8"Beginning SynchronousTaskQueue test");

    // If the process task queue is overridden to nullptr, any attempts to use it with a nullptr in XAsyncBlock
    // will fail.
    //XTaskQueueSetCurrentProcessTaskQueue(nullptr);

    // Override the process default task queue with the synchronous task queue
    XTaskQueueSetCurrentProcessTaskQueue(m_taskQueue_SynchronousTaskQueue);

    RunTask(m_cachedProcessDefaultTaskQueueHandle,
        // Work callback
        [&]()
        {
            static constexpr unsigned int taskCount = 20;
            static constexpr uint64_t nthPrimeToCalculate = 3000;

            // First perform some tasks in parallel and time it
            {
                double startTime = GetTime();
                ParallelExecute(taskCount, m_cachedProcessDefaultTaskQueueHandle,
                    [=](unsigned int)
                    {
                        NthPrime(nthPrimeToCalculate);
                    });
                double endTime = GetTime();

                char buffer[256] = {};
                sprintf_s(buffer, 256, u8"Asynchronously calculating NthPrime(%llu) %d times took %.3f seconds.", nthPrimeToCalculate, taskCount, endTime - startTime);
                s_sample->Log(buffer);
            }

            // Next perform those tasks, but do them synchronously on this thread and time it
            {
                // The process default task queue was overridden to a queue with both ports set to Immediate,
                // so these calculations will run synchronously.
                double startTime = GetTime();
                ParallelExecute(taskCount, nullptr,
                    [=](unsigned int)
                    {
                        NthPrime(nthPrimeToCalculate);
                    });
                double endTime = GetTime();

                char buffer[256] = {};
                sprintf_s(buffer, 256, u8"Synchronously calculating NthPrime(%llu) %d times took %.3f seconds.", nthPrimeToCalculate, taskCount, endTime - startTime);
                s_sample->Log(buffer);
            }
        },
        // Completion callback
        [&]()
        {
            // Restore the process default task queue to the original ThreadPool one.
            XTaskQueueSetCurrentProcessTaskQueue(m_cachedProcessDefaultTaskQueueHandle);

            s_sample->Log(u8"SynchronousTaskQueue test finished");
            s_sample->Log(u8"==========================================================================");
            m_testInProgress = false;
        });
}

// This test executes some tasks that are handled via a fully manual task queue. The work is performed on some threads that have their
// affinity set off of the first two cores. The completion callbacks are handled via the Windows Message Loop. To get the handling done there,
// a monitor was registered for this test's task queue that invokes an event to be propagated to the message loop.
void XAsyncExamples::StartTest_ManualAndMessageLoop()
{
    if (m_testInProgress)
    {
        return;
    }
    m_testInProgress = true;

    s_sample->Log(u8"==========================================================================");
    s_sample->Log(u8"Beginning ManualAndMessageLoop test");

    volatile LONG* testCounter = new volatile LONG(0);

    // Run a bunch of tasks using the manual queue. Log will report the monitor invocations.
    static constexpr LONG numTasks = 20;
    for (LONG i = 0; i < numTasks; ++i)
    {
        char buffer[256] = {};
        sprintf_s(buffer, 256, u8"Queuing Task Index [%d]", i);
        s_sample->Log(buffer);

        RunTask(m_taskQueue_ManualAndMessageLoop,
            // Work callback
            [i]()
            {
                char buffer[256] = {};
                sprintf_s(buffer, 256, u8"Work Callback Index [%d] Invoked. Sleeping to simulate work.", i);
                s_sample->Log(buffer);

                // Simulate some time-taking work
                Sleep(2000);

                sprintf_s(buffer, 256, u8"Work Callback Index [%d] sleep finished.", i);
                s_sample->Log(buffer);
            },
            // Completions callback
            [&, i, testCounter]()
            {
                char buffer[256] = {};
                sprintf_s(buffer, 256, u8"Completion Callback Index [%d] Invoked", i);
                s_sample->Log(buffer);

                // Detect when test is done
                LONG counterValue = InterlockedIncrement(testCounter);
                if (counterValue == numTasks)
                {
                    delete testCounter;
                    s_sample->Log(u8"ManualAndMessageLoop test finished");
                    s_sample->Log(u8"==========================================================================");
                    m_testInProgress = false;
                }
            });
    }
}

// This test shows how to use the SerializedThreadPool setting for the ports of the task queue. A serialized thread pool port
// processes work asynchronously on the system thread pools in the same way as a ThreadPool port, but only processes 1 element
// from the queue at a time. This allows the work to have dependencies between steps, but removes parallelism.
void XAsyncExamples::StartTest_SerializedAsync()
{
    if (m_testInProgress)
    {
        return;
    }
    m_testInProgress = true;

    s_sample->Log(u8"==========================================================================");
    s_sample->Log(u8"Beginning SerializedAsync test");

    // The following tasks all have dependencies upon each other and are ensured to run in-order with the serialized thread pool task queue.
    // However it's important to note that this only applies to callbacks within the same port.  Each port of the task queue could be set differently,
    // so the completion callback from one task will still run in parallel with a work callback from another task.
    // A use-case might be web requests, database queries, or other type of work which runs in the background and depends on the results at each step.

    std::vector<uint64_t>* workSet = new std::vector<uint64_t>();

    // First initialize the work set with some randomized numbers (use srand to be deterministic)
    RunTask(m_taskQueue_SerializedAsync,
        [workSet]()
        {
            char buffer[512] = {};
            size_t bytesWritten = static_cast<size_t>(sprintf_s(buffer, 512, u8"Initialized work set to { "));

            srand(0);
            for (unsigned int i = 0; i < 20; ++i)
            {
                uint64_t randVal = static_cast<uint64_t>(rand() % 3000);
                workSet->push_back(randVal);

                bytesWritten += sprintf_s(buffer + bytesWritten, 512 - bytesWritten, u8"%llu ", randVal);
            }

            bytesWritten += sprintf_s(buffer + bytesWritten, 512 - bytesWritten, u8"}");
            s_sample->Log(buffer);
        });

    // Next sort the numbers
    RunTask(m_taskQueue_SerializedAsync,
        [workSet]()
        {
            std::sort(workSet->begin(), workSet->end());

            char buffer[512] = {};
            size_t bytesWritten = static_cast<size_t>(sprintf_s(buffer, 512, u8"Sorted work set to { "));
            for (unsigned int i = 0; i < workSet->size(); ++i)
            {
                bytesWritten += sprintf_s(buffer + bytesWritten, 512 - bytesWritten, u8"%llu ", (*workSet)[i]);
            }
            bytesWritten += sprintf_s(buffer + bytesWritten, 512 - bytesWritten, u8"}");
            s_sample->Log(buffer);
        });

    // Use the numbers in the work set to calculate the nth primes
    RunTask(m_taskQueue_SerializedAsync,
        [workSet]()
        {
            char buffer[512] = {};
            size_t bytesWritten = static_cast<size_t>(sprintf_s(buffer, 512, u8"NthPrimes for work set calculated to { "));

            for (unsigned int i = 0; i < workSet->size(); ++i)
            {
                (*workSet)[i] = NthPrime((*workSet)[i]);

                bytesWritten += sprintf_s(buffer + bytesWritten, 512 - bytesWritten, u8"%llu ", (*workSet)[i]);
            }

            bytesWritten += sprintf_s(buffer + bytesWritten, 512 - bytesWritten, u8"}");
            s_sample->Log(buffer);
        });

    // Update all the numbers to be the prefix sum of itself
    RunTask(m_taskQueue_SerializedAsync,
        [workSet]()
        {
            char buffer[512] = {};
            size_t bytesWritten = static_cast<size_t>(sprintf_s(buffer, 512, u8"Prefix sum for work set calculated to { "));

            bytesWritten += sprintf_s(buffer + bytesWritten, 512 - bytesWritten, u8"%llu ", (*workSet)[0]);
            for (unsigned int i = 1; i < workSet->size(); ++i)
            {
                (*workSet)[i] += (*workSet)[i - 1];

                bytesWritten += sprintf_s(buffer + bytesWritten, 512 - bytesWritten, u8"%llu ", (*workSet)[i]);
            }

            bytesWritten += sprintf_s(buffer + bytesWritten, 512 - bytesWritten, u8"}");
            s_sample->Log(buffer);
        });

    // Finally just end the test
    RunTask(m_taskQueue_SerializedAsync,
        [&, workSet]()
        {
            delete workSet;

            s_sample->Log(u8"SerializedAsync test finished");
            s_sample->Log(u8"==========================================================================");
            m_testInProgress = false;
        });
}

// This test shows how to use the XAsync libraries to setup a ParallelFor style function to take advantage of multi-process
// calculations. A prefix sum calculation is performed in both single-thread and multi-thread manner to show the difference
// of timings. The parallelization is handled via a manual work port in the task queue. A thread was spawned with affinity set
// to a single core and these threads dispatch the work port themselves. Finally, the test itself is run on another thread
// using the default process task queue so-as to prevent the application from freezing while the tests are running.
void XAsyncExamples::StartTest_ParallelFor()
{
    if (m_testInProgress)
    {
        return;
    }
    m_testInProgress = true;

    s_sample->Log(u8"==========================================================================");
    s_sample->Log(u8"Beginning ParallelFor test");

    // nullptr specifies to use the process default queue which is [ThreadPool,ThreadPool] when not overridden
    RunTask(nullptr,
        // Work callback
        [&]()
        {
            // Calculate prefix sum
            char buffer[256] = {};

            // Setup elements to calculate over. Parallel case uses double-buffered shared memory.
            // Don't time the setup.
            const unsigned int numElements = 100000000;
            uint64_t* elements = new uint64_t[numElements];
            for (unsigned int i = 0; i < numElements; ++i)
            {
                elements[i] = i + 1;
            }

            // Sync calculation
            {
                sprintf_s(buffer, 256, u8"Starting synchronous prefix sum calculation for %d elements", numElements);
                s_sample->Log(buffer);

                // Allocate outside of timing
                uint64_t* finalElements = new uint64_t[numElements];
                memset(finalElements, 0, sizeof(uint64_t) * numElements);

                double startTime = GetTime();

                finalElements[0] = elements[0];
                for (unsigned int i = 1; i < numElements; ++i)
                {
                    finalElements[i] = finalElements[i - 1] + elements[i];
                }

                double endTime = GetTime();

                sprintf_s(buffer, 256, u8"Synchronous prefix sum calculation for %d elements took %.03f milliseconds", numElements, (endTime - startTime) * 1000.0);
                s_sample->Log(buffer);

                delete[] finalElements;
            }

            // Async calculation
            {
                sprintf_s(buffer, 256, u8"Starting asynchronous (ParallelFor) prefix sum calculation for %d elements", numElements);
                s_sample->Log(buffer);

                // Allocate outside of timing
                uint64_t* finalElements = new uint64_t[numElements];
                memset(finalElements, 0, sizeof(uint64_t) * numElements);
                uint64_t* offsetElements = new uint64_t[s_singleton->m_numCores];
                memset(offsetElements, 0, sizeof(uint64_t) * s_singleton->m_numCores);

                double startTime = GetTime();

                const unsigned int numElementsPerThread = numElements / (s_singleton->m_numCores + 1);
                const unsigned int extraElementsForLastThread = numElements % (s_singleton->m_numCores + 1);

                // First sweep to calculate offsets
                // First sweep only does the first m_numCores sets of data out of (m_numCores + 1)
                ParallelFor(0, s_singleton->m_numCores,
                    [&](unsigned int index)
                    {
                        const unsigned int offset = (index * numElementsPerThread);
                        finalElements[offset + 0] = elements[offset + 0];
                        for (unsigned int i = 1; i < numElementsPerThread; ++i)
                        {
                            finalElements[offset + i] = finalElements[offset + i - 1] + elements[offset + i];
                        }
                    });

                // Fast calculation of prefix sum of offsets
                offsetElements[0] = finalElements[numElementsPerThread - 1];
                for (unsigned int i = 1; i < s_singleton->m_numCores; ++i)
                {
                    const unsigned int curFinalElementsOffset = (numElementsPerThread - 1) + (numElementsPerThread * i);
                    offsetElements[i] = finalElements[curFinalElementsOffset] + offsetElements[i - 1];
                }

                // Second sweep to finalize prefix sums
                // Second sweep does the last m_numCores sets of data out of (m_numCores + 1), also taking into account the larger last set
                ParallelFor(0, s_singleton->m_numCores,
                    [&](unsigned int index)
                    {
                        const unsigned int numElementsThisThread = (index == s_singleton->m_numCores - 1) ? numElementsPerThread + extraElementsForLastThread : numElementsPerThread;

                        const unsigned int offset = ((index + 1) * numElementsPerThread);
                        finalElements[offset + 0] = elements[offset + 0] + offsetElements[index];
                        for (unsigned int i = 1; i < numElementsThisThread; ++i)
                        {
                            finalElements[offset + i] = finalElements[offset + i - 1] + elements[offset + i];
                        }
                    });

                double endTime = GetTime();

                sprintf_s(buffer, 256, u8"Asynchronous (ParallelFor) prefix sum calculation for %d elements took %.03f milliseconds", numElements, (endTime - startTime) * 1000.0);
                s_sample->Log(buffer);

                delete[] offsetElements;
                delete[] finalElements;
            }

            delete[] elements;
        },
        // Completion callback
        [&]()
        {
            s_sample->Log(u8"ParallelFor test finished");
            s_sample->Log(u8"==========================================================================");
            m_testInProgress = false;
        });
}

// This test shows how to use some more advanced features of XAsync/XTaskQueue. Instead of creating resources at class creation,
// all resources needed for this test will be created inline for demonstration. Demonstrated features include composite queues,
// duplicating queue handles, and using waiters and delayed dispatching. See documentation for more information about the behavior
// of these features. Note that all these tests will run in parallel as well.
void XAsyncExamples::StartTest_AdvancedUsage()
{
    if (m_testInProgress)
    {
        return;
    }
    m_testInProgress = true;

    s_sample->Log(u8"==========================================================================");
    s_sample->Log(u8"Beginning AdvancedUsage test");

    // Test data
    struct TestData
    {
        XTaskQueueHandle                baseQueue;
        XTaskQueueHandle                compositeQueue;
        XTaskQueuePortHandle            baseQueueWorkPort;

        std::thread                     baseQueueDispatcherThread;

        XTaskQueueHandle                duplicateBaseQueueHandle;

        HANDLE                          waiterEvent;
        XTaskQueueRegistrationToken     waiterToken;

        std::function<void(TestData*)>  autoCleanupFn;
        volatile LONG*                  testCounter;
    };
    TestData* testData = new TestData();
    testData->testCounter = new volatile LONG(0);

    // Make a function that will auto-cleanup on the last call as long as it's called "totalTests" amount of times.
    static constexpr LONG totalTests = 4;
    testData->autoCleanupFn = [&](TestData* testData)
    {
        LONG counterValue = InterlockedIncrement(testData->testCounter);
        if (counterValue == totalTests)
        {
            s_sample->Log(u8"AdvancedUsage test finished. Cleaning up.");

            XTaskQueueUnregisterWaiter(testData->baseQueue, testData->waiterToken);
            CloseHandle(testData->waiterEvent);

            XTaskQueueTerminate(testData->compositeQueue, true, nullptr, nullptr);
            XTaskQueueTerminate(testData->baseQueue, true, nullptr, nullptr);

            XTaskQueueCloseHandle(testData->compositeQueue);
            XTaskQueueCloseHandle(testData->duplicateBaseQueueHandle);
            XTaskQueueCloseHandle(testData->baseQueue);

            testData->baseQueueDispatcherThread.join();

            m_testInProgress = false;

            delete testData->testCounter;
            delete testData;

            s_sample->Log(u8"==========================================================================");
        }
    };

    // Composite Queue
    // Composite queues are queues composed of ports of other queues instead of owning their own ports. In this test, the composite
    // queue uses the work port from the base queue as its own work and completion port. Therefore, dispatching work to the composite
    // queue will all fall on the work port thread from the base queue which is set to ThreadPool.
    {
        DX::ThrowIfFailed(XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &testData->baseQueue));
        DX::ThrowIfFailed(XTaskQueueGetPort(testData->baseQueue, XTaskQueuePort::Work, &testData->baseQueueWorkPort));
        DX::ThrowIfFailed(XTaskQueueCreateComposite(testData->baseQueueWorkPort, testData->baseQueueWorkPort, &testData->compositeQueue));
        s_sample->Log(u8"Running a task on the compositeQueue for the compositeQueue test");
        RunTask(testData->compositeQueue,
            // Work callback
            []()
            {
                s_sample->Log(u8"Work callback executed on composite queue work port (base queue work port)");
            },
            // Completion callback
            [testData]()
            {
                s_sample->Log(u8"Completion callback executed on composite queue completion port (base queue work port)");

                // Ensures to clean up if this callback is last
                testData->autoCleanupFn(testData);
            });

        // Spawn a thread to handle the manual port of the base queue
        testData->baseQueueDispatcherThread = std::thread(
            [testData]()
            {
                while (true)
                {
                    bool processed = XTaskQueueDispatch(testData->baseQueue, XTaskQueuePort::Completion, INFINITE);
                    if (!processed)
                    {
                        s_sample->Log(u8"XTaskQueueDispatch on baseQueue returned false, exiting thread.");
                        break;
                    }
                }
            });
    }

    // Duplicating Queue Handles
    // Duplicate handles to increase ref count and introduce multiple ownership. For proper closing of the task queue, all handles must be closed.
    {
        DX::ThrowIfFailed(XTaskQueueDuplicateHandle(testData->baseQueue, &testData->duplicateBaseQueueHandle));
        // baseQueue and duplicateBaseQueueHandle now both reference the same queue and its ref count is 2 requiring both handles to be closed in cleanup.
        s_sample->Log(u8"Running a task on the baseQueue for duplicate handles test");
        RunTask(testData->baseQueue,
            // Work callback
            []()
            {
                s_sample->Log(u8"Work callback invoked on baseQueue");
            },
            // Completion callback
            [testData]()
            {
                s_sample->Log(u8"Completion callback invoked on baseQueue");

                // Ensures to clean up if this callback is last
                testData->autoCleanupFn(testData);
            });
        s_sample->Log(u8"Running a task on the duplicateBaseQueueHandle for duplicate handles test");
        RunTask(testData->duplicateBaseQueueHandle,
            // Work callback
            []()
            {
                s_sample->Log(u8"Work callback invoked on baseQueue via duplicateBaseQueueHandle");
            },
            // Completion callback
            [testData]()
            {
                s_sample->Log(u8"Completion callback invoked on baseQueue via duplicateBaseQueueHandle");

                // Ensures to clean up if this callback is last
                testData->autoCleanupFn(testData);
            });
    }

    // Waiters and delayed dispatching
    // Callbacks can be directly submitted to the ports of a task queue with optional wait intervals. In addition, a special waiter can be used
    // to cause the callback to not be submitted until an event is signaled.
    {
        testData->waiterEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        DX::ThrowIfFailed(XTaskQueueRegisterWaiter(testData->baseQueue, XTaskQueuePort::Work, testData->waiterEvent, testData,
            [](void* context, bool /*cancel*/)
            {
                TestData* testData = static_cast<TestData*>(context);

                s_sample->Log(u8"Callback registered via waiter invoked");

                // Ensures to clean up if this callback is last
                testData->autoCleanupFn(testData);
            },
            &testData->waiterToken));
        // First immediately submit callback to the work port of the base queue
        s_sample->Log(u8"Starting waiter and delayed callback submissions test with XTaskQueueSubmitCallback");
        DX::ThrowIfFailed(XTaskQueueSubmitCallback(testData->baseQueue, XTaskQueuePort::Work, testData,
            [](void* context, bool /*cancel*/)
            {
                TestData* testData = static_cast<TestData*>(context);

                s_sample->Log(u8"XTaskQueueSubmitCallback callback invoked, calling XTaskQueueSubmitDelayedCallback");

                // Next submit a delayed callback
                DX::ThrowIfFailed(XTaskQueueSubmitDelayedCallback(testData->baseQueue, XTaskQueuePort::Work, 1000, context,
                    [](void* context, bool /*cancel*/)
                    {
                        TestData* testData = static_cast<TestData*>(context);

                        s_sample->Log(u8"XTaskQueueSubmitDelayedCallback callback invoked, setting auto-reset event to cause callback to be invoked via the waiter");

                        // Finally, use the waiter to fire a callback
                        Sleep(1000);
                        SetEvent(testData->waiterEvent);
                    }));
            }));
    }
}

// XAsyncCancel can be used to cancel requests, but that request must have implemented a cancel path in its provider.
// Not all async requests can be canceled and any request started with XAsyncRun cannot be canceled. This test will show how
// an async request can be implemented using the GDK-style to handle canceling and how to invoke the cancel.
void XAsyncExamples::StartTest_Canceling()
{
    if (m_testInProgress)
    {
        return;
    }
    m_testInProgress = true;

    s_sample->Log(u8"==========================================================================");
    s_sample->Log(u8"Beginning Canceling test");

    XAsyncBlock* async = new XAsyncBlock{};
    ZeroMemory(async, sizeof(XAsyncBlock));
    async->queue = nullptr; // default process queue
    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        XAsyncExamples* examples = static_cast<XAsyncExamples*>(async->context);

        HRESULT status = XAsyncGetStatus(async, false);
        char buffer[256] = {};
        sprintf_s(buffer, 256, u8"CancelableInfiniteTaskAsync() finished with status hr=0x%08X", status);
        s_sample->Log(buffer);
        assert(status == E_ABORT); // This is an abort test, so the status should be E_ABORT

        s_sample->Log(u8"Canceling test finished");
        s_sample->Log(u8"==========================================================================");

        examples->m_testInProgress = false;
        delete async;
    };

    s_sample->Log(u8"Starting CancelableInfiniteTaskAsync() on the default process task queue");
    DX::ThrowIfFailed(CancelableInfiniteTaskAsync(async));

    // Also start a task to cancel the task after a short time
    RunTask(nullptr,
        [async]()
        {
            s_sample->Log(u8"Waiting several seconds before canceling CancelableInfiniteTaskAsync()...");
            Sleep(5250);
            s_sample->Log(u8"Canceling CancelableInfiniteTaskAsync() with XAsyncCancel()");
            XAsyncCancel(async);
        });
}

// It's important to know the overhead of the async systems being used. This test will calculate some overhead costs for different
// usages of the async libraries to get an understanding of the performance. All costs calculated here are background costs and not
// costs of the user async work being dispatched.
void XAsyncExamples::StartTest_OverheadCalculations()
{
    if (m_testInProgress)
    {
        return;
    }
    m_testInProgress = true;

    s_sample->Log(u8"==========================================================================");
    s_sample->Log(u8"Beginning overhead calculation test");

    StopwatchProfiler<Overhead_Total>* overheadProfiler_DefaultProcessQueue = new StopwatchProfiler<Overhead_Total>();
    StopwatchProfiler<Overhead_Total>* overheadProfiler_ManualQueue = new StopwatchProfiler<Overhead_Total>();
    const unsigned int numIterations = 10;
    char buffer[256] = {};

    // Calculate overheads for XAsyncRun - default process task queue
    sprintf_s(buffer, 256, u8"Queuing overhead calculation of XAsyncRun with %d iterations on default process task queue [ThreadPool, ThreadPool]...", numIterations);
    s_sample->Log(buffer);
    for (unsigned int i = 0; i < numIterations; ++i)
    {
        RunTask(m_taskQueue_SerializedAsync,
            [&, overheadProfiler_DefaultProcessQueue]()
            {
                CalculateOverhead_XAsyncRun(nullptr, *overheadProfiler_DefaultProcessQueue);
            });
    }
    RunTask(m_taskQueue_SerializedAsync,
        []()
        {
            s_sample->Log(u8"XAsyncRun overhead calculations on default process task queue complete.");
        });

    // Calculate overheads for XAsyncRun - manual task queue
    sprintf_s(buffer, 256, u8"Queuing overhead calculation of XAsyncRun with %d iterations on manual task queue [Manual, Immediate]...", numIterations);
    s_sample->Log(buffer);
    for (unsigned int i = 0; i < numIterations; ++i)
    {
        RunTask(m_taskQueue_SerializedAsync,
            [&, overheadProfiler_ManualQueue]()
            {
                CalculateOverhead_XAsyncRun(m_taskQueue_ParallelFor, *overheadProfiler_ManualQueue);
            });
    }
    RunTask(m_taskQueue_SerializedAsync,
        []()
        {
            s_sample->Log(u8"XAsyncRun overhead calculations on manual task queue complete.");
        });

    // Calculate overheads for the ParallelFor implemented in this sample
    sprintf_s(buffer, 256, u8"Queuing overhead calculation of ParallelFor with %d iterations...", numIterations);
    s_sample->Log(buffer);
    for (unsigned int i = 0; i < numIterations; ++i)
    {
        RunTask(m_taskQueue_SerializedAsync,
            [&, overheadProfiler_DefaultProcessQueue]()
            {
                CalculateOverhead_ParallelFor(*overheadProfiler_DefaultProcessQueue);
            });
    }
    RunTask(m_taskQueue_SerializedAsync,
        []()
        {
            s_sample->Log(u8"ParallelFor overhead calculations complete.");
        });

    // Calculate overheads for the GDK style API - default process task queue
    sprintf_s(buffer, 256, u8"Queuing overhead calculation of GDK Style APIs with %d iterations on default process task queue [ThreadPool, ThreadPool]...", numIterations);
    s_sample->Log(buffer);
    for (unsigned int i = 0; i < numIterations; ++i)
    {
        RunTask(m_taskQueue_SerializedAsync,
            [&, overheadProfiler_DefaultProcessQueue]()
            {
                CalculateOverhead_GDKAsyncStyle(nullptr, *overheadProfiler_DefaultProcessQueue);
            });
    }
    RunTask(m_taskQueue_SerializedAsync,
        []()
        {
            s_sample->Log(u8"GDK Style APIs overhead calculations on default process task queue complete.");
        });

    // Calculate overheads for the GDK style API - manual task queue
    sprintf_s(buffer, 256, u8"Queuing overhead calculation of GDK Style APIs with %d iterations on manual task queue [Manual, Immediate]...", numIterations);
    s_sample->Log(buffer);
    for (unsigned int i = 0; i < numIterations; ++i)
    {
        RunTask(m_taskQueue_SerializedAsync,
            [&, overheadProfiler_ManualQueue]()
            {
                CalculateOverhead_GDKAsyncStyle(m_taskQueue_ParallelFor, *overheadProfiler_ManualQueue);
            });
    }
    RunTask(m_taskQueue_SerializedAsync,
        []()
        {
            s_sample->Log(u8"GDK Style APIs overhead calculations on manual task queue complete.");
        });

    // Report the data!
    RunTask(m_taskQueue_SerializedAsync,
        [overheadProfiler_DefaultProcessQueue, overheadProfiler_ManualQueue]()
        {
            s_sample->Log(u8"Overhead Timings:");

            // Overhead_XAsyncRun_InvokeToWork
            LogTiming(overheadProfiler_DefaultProcessQueue->GetAccumulator(Overhead_XAsyncRun_InvokeToWork), u8"XAsyncRun_InvokeToWork (Process Default Task Queue)");
            LogTiming(overheadProfiler_ManualQueue->GetAccumulator(Overhead_XAsyncRun_InvokeToWork), u8"XAsyncRun_InvokeToWork (Manual Task Queue)");

            // Overhead_XAsyncRun_WorkToCompletion
            LogTiming(overheadProfiler_DefaultProcessQueue->GetAccumulator(Overhead_XAsyncRun_WorkToCompletion), u8"XAsyncRun_WorkToCompletion (Process Default Task Queue)");
            LogTiming(overheadProfiler_ManualQueue->GetAccumulator(Overhead_XAsyncRun_WorkToCompletion), u8"XAsyncRun_WorkToCompletion (Manual Task Queue)");

            // Overhead_ParallelFor_InvokeToBody
            LogTiming(overheadProfiler_DefaultProcessQueue->GetAccumulator(Overhead_ParallelFor_InvokeToBody), u8"ParallelFor_InvokeToWork");

            // Overhead_ParallelFor_InvokeToReturn
            LogTiming(overheadProfiler_DefaultProcessQueue->GetAccumulator(Overhead_ParallelFor_InvokeToReturn), u8"ParallelFor_InvokeToReturn");

            // Overhead_GDKAsyncStyle_TimeInProviderAverage
            LogTiming(overheadProfiler_DefaultProcessQueue->GetAccumulator(Overhead_GDKAsyncStyle_TimeInProviderAverage), u8"GDKAsyncStyle_TimeInProviderAverage (Process Default Task Queue)");
            LogTiming(overheadProfiler_ManualQueue->GetAccumulator(Overhead_GDKAsyncStyle_TimeInProviderAverage), u8"GDKAsyncStyle_TimeInProviderAverage (Manual Task Queue)");

            // Overhead_GDKAsyncStyle_TimeInProviderOverall
            LogTiming(overheadProfiler_DefaultProcessQueue->GetAccumulator(Overhead_GDKAsyncStyle_TimeInProviderOverall), u8"GDKAsyncStyle_TimeInProviderOverall (Process Default Task Queue)");
            LogTiming(overheadProfiler_ManualQueue->GetAccumulator(Overhead_GDKAsyncStyle_TimeInProviderOverall), u8"GDKAsyncStyle_TimeInProviderOverall (Manual Task Queue)");

            // Overhead_GDKAsyncStyle_InvokeToWork
            LogTiming(overheadProfiler_DefaultProcessQueue->GetAccumulator(Overhead_GDKAsyncStyle_InvokeToWork), u8"GDKAsyncStyle_InvokeToWork (Process Default Task Queue)");
            LogTiming(overheadProfiler_ManualQueue->GetAccumulator(Overhead_GDKAsyncStyle_InvokeToWork), u8"GDKAsyncStyle_InvokeToWork (Manual Task Queue)");

            // Overhead_GDKAsyncStyle_WorkToCompletion
            LogTiming(overheadProfiler_DefaultProcessQueue->GetAccumulator(Overhead_GDKAsyncStyle_WorkToCompletion), u8"GDKAsyncStyle_WorkToCompletion (Process Default Task Queue)");
            LogTiming(overheadProfiler_ManualQueue->GetAccumulator(Overhead_GDKAsyncStyle_WorkToCompletion), u8"GDKAsyncStyle_WorkToCompletion (Manual Task Queue)");
        });

    // Cleanup test
    RunTask(m_taskQueue_SerializedAsync,
        [&, overheadProfiler_DefaultProcessQueue, overheadProfiler_ManualQueue]()
        {
            delete overheadProfiler_DefaultProcessQueue;
            delete overheadProfiler_ManualQueue;

            s_sample->Log(u8"Overhead calculation test finished");
            s_sample->Log(u8"==========================================================================");
            m_testInProgress = false;
        });
}

void XAsyncExamples::CalculateOverhead_XAsyncRun(XTaskQueueHandle taskQueue, StopwatchProfiler<Overhead_Total>& overheadProfiler)
{
    struct CallContext
    {
        StopwatchProfiler<Overhead_Total>* profiler;
        HANDLE waitEvent;
    };
    CallContext* callContext = new CallContext();
    callContext->profiler = &overheadProfiler;
    callContext->waitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    XAsyncBlock* async = new XAsyncBlock{};
    ZeroMemory(async, sizeof(XAsyncBlock));
    async->queue = taskQueue;
    async->context = callContext;
    async->callback = [](XAsyncBlock* async)
    {
        CallContext* callContext = static_cast<CallContext*>(async->context);

        callContext->profiler->RecordCurrentTiming(Overhead_XAsyncRun_WorkToCompletion);
        callContext->profiler->Stop(Overhead_XAsyncRun_WorkToCompletion);

        SetEvent(callContext->waitEvent);
    };

    // Callback passed to XAsyncRun is the work callback
    overheadProfiler.Start(Overhead_XAsyncRun_InvokeToWork);
    XAsyncRun(async,
        [](XAsyncBlock* async)->HRESULT
        {
            CallContext* callContext = static_cast<CallContext*>(async->context);

            callContext->profiler->RecordCurrentTiming(Overhead_XAsyncRun_InvokeToWork);
            callContext->profiler->Stop(Overhead_XAsyncRun_InvokeToWork);

            callContext->profiler->Start(Overhead_XAsyncRun_WorkToCompletion);

            return S_OK;
        });

    // Wait for test to complete. We only want 1 test at a time to get accurate timings.
    WaitForSingleObject(callContext->waitEvent, INFINITE);
    CloseHandle(callContext->waitEvent);
    delete async;
}

void XAsyncExamples::CalculateOverhead_ParallelFor(StopwatchProfiler<Overhead_Total>& overheadProfiler)
{
    // Include setup time since we're timing the whole ParallelFor invocation
    overheadProfiler.Start(Overhead_ParallelFor_InvokeToBody);
    overheadProfiler.Start(Overhead_ParallelFor_InvokeToReturn);

    // How many for this test
    const unsigned int startIndex = 0;
    const unsigned int endIndex = 100;

    // Setup async blocks
    const unsigned int numInvocations = endIndex - startIndex;
    XAsyncBlock* asyncBlocks = new XAsyncBlock[numInvocations];
    ZeroMemory(asyncBlocks, sizeof(XAsyncBlock) * numInvocations);

    // Setup call data
    struct CallContext
    {
        double timeInvokeToBody;
        StopwatchProfiler<Overhead_Total>* profiler;
        HANDLE completionEvent;
    };
    CallContext* callContexts = new CallContext[numInvocations];

    // Start all the async methods
    for (unsigned int i = startIndex; i < endIndex; ++i)
    {
        CallContext* context = callContexts + (i - startIndex);
        context->timeInvokeToBody = 0.0;
        context->profiler = &overheadProfiler;
        context->completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        XAsyncBlock* async = asyncBlocks + (i - startIndex);
        async->queue = s_singleton->m_taskQueue_ParallelFor;
        async->context = context;
        async->callback = [](XAsyncBlock* async)
        {
            CallContext* context = static_cast<CallContext*>(async->context);
            SetEvent(context->completionEvent);
        };
        XAsyncRun(async,
            [](XAsyncBlock* async)->HRESULT
            {
                CallContext* context = static_cast<CallContext*>(async->context);

                // Store the profiler time instead of tracking it directly since the tracking has a lock and we want the
                // most accurate timing
                context->timeInvokeToBody = context->profiler->GetStopWatch(Overhead_ParallelFor_InvokeToBody)->GetCurrentSeconds();

                return S_OK;
            });
    }

    // Wait for async methods to complete
    for (unsigned int i = 0; i < numInvocations; ++i)
    {
        WaitForSingleObject(callContexts[i].completionEvent, INFINITE);
    }

    // Record all the Invoke-To-Body times
    overheadProfiler.Stop(Overhead_ParallelFor_InvokeToBody);
    for (unsigned int i = startIndex; i < endIndex; ++i)
    {
        CallContext* context = callContexts + (i - startIndex);
        overheadProfiler.GetAccumulator(Overhead_ParallelFor_InvokeToBody)->PushValue(context->timeInvokeToBody);
    }

    for (unsigned int i = 0; i < numInvocations; ++i)
    {
        CloseHandle(callContexts[i].completionEvent);
    }
    delete[] callContexts;
    delete[] asyncBlocks;

    overheadProfiler.RecordCurrentTiming(Overhead_ParallelFor_InvokeToReturn);
    overheadProfiler.Stop(Overhead_ParallelFor_InvokeToReturn);
}

void XAsyncExamples::CalculateOverhead_GDKAsyncStyle(XTaskQueueHandle taskQueue, StopwatchProfiler<Overhead_Total>& overheadProfiler)
{
    // Setup XAsyncBlock
    XAsyncBlock* async = new XAsyncBlock{};
    ZeroMemory(async, sizeof(XAsyncBlock));
    async->queue = taskQueue;
    async->context = &overheadProfiler;
    // Completion callback
    async->callback =
        [](XAsyncBlock* async)
        {
            StopwatchProfiler<Overhead_Total>* profiler = static_cast<StopwatchProfiler<Overhead_Total>*>(async->context);

            profiler->RecordCurrentTiming(Overhead_GDKAsyncStyle_WorkToCompletion);
            profiler->Stop(Overhead_GDKAsyncStyle_WorkToCompletion);
        };

    // This is where an invoke would start for a normal call to the GDK style API
    overheadProfiler.Start(Overhead_GDKAsyncStyle_InvokeToWork);

    struct CallData
    {
        StopwatchProfiler<Overhead_Total>* profiler;
        double* timeInProviderOverall;
    };

    CallData* callData = new CallData();
    callData->profiler = &overheadProfiler;
    double* timeInProviderOverall = new double(0.0);
    callData->timeInProviderOverall = timeInProviderOverall;

    DX::ThrowIfFailed(XAsyncBegin(async, callData, nullptr, __FUNCTION__,
        [](XAsyncOp op, const XAsyncProviderData* providerData)
        {
            CallData* callData = (CallData*)providerData->context;

            // Using a local stopwatch since this provider can be used in parallel
            RDTSCPStopWatch stopWatch;
            stopWatch.Start();

            HRESULT hr = S_OK;

            switch (op)
            {
            case XAsyncOp::Begin:
                hr = XAsyncSchedule(providerData->async, 0);
                break;

            case XAsyncOp::DoWork:
                // "Work Callback" would be invoked here
                callData->profiler->RecordCurrentTiming(Overhead_GDKAsyncStyle_InvokeToWork);
                callData->profiler->Stop(Overhead_GDKAsyncStyle_InvokeToWork);
                callData->profiler->Start(Overhead_GDKAsyncStyle_WorkToCompletion);

                XAsyncComplete(providerData->async, S_OK, 0);
                break;

            case XAsyncOp::Cleanup:
            case XAsyncOp::GetResult:
            case XAsyncOp::Cancel:
                break;
            }

            const double seconds = stopWatch.GetCurrentSeconds();
            callData->profiler->GetAccumulator(Overhead_GDKAsyncStyle_TimeInProviderAverage)->PushValue(seconds);
            (*callData->timeInProviderOverall) += seconds;

            return hr;
        }));

    // Wait for whole async operation to finish, and some extra time to ensure the threaded tasks can complete fully
    // XAsyncGetStatus can quickly return when XAsyncComplete is called on a separate thread, but the callback back has extra work at the bottom.
    // The sleep ensures that thread completes fully before cleanup below.
    DX::ThrowIfFailed(XAsyncGetStatus(async, true));
    Sleep(100);
    overheadProfiler.GetAccumulator(Overhead_GDKAsyncStyle_TimeInProviderOverall)->PushValue((*callData->timeInProviderOverall));
    delete timeInProviderOverall;
    delete callData;
    delete async;
}

bool XAsyncExamples::IsPrime(uint64_t num)
{
    if (num < 2)
    {
        return false;
    }

    for (uint64_t counter = 2; counter < num; ++counter)
    {
        if (num % counter == 0)
        {
            return false;
        }
    }
    return true;
}

uint64_t XAsyncExamples::NthPrime(uint64_t n)
{
    if (n == 0)
    {
        return 1;
    }

    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"Beginning calculating Nth prime %llu", n);
    s_sample->Log(buffer);

    double startTime = GetTime();

    uint64_t currentN = 1;
    uint64_t currentNum = 2;
    while (currentN <= n)
    {
        if (IsPrime(currentNum))
        {
            if (currentN == n)
            {
                double endTime = GetTime();

                sprintf_s(buffer, 256, u8"Finished calculating Nth prime %llu as %llu. Took %.2f seconds.", n, currentNum, endTime - startTime);
                s_sample->Log(buffer);

                return currentNum;
            }
            else
            {
                ++currentN;
            }
        }

        ++currentNum;
    }

    sprintf_s(buffer, 256, u8"Failed to calculate Nth prime %llu", n);
    s_sample->Log(buffer);

    return 0;
}

HRESULT XAsyncExamples::NthPrimeAsync(uint64_t n, XAsyncBlock* async)
{
    struct CallData
    {
        uint64_t value;
        uint64_t result;
    };

    CallData* callData = new CallData();
    callData->value = n;
    callData->result = 1;

    HRESULT hr = XAsyncBegin(async, callData, NthPrimeAsync, __FUNCTION__,
        [](XAsyncOp op, const XAsyncProviderData* providerData)
        {
            CallData* callData = (CallData*)providerData->context;

            switch (op)
            {
            case XAsyncOp::Begin:
                return XAsyncSchedule(providerData->async, 0);

            case XAsyncOp::Cleanup:
                delete callData;
                break;

            case XAsyncOp::GetResult:
                memcpy(providerData->buffer, &callData->result, sizeof(uint64_t));
                break;

            case XAsyncOp::DoWork:
                callData->result = NthPrime(callData->value);
                XAsyncComplete(providerData->async, S_OK, sizeof(uint64_t));
                break;

            case XAsyncOp::Cancel:
                // This call can't be canceled
                break;
            }

            return S_OK;
        });

    return hr;
}

HRESULT XAsyncExamples::NthPrimeAsyncResult(XAsyncBlock* async, uint64_t* result)
{
    return XAsyncGetResult(async, NthPrimeAsync, sizeof(uint64_t), result, nullptr);
}

HRESULT XAsyncExamples::CancelableInfiniteTaskAsync(XAsyncBlock* async)
{
    struct CallData
    {
        HANDLE cancelEvent;
    };
    CallData* callData = new CallData{ CreateEvent(NULL, FALSE, FALSE, NULL) };

    HRESULT hr = XAsyncBegin(async, callData, CancelableInfiniteTaskAsync, __FUNCTION__,
        [](XAsyncOp op, const XAsyncProviderData* providerData)
        {
            CallData* callData = static_cast<CallData*>(providerData->context);

            switch (op)
            {
            case XAsyncOp::Begin:
                return XAsyncSchedule(providerData->async, 0);

            case XAsyncOp::DoWork:
            {
                bool canceled = false;
                while (true)
                {
                    s_sample->Log(u8"CancelableInfiniteTaskAsync() work still looping indefinitely");

                    DWORD waitResult = WaitForSingleObject(callData->cancelEvent, 0);
                    if (waitResult != WAIT_TIMEOUT)
                    {
                        s_sample->Log(u8"Cancel detected, breaking out of CancelableInfiniteTaskAsync() wait loop");
                        canceled = true;
                        break;
                    }

                    Sleep(1000);
                }

                if (canceled)
                {
                    s_sample->Log(u8"Completing CancelableInfiniteTaskAsync() with E_ABORT");
                    XAsyncComplete(providerData->async, E_ABORT, 0);
                }
                else
                {
                    // Normal completion should call this. However, in this cancel case, this won't get called
                    s_sample->Log(u8"Completing CancelableInfiniteTaskAsync() with S_OK");
                    XAsyncComplete(providerData->async, S_OK, 0);
                }

                break;
            }

            case XAsyncOp::Cancel:
                // Signal to our work to cancel
                s_sample->Log(u8"Provider called with Cancel request. Setting event to inform work to cancel.");
                SetEvent(callData->cancelEvent);
                break;

            case XAsyncOp::Cleanup:
                CloseHandle(callData->cancelEvent);
                delete callData;
                break;

            case XAsyncOp::GetResult:
                break;
            }

            return S_OK;
        });

    return hr;
}

void XAsyncExamples::ParallelFor(unsigned int startIndex, unsigned int endIndex, std::function<void(unsigned int)> bodyFunction)
{
    // Setup async blocks
    assert(endIndex > startIndex);
    const unsigned int numInvocations = endIndex - startIndex;
    XAsyncBlock* asyncBlocks = new XAsyncBlock[numInvocations];
    ZeroMemory(asyncBlocks, sizeof(XAsyncBlock) * numInvocations);

    // Setup call data
    struct CallContext
    {
        std::function<void(unsigned int)> bodyFunction;
        unsigned int index;
        HANDLE completionEvent;
    };
    CallContext* callContexts = new CallContext[numInvocations];

    // Start all the async methods
    for (unsigned int i = startIndex; i < endIndex; ++i)
    {
        CallContext* context = callContexts + (i - startIndex);
        context->bodyFunction = bodyFunction;
        context->index = i;
        context->completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        XAsyncBlock* async = asyncBlocks + (i - startIndex);
        async->queue = s_singleton->m_taskQueue_ParallelFor;
        async->context = context;
        async->callback = [](XAsyncBlock* async)
        {
            CallContext* context = static_cast<CallContext*>(async->context);
            SetEvent(context->completionEvent);
        };
        XAsyncRun(async,
            [](XAsyncBlock* async)->HRESULT
            {
                CallContext* context = static_cast<CallContext*>(async->context);
                context->bodyFunction(context->index);
                return S_OK;
            });
    }

    // Wait for async methods to complete
    for (unsigned int i = 0; i < numInvocations; ++i)
    {
        WaitForSingleObject(callContexts[i].completionEvent, INFINITE);
    }
    for (unsigned int i = 0; i < numInvocations; ++i)
    {
        CloseHandle(callContexts[i].completionEvent);
    }
    delete[] callContexts;
    delete[] asyncBlocks;
}

void XAsyncExamples::ParallelExecute(unsigned int numTasks, XTaskQueueHandle taskQueue, std::function<void(unsigned int)> bodyFunction)
{
    // Setup async blocks
    XAsyncBlock* asyncBlocks = new XAsyncBlock[numTasks];
    ZeroMemory(asyncBlocks, sizeof(XAsyncBlock) * numTasks);

    // Setup call data
    struct CallContext
    {
        std::function<void(unsigned int)> bodyFunction;
        unsigned int index;
        HANDLE completionEvent;
    };
    CallContext* callContexts = new CallContext[numTasks];

    // Start all the async methods
    for (unsigned int i = 0; i < numTasks; ++i)
    {
        CallContext* context = callContexts + i;
        context->bodyFunction = bodyFunction;
        context->index = i;
        context->completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        XAsyncBlock* async = asyncBlocks + i;
        async->queue = taskQueue;
        async->context = context;
        async->callback = [](XAsyncBlock* async)
        {
            CallContext* context = static_cast<CallContext*>(async->context);
            SetEvent(context->completionEvent);
        };
        XAsyncRun(async,
            [](XAsyncBlock* async)->HRESULT
            {
                CallContext* context = static_cast<CallContext*>(async->context);
                context->bodyFunction(context->index);
                return S_OK;
            });
    }

    // Wait for async methods to complete
    for (unsigned int i = 0; i < numTasks; ++i)
    {
        WaitForSingleObject(callContexts[i].completionEvent, INFINITE);
    }
    for (unsigned int i = 0; i < numTasks; ++i)
    {
        CloseHandle(callContexts[i].completionEvent);
    }
    delete[] callContexts;
    delete[] asyncBlocks;
}

void XAsyncExamples::RunTask(XTaskQueueHandle taskQueue, std::function<void()> workCallback, std::function<void()> completionCallback)
{
    struct RunTaskContext
    {
        std::function<void()> workCallback;
        std::function<void()> completionCallback;
    };
    RunTaskContext* context = new RunTaskContext();
    context->workCallback = workCallback;
    context->completionCallback = completionCallback;

    XAsyncBlock* async = new XAsyncBlock{};
    ZeroMemory(async, sizeof(XAsyncBlock));
    async->queue = taskQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        RunTaskContext* context = static_cast<RunTaskContext*>(async->context);

        context->completionCallback();

        delete context;
        delete async;
    };

    // Callback passed to XAsyncRun is the work callback
    XAsyncRun(async,
        [](XAsyncBlock* async)->HRESULT
        {
            RunTaskContext* context = static_cast<RunTaskContext*>(async->context);

            context->workCallback();

            return S_OK;
        });
}

void XAsyncExamples::RunTask(XTaskQueueHandle taskQueue, std::function<void()> workCallback)
{
    struct RunTaskContext
    {
        std::function<void()> workCallback;
    };
    RunTaskContext* context = new RunTaskContext();
    context->workCallback = workCallback;

    XAsyncBlock* async = new XAsyncBlock{};
    ZeroMemory(async, sizeof(XAsyncBlock));
    async->queue = taskQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        RunTaskContext* context = static_cast<RunTaskContext*>(async->context);

        delete context;
        delete async;
    };

    // Callback passed to XAsyncRun is the work callback
    XAsyncRun(async,
        [](XAsyncBlock* async)->HRESULT
        {
            RunTaskContext* context = static_cast<RunTaskContext*>(async->context);

            context->workCallback();

            return S_OK;
        });
}
