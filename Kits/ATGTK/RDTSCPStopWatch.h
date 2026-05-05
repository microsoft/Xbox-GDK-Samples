//--------------------------------------------------------------------------------------
// RDTSCPStopWatch.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <cstdint>

#include <intrin.h>

namespace ATG
{
    double CalcRDTSCPFrequency();
    extern double s_rdtscpFrequencySecs;
    extern double s_rdtscpFrequencyMS;
    extern double s_rdtscpFrequencyUS;
    extern double s_rdtscpFrequencyNS;

    class RDTSCPStopWatch
    {
    private:
        uint64_t m_startTimeRaw;
        uint64_t m_stopTimeRaw;
        bool m_running;

    public:
        __forceinline static uint64_t GetCPUTimeStamp()
        {
#ifndef _M_ARM64
            uint32_t tempAux;
            return __rdtscp(&tempAux);
#else
            __isb(_ARM64_BARRIER_SY);
            return static_cast<uint64_t> (_ReadStatusReg(ARM64_SYSREG(3, 3, 14, 0, 2)));    //ARM64_CNTVCT_EL0
#endif
        }

        RDTSCPStopWatch() :m_startTimeRaw(0), m_stopTimeRaw(0), m_running(false) {}

        bool IsRunning() const noexcept { return m_running; }
        void Start()
        {
            assert(!m_running);
            m_running = true;
            m_stopTimeRaw = 0;
            m_startTimeRaw = GetCPUTimeStamp();
        }
        double Stop()
        {
            assert(m_running);
            m_stopTimeRaw = GetCPUTimeStamp();
            m_running = false;
            return GetTotalSeconds();
        }

        void Reset()
        {
            m_startTimeRaw = GetCPUTimeStamp();
            m_stopTimeRaw = 0;
        }

        static double GetFrequencyMilliseconds() { return s_rdtscpFrequencyMS; }
        static double GetFrequencyMicroseconds() { return s_rdtscpFrequencyUS; }
        static double GetFrequencyNanoseconds() { return s_rdtscpFrequencyNS; }

        double GetCurrentSeconds() const
        {
            assert(m_running);
            uint64_t temp;
            temp = GetCPUTimeStamp();
            return static_cast<double>(temp - m_startTimeRaw) / s_rdtscpFrequencySecs;
        }
        double GetCurrentMilliseconds() const
        {
            assert(m_running);
            uint64_t temp;
            temp = GetCPUTimeStamp();
            return static_cast<double>(temp - m_startTimeRaw) / s_rdtscpFrequencyMS;
        }
        double GetCurrentMicroseconds() const
        {
            assert(m_running);
            uint64_t temp;
            temp = GetCPUTimeStamp();
            return static_cast<double>(temp - m_startTimeRaw) / s_rdtscpFrequencyUS;
        }
        uint64_t GetCurrentRaw() const
        {
            assert(m_running);
            uint64_t temp;
            temp = GetCPUTimeStamp();
            return temp - m_startTimeRaw;
        }

        double GetTotalSeconds() const
        {
            assert(!m_running);
            return static_cast<double>(m_stopTimeRaw - m_startTimeRaw) / s_rdtscpFrequencySecs;
        }
        double GetTotalMilliseconds() const
        {
            assert(!m_running);
            return static_cast<double>(m_stopTimeRaw - m_startTimeRaw) / s_rdtscpFrequencyMS;
        }
        double GetTotalMicroseconds() const
        {
            assert(!m_running);
            return static_cast<double>(m_stopTimeRaw - m_startTimeRaw) / s_rdtscpFrequencyUS;
        }

        uint64_t GetTotalRaw() const
        {
            assert(!m_running);
            return m_stopTimeRaw - m_startTimeRaw;
        }
    };
}
