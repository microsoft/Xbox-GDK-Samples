//--------------------------------------------------------------------------------------
// JobSystem.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "JobSystem.h"

#include "WorkStealDequeContainer.h"
#include "WorkStealDequeFunc.h"
#include "Processor.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ATG
{
    namespace JobSystem
    {
        std::atomic<bool> g_jobShutdownRequest(false);
        std::atomic<uint64_t> g_waitingJobs(0);

        std::thread** g_jobThreads = nullptr;

        uint64_t g_numJobThreads = 0;
        uint64_t g_numJobDeques = 0;
        ATG::WorkSteal::WorkStealDequeContainer<ATG::WorkSteal::DequeFunc<>>* g_jobDeques = nullptr;

        bool InvokeSingleJob(uintptr_t& jobOutput, uint64_t queueIndex)
        {
            bool jobDone = false;
            switch (g_jobDeques->invoke_front(queueIndex, jobOutput, false))
            {
            case DequeContainer::PopDataType::OWNED_ELEMENT:
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
            return jobDone;
        }

        void PushNewJob(const ATG::WorkSteal::DequeFunc<>::DataType& job, uint64_t queueIndex, bool front)
        {
            g_waitingJobs++;
            if (front)
                g_jobDeques->push_front(queueIndex, job);
            else
                g_jobDeques->push_back(queueIndex, job);
        }
        void CreateJobQueues(uint64_t numThreads, uint64_t numDeques, int32_t jobThreadPriority, bool busySpin)
        {
            CleanupJobQueues();
            g_jobShutdownRequest = false;
            g_numJobDeques = numDeques;

            g_jobDeques = new ATG::WorkSteal::WorkStealDequeContainer<ATG::WorkSteal::DequeFunc<>>(numDeques);

            if (numThreads > ATG::GetTotalNumberCoresIncludingNuma())
                numThreads = ATG::GetTotalNumberCoresIncludingNuma();

            g_numJobThreads = numThreads;

            uint32_t coreHyperthreadMultiplier = 1;
            if (ATG::IsSMTSupported() && (numThreads <= 32))
                coreHyperthreadMultiplier = 2;
            g_jobThreads = new std::thread * [numThreads];
            std::vector<uint16_t> groupAffinity;
#if !defined(_GAMING_XBOX)
            if (numThreads > 64)                    // Each set of 64 cores is broken into a separate group, extra work must be done to bind threads to other groups than the default process group
            {
                ATG::UniqueProcessorMask currentMask;
                uint64_t currentGroupNumCores = ATG::GetNumGroupCores(currentMask.groupID);
                uint32_t currentGroupCore = 0;
                currentMask.coreMask = 0x01;
                for (uint64_t i = 0; i < g_numJobThreads; i++)
                {
                    g_jobThreads[i] = new std::thread(JobThreadFunc, i % g_numJobDeques, jobThreadPriority, busySpin);
                    ATG::SetThreadAffinityMask(g_jobThreads[i]->native_handle(), currentMask);
                    currentGroupCore++;
                    if (currentGroupCore == currentGroupNumCores)
                    {
                        currentGroupCore = 0;
                        currentMask.groupID++;
                        currentGroupNumCores = ATG::GetNumGroupCores(currentMask.groupID);
                        currentMask.coreMask = 0x01ULL << currentGroupCore;
                    }
                    else
                    {
                        currentMask.coreMask = 0x01ULL << currentGroupCore;
                    }
                }
                ATG::GetProcessGroupAffinity(groupAffinity);
            }
            else
#endif
            {
                for (uint64_t i = 0; i < g_numJobThreads; i++)
                {
                    g_jobThreads[i] = new std::thread(JobThreadFunc, i % g_numJobDeques, jobThreadPriority, busySpin);

                    // going to keep them off core 0
                    SetThreadAffinityMask(g_jobThreads[i]->native_handle(), 0x01ULL << ((i + 1) * coreHyperthreadMultiplier));
                }
            }
        }

        void CleanupJobQueues()
        {
            g_jobShutdownRequest = true;
            if (g_jobDeques)
                g_jobDeques->wake_all();
            for (uint64_t i = 0; i < g_numJobThreads; i++)
            {
                g_jobThreads[i]->join();
                delete g_jobThreads[i];
            }
            delete[] g_jobThreads;
            delete g_jobDeques;
            g_jobDeques = nullptr;
            g_jobThreads = nullptr;
            g_numJobThreads = 0;
            g_numJobDeques = 0;
        }
    }
}
