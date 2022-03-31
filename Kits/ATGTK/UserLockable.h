//--------------------------------------------------------------------------------------
// UserLockable.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <chrono>

namespace ATG
{
    //////////////////////////////////////////////////////////////////////////
    ///
    /// Event object that attempts to stay at user level as much as possible
    ///   The object will only go to kernel mode if there is actually a thread waiting on the event
    ///   This allows for the maximum performance.
    ///   Interfaces follow the C++11 TimeLockable concept
    ///   The template parameter on manual reset is used to allow the compiler to optimize out the conditional
    ///
    //////////////////////////////////////////////////////////////////////////
    template <bool manualReset = false>
    class UserEventLockable
    {
    private:
        std::atomic<uint32_t> m_threadsWaiting;         // use a flag to say how many threads are actually checking/waiting on event, used to control number of calls to Wake to lower cost
        uint32_t m_eventFlag;                           // the current event flag, interlocks are used on it for thread safety, one means signaled

    public:
        /// The UserSemaphoreLockable cannot be copied
        UserEventLockable(const UserEventLockable&) = delete;
        UserEventLockable& operator=(const UserEventLockable&) = delete;

        UserEventLockable(bool initialState = true) : m_threadsWaiting(0), m_eventFlag(static_cast<uint32_t> (initialState ? 1 : 0)) {  }
        UserEventLockable(UserEventLockable&& rhs) = default;
        ~UserEventLockable() { }

        void SetEvent() { unlock(); }
        void ResetEvent() { InterlockedBitTestAndReset(reinterpret_cast<LONG*> (&m_eventFlag), 0); }

        void lock()			// waits until the event is signaled
        {
            if (try_lock()) // try a quick test first, fast path
                return;

            m_threadsWaiting++;     // need to avoid a race condition on the try_lock and the WaitOnAddress where it could be changed and we don't get notified.
                                    // Another thread could unlock the semaphore before the WaitOnAddress call, this means unlock will not call WakeByAddress
                                    // and this thread will not get notified to wake up. The threads waiting count controls whether to call WakeByAddress
            while (!try_lock())     // there could be a spurious wakeup from race conditions
            {
                uint32_t temp(0);                                            // even if the wake comes before this call it is still safe
                WaitOnAddress(&m_eventFlag, &temp, sizeof(m_eventFlag), INFINITE);  // the underlying WaitOnAddress has protection from that race condition
            }
            m_threadsWaiting--;
        }

        bool try_lock()
        {
            if (manualReset)    // conditional is optimized out due to template parameter so the conditional is always the same
            {
                if (m_eventFlag)
                    return true;
            }
            else
            {
                if (InterlockedBitTestAndReset(reinterpret_cast<LONG*> (&m_eventFlag), 0))
                    return true;
            }
            return false;
        }

        void unlock()		// is the same as SetEvent
        {
            InterlockedBitTestAndSet(reinterpret_cast<LONG*> (&m_eventFlag), 0);
            if (m_threadsWaiting)           // only call WakeByAddress if there is a thread actually waiting.
            {
                if (manualReset)            // conditional is optimized out due to template parameter so the conditional is always the same
                {
                    WakeByAddressAll(&m_eventFlag);
                }
                else
                {
                    WakeByAddressSingle(&m_eventFlag);
                }
            }
        }

        template<class _Rep, class _Period>
        bool try_lock_for(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            if (try_lock())
                return true;

            while (!try_lock())     // there could be a spurious wakeup from race conditions
            {
                int64_t msWait = std::chrono::duration_cast<std::chrono::nanoseconds>(relTime).count(); // convert to nanoseconds so we can force the round up to milliseconds
                if (msWait < 0)
                    msWait = 0;
                msWait = static_cast<uint64_t> (std::ceil(static_cast<double> (msWait) / 1000000.0));

                m_threadsWaiting++;     // need to avoid a race condition on the try_lock and the WaitOnAddress where it could be changed and we don't get notified.
                                        // Another thread could unlock the semaphore before the WaitOnAddress call, this means unlock will not call WakeByAddress
                                        // and this thread will not get notified to wake up. The threads waiting count controls whether to call WakeByAddress
                uint32_t temp(0);                                            // even if the wake comes before this call it is still safe
                if (WaitOnAddress(&m_eventFlag, &temp, sizeof(m_eventFlag), static_cast<DWORD> (msWait)) == WAIT_TIMEOUT)  // the underlying WaitOnAddress has protection from that race condition
                {
                    m_threadsWaiting--;
                    return false;
                }
                m_threadsWaiting--;
            }
            return true;
        }

        template<class _Clock, class _Duration>
        bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& absTime)
        {
            return try_lock_for(absTime - std::chrono::steady_clock::now());
        }
    };

    //////////////////////////////////////////////////////////////////////////
    ///
    /// Semaphore object that attempts to stay at user level as much as possible
    ///   The object will only go to kernel mode if there is actually a thread waiting on the semaphore
    ///   This allows for the maximum performance.
    ///   Interfaces follow the C++11 TimeLockable concept
    ///
    //////////////////////////////////////////////////////////////////////////
    class UserSemaphoreLockable
    {
    private:
        std::atomic<uint32_t> m_threadsWaiting;         // use a flag to say how many threads are actually checking/waiting on event, used to control number of calls to Wake to lower cost
        uint32_t m_semaphore;                           // the current count on the semaphore, interlocks are used on it for thread safety

    public:

        /// The UserSemaphoreLockable cannot be copied
        UserSemaphoreLockable(const UserSemaphoreLockable&) = delete;
        UserSemaphoreLockable& operator=(const UserSemaphoreLockable&) = delete;
        UserSemaphoreLockable(UserSemaphoreLockable&& rhs) = delete;
        UserSemaphoreLockable& operator=(const UserSemaphoreLockable&&) = delete;

        UserSemaphoreLockable(uint32_t intialValue) : m_threadsWaiting(0), m_semaphore(intialValue) {  }
        ~UserSemaphoreLockable() { }

        void lock()
        {
            if (try_lock()) // try a quick test first, fast path
                return;

            m_threadsWaiting++;     // need to avoid a race condition on the try_lock and the WaitOnAddress where it could be changed and we don't get notified.
                                    // Another thread could unlock the semaphore before the WaitOnAddress call, this means unlock will not call WakeByAddress
                                    // and this thread will not get notified to wake up. The threads waiting count controls whether to call WakeByAddress
            while (!try_lock())     // there could be a spurious wakeup from race conditions
            {
                uint32_t temp(0);                                            // even if the wake comes before this call it is still safe
                WaitOnAddress(&m_semaphore, &temp, sizeof(m_semaphore), INFINITE);  // the underlying WaitOnAddress has protection from that race condition
            }
            m_threadsWaiting--;
        }

        bool try_lock()
        {
            // need to loop until the interlock CAS operation is successful
            for (;;)
            {
                const uint32_t currentValue = *((volatile uint32_t*)&m_semaphore);  // this needs to be forced copy, don't want the compiler to optimize it away
                if (currentValue == 0)                                              // another thread count have grabbed the event
                    return false;
                const uint32_t newValue = currentValue - 1;                         // locking only reduces the count by 1
                if (InterlockedCompareExchange(&m_semaphore, newValue, currentValue) == currentValue)
                    return true;
            }
        }

        void unlock(uint32_t releaseCount = 1) // releasing a semaphore can be for any count
        {
            // need to loop until the interlock CAS operation is successful
            for (;;)
            {
                const uint32_t currentValue = *((volatile uint32_t*)&m_semaphore);  // this needs to be forced copy, don't want the compiler to optimize it away
                const uint32_t newValue = currentValue + releaseCount;
                if (InterlockedCompareExchange(&m_semaphore, newValue, currentValue) == currentValue)
                    break;
            }

            if (m_threadsWaiting)           // only call WakeByAddress if there is a high probability of a thread actually waiting.
            {
                // need to wake either all waiting threads or just the release count of them.
                if (m_threadsWaiting < releaseCount)
                {
                    WakeByAddressAll(&m_semaphore);
                }
                else
                {
                    while (releaseCount)
                    {
                        WakeByAddressSingle(&m_semaphore);
                        releaseCount--;
                    }
                }
            }
        }

        template<class _Rep, class _Period>
        bool try_lock_for(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            if (try_lock())
                return true;

            while (!try_lock())     // there could be a spurious wakeup from race conditions
            {
                int64_t msWait = std::chrono::duration_cast<std::chrono::nanoseconds>(relTime).count(); // convert to nanoseconds so we can force the round up to milliseconds
                if (msWait < 0)
                    msWait = 0;
                msWait = static_cast<uint64_t> (std::ceil(static_cast<double> (msWait) / 1000000.0));

                m_threadsWaiting++;     // need to avoid a race condition on the try_lock and the WaitOnAddress where it could be changed and we don't get notified.
                                        // Another thread could unlock the semaphore before the WaitOnAddress call, this means unlock will not call WakeByAddress
                                        // and this thread will not get notified to wake up. The threads waiting count controls whether to call WakeByAddress
                uint32_t temp(0);                                            // even if the wake comes before this call it is still safe
                if (WaitOnAddress(&m_semaphore, &temp, sizeof(m_semaphore), static_cast<DWORD> (msWait)) == WAIT_TIMEOUT)  // the underlying WaitOnAddress has protection from that race condition
                {
                    m_threadsWaiting--;
                    return false;
                }
                m_threadsWaiting--;
            }
            return true;
        }

        template<class _Clock, class _Duration>
        bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& absTime)
        {
            return try_lock_for(absTime - std::chrono::steady_clock::now());
        }
    };

    //////////////////////////////////////////////////////////////////////////
    ///
    /// Event object that attempts to stay at user level as much as possible
    ///   The object will only go to kernel mode if there is actually a thread waiting on the event
    ///   This allows for the maximum performance.
    ///   Interfaces follow the C++11 TimeLockable concept
    ///   The template parameter on manual reset is used to allow the compiler to optimize out the conditional
    ///
    //////////////////////////////////////////////////////////////////////////
#if !defined(__clang__) || defined (__MWAITX__)     //NOTE: Clang explictly only includes this intrinsic if __MWAITX__ is defined and it needs to be defined in the pch file before the x86intrin.h is included
#pragma warning (push)
#pragma warning (disable: 4324)
    class alignas(64) MonitorXMwaitXLockable
    {
    private:
        uint32_t m_eventFlag;                           // the current event flag, interlocks are used on it for thread safety
        uint32_t m_owningThread;
        char unusedForceSizeBlock[56];                  // force size to be 64 bytes

    public:
        /// The UserSemaphoreLockable cannot be copied
        MonitorXMwaitXLockable(const MonitorXMwaitXLockable&) = delete;
        MonitorXMwaitXLockable& operator=(const MonitorXMwaitXLockable&) = delete;

        MonitorXMwaitXLockable(bool initialState = false) : m_eventFlag(static_cast<uint32_t> (initialState ? 1 : 0)), m_owningThread(0), unusedForceSizeBlock{ } {  }
        MonitorXMwaitXLockable(MonitorXMwaitXLockable&& rhs) = default;
        ~MonitorXMwaitXLockable() { }

        void lock()
        {
            if (try_lock()) // try a quick test first, fast path
            {
                assert(m_eventFlag == 1);
                assert(m_owningThread == 0);
                m_owningThread = GetCurrentThreadId();
                return;
            }

            while (InterlockedCompareExchange(reinterpret_cast<LONG*> (&m_eventFlag), 1, 0))
            {
                _mm_monitorx(&m_eventFlag, 0, 0);
                if (InterlockedCompareExchange(reinterpret_cast<LONG*> (&m_eventFlag), 1, 0))
                    _mm_mwaitx(0x02, 0, 0xffffff);
                else
                    break;
            }
            assert(m_owningThread == 0);
            m_owningThread = GetCurrentThreadId();
            assert(m_eventFlag == 1);
        }

        bool try_lock()
        {
            return InterlockedCompareExchange(reinterpret_cast<LONG*> (&m_eventFlag), 1, 0) == 0;
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
            if (try_lock())
                return true;

            while (!try_lock())     // there could be a spurious wakeup from race conditions
            {
                int64_t msWait = std::chrono::duration_cast<std::chrono::nanoseconds>(relTime).count(); // convert to nanoseconds so we can force the round up to milliseconds
                if (msWait < 0)
                    msWait = 0;

                msWait = static_cast<uint64_t> (std::ceil(static_cast<double> (msWait) / 1000000.0));

                while (!InterlockedBitTestAndReset(reinterpret_cast<LONG*> (&m_eventFlag), 0))
                {
                    _mm_monitorx(&m_eventFlag, 0, 0);
                    if (!InterlockedBitTestAndReset(reinterpret_cast<LONG*> (&m_eventFlag), 0))
                        _mm_mwaitx(0x02, 0, 0xffffffff);
                }
            }
            return true;
        }

        template<class _Clock, class _Duration>
        bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& absTime)
        {
            return try_lock_for(absTime - std::chrono::steady_clock::now());
        }
    };
#pragma warning (pop)
#endif  // !__clang__ || __MWAITX__

    //////////////////////////////////////////////////////////////////////////
    ///
    /// Event object that attempts to stay at user level as much as possible
    ///   The object will only go to kernel mode if there is actually a thread waiting on the event
    ///   This allows for the maximum performance.
    ///   Interfaces follow the C++11 TimeLockable concept
    ///   The template parameter on manual reset is used to allow the compiler to optimize out the conditional
    ///
    //////////////////////////////////////////////////////////////////////////
    template<bool threadOwner>
    class UserBarrier
    {
    private:
        std::atomic<uint64_t> m_barrierValue;
        uint64_t m_initialValue;
        UserEventLockable<true> m_blockEvent;
        uint32_t m_threadOwnerID;

    public:
        /// The UserBarrier cannot be copied
        UserBarrier(const UserBarrier&) = delete;
        UserBarrier& operator=(const UserBarrier&) = delete;

        UserBarrier(uint64_t initialValue = 1) : m_barrierValue(initialValue), m_initialValue(initialValue), m_blockEvent(false), m_threadOwnerID(0) { }
        UserBarrier(UserBarrier&& rhs) = default;
        ~UserBarrier() { }

        void clearOwner() { m_threadOwnerID = 0; }

        bool enterBarrier(bool block = false)
        {
            assert(m_barrierValue != 0);
            if (threadOwner)
            {
                if (m_threadOwnerID == 0)
                    m_threadOwnerID = GetCurrentThreadId();
            }
            uint64_t myValue = m_barrierValue.fetch_sub(1);
            if (myValue == 1)
            {
                m_blockEvent.SetEvent();
                return true;
            }
            else if (block)
            {
                m_blockEvent.lock();
            }
            else
            {
                while (m_barrierValue.load() != 0);
            }
            return false;
        }

        bool reset() { return reset(m_initialValue); }
        bool reset(uint64_t newValue)
        {
            if (threadOwner)
            {
                if (m_threadOwnerID != GetCurrentThreadId())
                {
                    if (InterlockedCompareExchange(&m_threadOwnerID, GetCurrentThreadId(), 0) != 0)
                        return false;
                }
            }
            m_initialValue = newValue;
            m_barrierValue = newValue;
            m_blockEvent.SetEvent();
            m_blockEvent.ResetEvent();
            return true;
        }
    };
}
