//--------------------------------------------------------------------------------------
// JobThread.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "JobSystem.h"

#include "WorkStealDequeContainer.h"
#include "WorkStealDequeFunc.h"

namespace ATG
{
    namespace JobSystem
    {
        extern std::atomic<bool> g_jobShutdownRequest;

        typedef ATG::WorkSteal::WorkStealDequeContainer<ATG::WorkSteal::DequeFunc<>> DequeContainer;
        extern DequeContainer* g_jobDeques;

        uint64_t JobThreadFunc(uint64_t queueId, int32_t jobThreadPriority, bool busySpin)
        {
            HANDLE curThread = GetCurrentThread();
            int32_t workingThreadPriority = jobThreadPriority;
            int32_t waitingThreadPriority = jobThreadPriority - 2;
            int32_t curPriority = waitingThreadPriority;
            SetThreadPriority(curThread, curPriority);

            while (!g_jobShutdownRequest.load(std::memory_order_relaxed))
            {
                if (curPriority != workingThreadPriority)
                {
                    curPriority = workingThreadPriority;
                    SetThreadPriority(curThread, curPriority);
                }

                bool jobDone = false;
                bool ownJobDone = false;
                uintptr_t jobOutput;
                switch (g_jobDeques->invoke_front(queueId, jobOutput, busySpin))
                {
                case DequeContainer::PopDataType::OWNED_ELEMENT:
                    ownJobDone = true;
                    jobDone = true;
                    break;
                case DequeContainer::PopDataType::STOLEN_ELEMENT:
                    jobDone = true;
                    break;
                case DequeContainer::PopDataType::NO_ELEMENT:
                    break;
                }
                if (jobDone)
                    g_waitingJobs--;

                if (!jobDone)
                {
                    // we didn't find anything to do so drop the priority, let another thread preempt if it needs to
                    if (curPriority != waitingThreadPriority)
                    {
                        curPriority = waitingThreadPriority;
                        SetThreadPriority(curThread, curPriority);
                    }
                }
            }
            return 0;
        }
    }
}
