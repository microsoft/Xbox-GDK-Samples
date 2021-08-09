//--------------------------------------------------------------------------------------
// RDTSCPStopWatch.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "RDTSCPStopWatch.h"

#include "intrin.h"

namespace ATG
{
    double s_rdtscpFrequencySecs = CalcRDTSCPFrequency();	// force init at startup
    double s_rdtscpFrequencyMS;								// CalcRDTSCPFrequency auto sets these
    double s_rdtscpFrequencyUS;								// CalcRDTSCPFrequency auto sets these

    double CalcRDTSCPFrequency()
    {
        const uint32_t msDuration = 1000;

        LARGE_INTEGER qpcTempRate;
        QueryPerformanceFrequency(&qpcTempRate);
        const double qpcRate = (double)qpcTempRate.QuadPart;
        double rdtscFrequencyIdle;
        double rdtscFrequencyLoad;
        uint32_t tempAux;

        int32_t currentPriority = GetThreadPriority(GetCurrentThread());
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        uint64_t oldAffinityMask = SetThreadAffinityMask(GetCurrentThread(), 0x04);		// don't use the first core, don't really care if this fails the code still works, this is just here as an extra forcing value just in case
        // Measure __rdtscp() when the machine is mostly idle
        {
            uint64_t rdtscStart = __rdtscp(&tempAux);
            QueryPerformanceCounter(&qpcTempRate);
            int64_t qpcStart = qpcTempRate.QuadPart;
            Sleep(msDuration);
            uint64_t rdtscElapsed = __rdtscp(&tempAux) - rdtscStart;
            QueryPerformanceCounter(&qpcTempRate);
            int64_t qpcElapsed = qpcTempRate.QuadPart - qpcStart;
            rdtscFrequencyIdle = rdtscElapsed / (qpcElapsed / qpcRate);
        }

        // Measure __rdtscp() when the machine is busy
        {
            uint64_t rdtscStart = __rdtscp(&tempAux);
            QueryPerformanceCounter(&qpcTempRate);
            int64_t qpcStart = qpcTempRate.QuadPart;
            uint32_t startTick = GetTickCount();
            for (;;)
            {
                uint32_t tickDuration = GetTickCount() - startTick;
                if (tickDuration >= msDuration)
                    break;
            }
            uint64_t rdtscElapsed = __rdtscp(&tempAux) - rdtscStart;
            QueryPerformanceCounter(&qpcTempRate);
            int64_t qpcElapsed = qpcTempRate.QuadPart - qpcStart;
            rdtscFrequencyLoad = rdtscElapsed / (qpcElapsed / qpcRate);
        }
        if (oldAffinityMask != 0)
            SetThreadAffinityMask(GetCurrentThread(), oldAffinityMask);

        SetThreadPriority(GetCurrentThread(), currentPriority);
        s_rdtscpFrequencySecs = (rdtscFrequencyIdle + rdtscFrequencyLoad) / 2.0;		// the numbers should be close to identical so just average
        s_rdtscpFrequencyMS = s_rdtscpFrequencySecs / 1000;
        s_rdtscpFrequencyUS = s_rdtscpFrequencySecs / 1000000;

        return s_rdtscpFrequencySecs;
    }
}
