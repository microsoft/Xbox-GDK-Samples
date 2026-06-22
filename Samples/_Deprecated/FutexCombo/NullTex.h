//--------------------------------------------------------------------------------------
// Nulltex.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <chrono>
#include <RDTSCPStopWatch.h>

namespace ATG
{
    //////////////////////////////////////////////////////////////////////////
    ///
    /// Nulltex
    ///   Dummy locking class that just Sleeps when a lock is attempted
    ///   Used for comparing various lock implementations against a baseline
    ///   If template parameter sleepTimeMS == UINT32_MAX then Sleep is not called
    ///   Interfaces follow the C++11 TimeLockable concept
    ///
    //////////////////////////////////////////////////////////////////////////
    template <uint32_t sleepTimeMS = UINT32_MAX>
    class Nulltex
    {
    public:
        Nulltex(const Nulltex&) = delete;
        Nulltex& operator=(const Nulltex&) = delete;

        Nulltex() = default;
        Nulltex(Nulltex&& rhs) = default;
        ~Nulltex() { }

        void changeSpinTime(uint32_t /*newSpinTime*/) {}

        void lock()
        {
            try_lock();
        }

        bool try_lock()
        {
            if constexpr (sleepTimeMS != UINT32_MAX)
                Sleep(sleepTimeMS);
            return true;
        }

        void unlock()
        {
        }

        double getWastedTimeUS(bool /*reset*/ = false)
        {
            return false;
        }

        template<class _Rep, class _Period>
        bool try_lock_for(const std::chrono::duration<_Rep, _Period>& /*relTime*/)
        {
            return try_lock();
        }

        template<class _Clock, class _Duration>
        bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& /*absTime*/)
        {
            return try_lock();
        }
    };
}
