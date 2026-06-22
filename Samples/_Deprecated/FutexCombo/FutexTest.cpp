//--------------------------------------------------------------------------------------
// FutexTest.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "FutexCombo.h"
#include "UserFutex.h"
#include "UserSlowtex.h"
#include "NullTex.h"
#include "RDTSCPStopWatch.h"
#include "Random.h"
#include "Processor.h"

using namespace DirectX;

namespace FutexTest
{
    constexpr uint32_t c_numThreads = 4;
    constexpr uint32_t c_firstTestRuntimeSecs = 2;      // quick first runtime to get numbers
    constexpr uint32_t c_laterTestRuntimeSecs = 5;      // later test runs execute longer for more accurate data

    std::map<Sample::ContentionLevel, std::pair<uint32_t, uint32_t>> backgroundThreadWorkTimes =
    {
        { Sample::ContentionLevel::e_lowContention, std::pair<uint32_t,uint32_t>(750,500)},
        { Sample::ContentionLevel::e_mediumContention, std::pair<uint32_t,uint32_t>(750,500)},
        { Sample::ContentionLevel::e_highContention, std::pair<uint32_t,uint32_t>(750,500)}
    };
    std::map<Sample::ContentionLevel, std::pair<uint32_t, uint32_t>> backgroundThreadMutexHoldTimes =
    {
        { Sample::ContentionLevel::e_lowContention, std::pair<uint32_t,uint32_t>(0,0)},
        { Sample::ContentionLevel::e_mediumContention, std::pair<uint32_t,uint32_t>(0,0)},
        { Sample::ContentionLevel::e_highContention, std::pair<uint32_t,uint32_t>(0,0)}
    };
    std::map<Sample::ContentionLevel, std::pair<uint32_t, uint32_t>> workerThreadWorkTimes =
    {
        { Sample::ContentionLevel::e_lowContention, std::pair<uint32_t,uint32_t>(500,200)},
        { Sample::ContentionLevel::e_mediumContention, std::pair<uint32_t,uint32_t>(250,200)},
        { Sample::ContentionLevel::e_highContention, std::pair<uint32_t,uint32_t>(100,100)}
    };
    std::map<Sample::ContentionLevel, std::pair<uint32_t, uint32_t>> workerThreadMutexHoldTimes =
    {
        { Sample::ContentionLevel::e_lowContention, std::pair<uint32_t,uint32_t>(25,50)},
        { Sample::ContentionLevel::e_mediumContention, std::pair<uint32_t,uint32_t>(25,50)},
        { Sample::ContentionLevel::e_highContention, std::pair<uint32_t,uint32_t>(25,50)}
    };

    std::vector<std::thread*> g_workerThreads;
    std::vector<std::thread*> g_backgroundThreads;

    Sample* g_sample = nullptr;
    std::thread* g_testThread;

    std::atomic<bool> g_runTests;
    std::atomic<bool> g_stopThreads;
    std::atomic<bool> g_threadsStart;
    XMMATRIX g_matrices[c_numThreads][128];
    const uint32_t c_numTimeSlots = 100000;
    std::vector<uint64_t> g_workerWorkDone[c_numThreads];
    std::vector<uint64_t> g_backgroundWorkDone[c_numThreads];
    uint64_t g_totalBackgroundWorkPerThread[c_numTimeSlots];
    uint64_t g_totalWorkerWorkPerThread[c_numTimeSlots];
    uint64_t g_totalBackgroundWork(0);
    uint64_t g_totalWorkerWork(0);
    double g_wastedTimeUS;

    void WorkerFloatingPoint256(uint32_t index)
    {
        uint64_t M0 = 0;
        uint64_t M1 = 1;
        uint64_t M2 = 2;
        uint64_t M3 = 3;

        for (uint32_t i = 0; i < (128 - M3); i++)
        {
            g_matrices[index][i + M2] = DirectX::XMMatrixMultiply(g_matrices[index][i + M0], g_matrices[index][i + M1]);
        }
    }

    template<class mutexObjectType>
    void WorkerThread(uint32_t threadIndex, bool background, uint64_t threadAffinityMask, int threadPriority, mutexObjectType* mutexObject, const std::pair<uint32_t, uint32_t>& workTimes, const std::pair<uint32_t, uint32_t>& mutexHoldTimes, uint32_t matrixIndexOverride)
    {
        ATG::RDTSCPStopWatch slotTimer;
        SetThreadPriority(GetCurrentThread(), threadPriority);
        SetThreadAffinityMask(GetCurrentThread(), threadAffinityMask);

        while (!g_threadsStart) { _mm_pause(); }
        ATG::RDTSCPStopWatch timer;
        timer.Start();
        slotTimer.Start();
        double workTime = ATG::GetRandomValue<uint32_t>(workTimes.second) + workTimes.first;
        while (!g_stopThreads)
        {
            if (timer.GetCurrentMicroseconds() > workTime)
            {
                workTime = ATG::GetRandomValue<uint32_t>(workTimes.second) + workTimes.first;
                if (mutexHoldTimes.second != 0)
                {
                    double lockTime = ATG::GetRandomValue<uint32_t>(mutexHoldTimes.second) + mutexHoldTimes.first;
                    mutexObject->lock();
                    timer.Reset();
                    while (timer.GetCurrentMicroseconds() < lockTime)
                    {
                        WorkerFloatingPoint256(matrixIndexOverride);
                        if (background)
                            g_backgroundWorkDone[threadIndex][(uint32_t)slotTimer.GetCurrentMilliseconds() % c_numTimeSlots]++;
                        else
                            g_workerWorkDone[threadIndex][(uint32_t)slotTimer.GetCurrentMilliseconds() % c_numTimeSlots]++;
                    }
                    mutexObject->unlock();
                }
                timer.Reset();
            }
            WorkerFloatingPoint256(matrixIndexOverride);
            if (background)
                g_backgroundWorkDone[threadIndex][(uint32_t)slotTimer.GetCurrentMilliseconds() % c_numTimeSlots]++;
            else
                g_workerWorkDone[threadIndex][(uint32_t)slotTimer.GetCurrentMilliseconds() % c_numTimeSlots]++;
        }
    }

    template<class mutexObjectType>
    void PerformOneTestRun(bool threadsLockedToCores, mutexObjectType* mutexObject, std::pair<uint32_t, uint32_t> backgroundWorkTimes, std::pair<uint32_t, uint32_t> backgroundMutexHoldTimes, std::pair<uint32_t, uint32_t> workerWorkTimes, std::pair<uint32_t, uint32_t> workerMutexHoldTimes, uint32_t testRunTimeSecs)
    {
        g_threadsStart = false;
        g_stopThreads = false;
        for (uint32_t index = 0; index < c_numThreads; index++)
        {
            g_workerWorkDone[index].clear();
            g_backgroundWorkDone[index].clear();
            g_workerWorkDone[index].resize(c_numTimeSlots, 0);
            g_backgroundWorkDone[index].resize(c_numTimeSlots, 0);

            uint64_t threadAffinityMask = ATG::GetTopLevelCacheCoreMask(0);
            if (threadsLockedToCores)
                threadAffinityMask = 1ULL << (index * (ATG::IsSMTSupported() ? 2 : 1));
            g_workerThreads.push_back(new std::thread(std::bind(WorkerThread<mutexObjectType>, index, false, threadAffinityMask, THREAD_PRIORITY_HIGHEST, mutexObject, workerWorkTimes, workerMutexHoldTimes, index)));
            g_backgroundThreads.push_back(new std::thread(std::bind(WorkerThread<mutexObjectType>, index, true, threadAffinityMask, THREAD_PRIORITY_LOWEST, mutexObject, backgroundWorkTimes, backgroundMutexHoldTimes, index)));

            wchar_t buffer[64];
            swprintf(buffer, 64, L"Worker: %d", index);
            SetThreadDescription(g_workerThreads[index]->native_handle(), buffer);
            swprintf(buffer, 64, L"Background: %d", index);
            SetThreadDescription(g_backgroundThreads[index]->native_handle(), buffer);
        }
        g_threadsStart = true;

        Sleep(testRunTimeSecs * 1000);

        g_stopThreads = true;

        for (auto& iter : g_backgroundThreads)
        {
            iter->join();
            delete iter;
        }
        for (auto& iter : g_workerThreads)
        {
            iter->join();
            delete iter;
        }
        g_workerThreads.clear();
        g_backgroundThreads.clear();
        g_wastedTimeUS = mutexObject->getWastedTimeUS();
    }

    void CalculateMetrics(Sample* sample, Sample::MutexType mutexType, Sample::ContentionLevel contentionLevel, bool threadsLocked, uint32_t testRunTimeSecs)
    {
        for (uint32_t i = 0; i < c_numThreads; i++)
        {
            std::sort(g_workerWorkDone[i].begin(), g_workerWorkDone[i].end(), [&](const uint64_t& lhs, const uint64_t& rhs) {return rhs < lhs; });
            std::sort(g_backgroundWorkDone[i].begin(), g_backgroundWorkDone[i].end(), [&](const uint64_t& lhs, const uint64_t& rhs) {return rhs < lhs; });
            for (uint32_t j = 0; j < c_numTimeSlots; j++)
            {
                if (g_workerWorkDone[i][j] == 0)
                {
                    g_workerWorkDone[i].resize(j);
                    break;
                }
            }
            for (uint32_t j = 0; j < c_numTimeSlots; j++)
            {
                if (g_backgroundWorkDone[i][j] == 0)
                {
                    g_backgroundWorkDone[i].resize(j);
                    break;
                }
            }
        }

        for (uint32_t i = 0; i < c_numThreads; i++)
        {
            g_totalBackgroundWorkPerThread[i] = 0;
            g_totalWorkerWorkPerThread[i] = 0;
            for (auto& iter : g_backgroundWorkDone[i])
            {
                iter /= testRunTimeSecs;
                g_totalBackgroundWorkPerThread[i] += iter;
            }
            for (auto& iter : g_workerWorkDone[i])
            {
                iter /= testRunTimeSecs;
                g_totalWorkerWorkPerThread[i] += iter;
            }
        }

        g_totalWorkerWork = 0;
        g_totalBackgroundWork = 0;
        for (uint32_t i = 0; i < c_numThreads; i++)
        {
            g_totalWorkerWork += g_totalWorkerWorkPerThread[i];
            g_totalBackgroundWork += g_totalBackgroundWorkPerThread[i];
        }
        sample->SetMutexTestResults(mutexType, contentionLevel, c_numThreads, threadsLocked, g_totalWorkerWork, g_totalBackgroundWork, g_totalWorkerWorkPerThread, g_totalBackgroundWorkPerThread);
    }

    void FutexTestThread()
    {
        ATG::Futex<50, 0, true> futexObject;
        ATG::Slowtex<50, 0, true> slowtexObject;
        ATG::Nulltex<0> nulltexObject;

        for (uint32_t i = 0;; i++)
        {
            for (auto& iter : backgroundThreadWorkTimes)
            {
                uint32_t testRunTimeSecs = i == 0 ? c_firstTestRuntimeSecs : c_laterTestRuntimeSecs;
                auto backgroundMutexTimes = backgroundThreadMutexHoldTimes.find(iter.first);
                auto workerWorkTimes = workerThreadWorkTimes.find(iter.first);
                auto workerMutexTimes = workerThreadMutexHoldTimes.find(iter.first);
                PerformOneTestRun(true, &futexObject, iter.second, backgroundMutexTimes->second, workerWorkTimes->second, workerMutexTimes->second, testRunTimeSecs);
                CalculateMetrics(g_sample, Sample::MutexType::e_futex, iter.first, true, testRunTimeSecs);
                if (!g_runTests)
                    return;

                PerformOneTestRun(false, &futexObject, iter.second, backgroundMutexTimes->second, workerWorkTimes->second, workerMutexTimes->second, testRunTimeSecs);
                CalculateMetrics(g_sample, Sample::MutexType::e_futex, iter.first, false, testRunTimeSecs);
                if (!g_runTests)
                    return;

                PerformOneTestRun(true, &slowtexObject, iter.second, backgroundMutexTimes->second, workerWorkTimes->second, workerMutexTimes->second, testRunTimeSecs);
                CalculateMetrics(g_sample, Sample::MutexType::e_slowtex, iter.first, true, testRunTimeSecs);
                if (!g_runTests)
                    return;

                PerformOneTestRun(false, &slowtexObject, iter.second, backgroundMutexTimes->second, workerWorkTimes->second, workerMutexTimes->second, testRunTimeSecs);
                CalculateMetrics(g_sample, Sample::MutexType::e_slowtex, iter.first, false, testRunTimeSecs);
                if (!g_runTests)
                    return;

                PerformOneTestRun(true, &nulltexObject, iter.second, backgroundMutexTimes->second, workerWorkTimes->second, workerMutexTimes->second, testRunTimeSecs);
                CalculateMetrics(g_sample, Sample::MutexType::e_nulltex, iter.first, true, testRunTimeSecs);
                if (!g_runTests)
                    return;

                PerformOneTestRun(false, &nulltexObject, iter.second, backgroundMutexTimes->second, workerWorkTimes->second, workerMutexTimes->second, testRunTimeSecs);
                CalculateMetrics(g_sample, Sample::MutexType::e_nulltex, iter.first, false, testRunTimeSecs);
                if (!g_runTests)
                    return;
            }
        }
    }

    void StopFutexTest()
    {
        if (g_testThread)
        {
            g_runTests = false;
            g_testThread->join();
            delete g_testThread;
            g_testThread = nullptr;
        }
    }

    void StartFutexTest(Sample* sample)
    {
        if (g_testThread)
            return;

        ATG::SetupProcessorData();
        g_sample = sample;
        g_runTests = true;

        g_testThread = new std::thread(FutexTestThread);
    }
}
