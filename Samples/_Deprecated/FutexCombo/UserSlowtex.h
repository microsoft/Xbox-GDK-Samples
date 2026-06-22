//--------------------------------------------------------------------------------------
// UserSlowtex.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <chrono>
#include <RDTSCPStopWatch.h>

namespace ATG
{
    extern __declspec(selectany) __declspec(thread) ATG::RDTSCPStopWatch g_slowtexWastedTimeTimer;
    //////////////////////////////////////////////////////////////////////////
    ///
    /// Slowtex object that attempts to stay at user level as much as possible
    ///   Spin lock implemented using the SwitchToThread
    ///     Note: Internally Sleep(0) and SwitchToThread are the same function. Sleep(0) is short circuited to call SwitchToThread directly
    ///   Interfaces follow the C++11 TimeLockable concept
    ///
    //////////////////////////////////////////////////////////////////////////
    template <uint32_t initialSpinTimeUS = 50, uint32_t alignment = 0, bool profile = false>
    class Slowtex
    {
    private:
        alignas(alignment) uint64_t m_eventFlag;                           // the current event flag, interlocks are used on it for thread safety
        uint32_t spinTimeUS;
        uint32_t spinTimeInTicks;
        DWORD m_owningThread;
        std::atomic<double> m_wastedTimeUS;
        std::atomic<uint32_t> m_waiters;
        char padding[16] = {};

        void start_profile()
        {
            if constexpr (profile)
                g_futexWastedTimeTimer.Start();
        }

        void stop_profile()
        {
            if constexpr (profile)
            {
                g_futexWastedTimeTimer.Stop();
                m_wastedTimeUS += g_futexWastedTimeTimer.GetTotalMicroseconds();
            }
        }

        bool PerformOneSpin()
        {
            if (try_lock()) // try a quick test first, fast path
            {
                assert(m_eventFlag == 1);
                assert(m_owningThread == GetCurrentThreadId());
                return true;
            }

            uint64_t curTime, stopTime;
            uint32_t tempAux;
            curTime = __rdtscp(&tempAux);
            stopTime = curTime + spinTimeInTicks;

            while (curTime < stopTime)
            {
                _mm_pause();
                if (try_lock())
                {
                    assert(m_eventFlag == 1);
                    assert(m_owningThread == GetCurrentThreadId());
                    return true;
                }
                curTime = __rdtscp(&tempAux);
            }
            start_profile();
            SwitchToThread();     // Note: Internally Sleep(0) and SwitchToThread are the same function. Sleep(0) is short circuited to call SwitchToThread directly
            stop_profile();
            return false;
        }

    public:
        /// The Futex cannot be copied
        Slowtex(const Slowtex&) = delete;
        Slowtex& operator=(const Slowtex&) = delete;

        Slowtex(bool initialState = false) : m_eventFlag(initialState ? 1ull : 0), spinTimeUS(initialSpinTimeUS), spinTimeInTicks((uint32_t)(ATG::RDTSCPStopWatch::GetFrequencyMicroseconds()* initialSpinTimeUS)), m_owningThread(0) {  }
        Slowtex(Slowtex&& rhs) = default;
        ~Slowtex() { }

        double getWastedTimeUS(bool reset = false)
        {
            double toret = m_wastedTimeUS;
            if (reset)
                m_wastedTimeUS = 0;
            return toret;
        }

        void changeSpinTime(uint32_t newSpinTime)
        {
            spinTimeUS = newSpinTime;
            spinTimeInTicks = (uint32_t)(ATG::RDTSCPStopWatch::GetFrequencyMicroseconds() * spinTimeUS);
        }

        void lock()
        {
            assert(m_owningThread != GetCurrentThreadId());

            while (!PerformOneSpin());
            assert(m_eventFlag == 1);
            assert(m_owningThread == GetCurrentThreadId());
        }

        bool try_lock()
        {
            assert(m_owningThread != GetCurrentThreadId());
            if (!m_eventFlag)
            {
                if (InterlockedCompareExchange(reinterpret_cast<LONG*> (&m_eventFlag), 1, 0) == 0)
                {
                    assert(m_owningThread == 0);
                    m_owningThread = GetCurrentThreadId();
                    return true;
                }
            }
            return false;
        }

        void unlock()
        {
            assert(m_owningThread == GetCurrentThreadId());
            assert(m_eventFlag == 1);
            m_owningThread = 0;
            InterlockedCompareExchange(reinterpret_cast<LONG*> (&m_eventFlag), 0, 1);
        }

        template<class _Rep, class _Period>
        bool try_lock_for(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            assert(m_owningThread != GetCurrentThreadId());
            if (try_lock())
            {
                assert(m_eventFlag == 1);
                assert(m_owningThread == GetCurrentThreadId());
                return true;
            }

            auto startTime = std::chrono::steady_clock::now();
            while (true)
            {
                if (PerformOneSpin())
                {
                    assert(m_eventFlag == 1);
                    assert(m_owningThread == GetCurrentThreadId());
                    return true;
                }
                else
                {
                    auto runningTime = std::chrono::steady_clock::now() - startTime;
                    if (runningTime > relTime)
                        return false;
                }
            }
            return false;
        }

        template<class _Clock, class _Duration>
        bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& absTime)
        {
            return try_lock_for(absTime - std::chrono::steady_clock::now());
        }
    };
}
