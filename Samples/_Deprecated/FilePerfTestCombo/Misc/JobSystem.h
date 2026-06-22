//--------------------------------------------------------------------------------------
// JobSystem.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "WorkStealDequeFunc.h"
#include "WorkStealDequeContainer.h"

namespace ATG
{
    namespace JobSystem
    {
        uint64_t JobThreadFunc(uint64_t queueId, int32_t jobThreadPriority, bool busySpin);

        extern std::atomic<bool> g_jobShutdownRequest;
        extern std::atomic<uint64_t> g_waitingJobs;
        typedef ATG::WorkSteal::WorkStealDequeContainer<ATG::WorkSteal::DequeFunc<>> DequeContainer;
        extern DequeContainer* g_jobDeques;

        void CreateJobQueues(uint64_t numThreads, uint64_t numDeques, int32_t jobThreadPriority = 0 /*THREAD_PRIORITY_NORMAL*/, bool busySpin = false);
        void CleanupJobQueues();
        void PushNewJob(const ATG::WorkSteal::DequeFunc<>::DataType& job, uint64_t queueIndex, bool front = false);
        bool InvokeSingleJob(uintptr_t& output, uint64_t queueIndex = 0);
    }
}
