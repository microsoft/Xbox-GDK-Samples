//--------------------------------------------------------------------------------------
// UserFutex.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#if __cplusplus < 202002L
#error Requires C++20 and /Zc:__cplusplus
#endif

#include <chrono>
#include <RDTSCPStopWatch.h>
#include <type_traits>

namespace ATG
{
    //////////////////////////////////////////////////////////////////////////
    ///
    /// Futex object that attempts to stay at user level as much as possible
    ///   Spin lock implemented using the monitorx/mwaitx instructions
    ///   After spinning for a set amount of time it will suspend the thread
    ///   Interfaces follow the C++11 TimeLockable concept
    ///
    //////////////////////////////////////////////////////////////////////////

    extern __declspec(selectany) __declspec(thread) ATG::RDTSCPStopWatch g_futexWastedTimeTimer;

    template <uint32_t initialSpinTimeUS = 50, uint32_t alignment = 0, bool profile = false>
    class Futex
    {
    private:
        alignas(alignment) uint64_t m_eventFlag;                           // the current event flag, interlocks are used on it for thread safety
        uint64_t spinTimeUS;
        uint64_t spinTimeInTicks;
        DWORD m_owningThread;
        std::atomic<double> m_wastedTimeUS;
        std::atomic<uint32_t> m_waiters;

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
                m_wastedTimeUS.fetch_add(g_futexWastedTimeTimer.GetTotalMicroseconds());
            }
        }

        bool PerformOneSpin(uint32_t timeOutMS)
        {
            if (try_lock())
                return true;

            uint64_t curTime, stopTime;
            uint32_t tempAux;
            curTime = __rdtscp(&tempAux);
            stopTime = curTime + spinTimeInTicks;

            static uint64_t s_lockedFlag(1);
            while (curTime < stopTime)
            {
#ifdef _GAMING_XBOX_SCARLETT
                _mm_monitorx(&m_eventFlag, 0, 0);
                if (InterlockedCompareExchange(reinterpret_cast<LONG*> (&m_eventFlag), 1, 0))
                    _mm_mwaitx(0x02, 0, (uint32_t)spinTimeInTicks);
                else
                {
                    m_owningThread = GetCurrentThreadId();
                    return true;
                }
#else
                _mm_pause();
#endif
                if (try_lock())
                {
                    return true;
                }
                curTime = __rdtscp(&tempAux);
            }

            start_profile();
            m_waiters++;
            WaitOnAddress(&m_eventFlag, &s_lockedFlag, sizeof(m_eventFlag), timeOutMS);
            m_waiters--;
            stop_profile();
            if (InterlockedCompareExchange(reinterpret_cast<LONG*> (&m_eventFlag), 1, 0) == 0)
            {
                m_owningThread = GetCurrentThreadId();
                return true;
            }
            return false;
        }

    public:
        /// The Futex cannot be copied
        Futex(const Futex&) = delete;
        Futex& operator=(const Futex&) = delete;

        Futex(bool initialState = false) :
            m_eventFlag(initialState ? 1ull : 0)
            , spinTimeUS(initialSpinTimeUS)
            , spinTimeInTicks((uint32_t)(ATG::RDTSCPStopWatch::GetFrequencyMicroseconds()* initialSpinTimeUS))
            , m_owningThread(0)
            , m_wastedTimeUS(0)
        {  }
        Futex(Futex&& rhs) = default;
        ~Futex() { }

        void lock()
        {
            assert(m_owningThread != GetCurrentThreadId());
            if (try_lock()) // try a quick test first, fast path
            {
                assert(m_eventFlag == 1);
                assert(m_owningThread == GetCurrentThreadId());
                return;
            }

            while (!PerformOneSpin(INFINITE));
            assert(m_owningThread == GetCurrentThreadId());
            assert(m_eventFlag == 1);
        }

        void changeSpinTime(uint32_t newSpinTime)
        {
            spinTimeUS = newSpinTime;
            spinTimeInTicks = (uint32_t)(ATG::RDTSCPStopWatch::GetFrequencyMicroseconds() * spinTimeUS);
        }

        double getWastedTimeUS(bool reset = false)
        {
            double toret = m_wastedTimeUS;
            if (reset)
                m_wastedTimeUS = 0;
            return toret;
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
            if (m_waiters)
                WakeByAddressSingle(&m_eventFlag);
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

            int64_t msWait = std::chrono::duration_cast<std::chrono::nanoseconds>(relTime).count(); // convert to nanoseconds so we can force the round up to milliseconds
            if (msWait < 0)
                msWait = 0;

            msWait = static_cast<uint64_t> (std::ceil(static_cast<double> (msWait) / 1000000.0));
            auto startTime = std::chrono::steady_clock::now();
            while (true)
            {
                if (PerformOneSpin(msWait))
                {
                    assert(m_owningThread == 0);
                    m_owningThread = GetCurrentThreadId();
                    assert(m_eventFlag == 1);
                    return true;
                }
                else
                {
                    auto runningTime = std::chrono::steady_clock::now() - startTime;
                    if (runningTime > relTime)
                        return false;

                    msWait = std::chrono::duration_cast<std::chrono::nanoseconds>(runningTime).count(); // convert to nanoseconds so we can force the round up to milliseconds
                    if (msWait < 0)
                        msWait = 0;

                    msWait = static_cast<uint64_t> (std::ceil(static_cast<double> (msWait) / 1000000.0));
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
