//--------------------------------------------------------------------------------------
// StopwatchProfiler.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "RDTSCPStopWatch.h"

namespace ATG
{

template<unsigned int numLabels, unsigned int numDuplicates = 1>
class StopwatchProfiler
{
public:

    struct TimingAccumulator
    {
        TimingAccumulator()
            : m_curTotal(0.0)
            , m_lowestVal(999999.999999)
            , m_highestVal(0.0)
            , m_curElements(0)
        {
            InitializeCriticalSection(&m_critSection);
        }

        ~TimingAccumulator()
        {
            DeleteCriticalSection(&m_critSection);
        }

        void PushValue(double value)
        {
            // This is scope locked to ensure proper data. The timing of this function should not matter as it's only important that
            // the number being passed to this function was as accurate as possible.
            EnterCriticalSection(&m_critSection);
            ++m_curElements;
            m_curTotal += value;
            m_lowestVal = value < m_lowestVal ? value : m_lowestVal;
            m_highestVal = value > m_highestVal ? value : m_highestVal;
            LeaveCriticalSection(&m_critSection);
        }

        double GetAverage(void) const
        {
            return m_curTotal / static_cast<double>(m_curElements);
        }

        double              m_curTotal;
        double              m_lowestVal;
        double              m_highestVal;
        unsigned int        m_curElements;

        CRITICAL_SECTION    m_critSection;
    };

public:

    StopwatchProfiler()
    {
    }

    ~StopwatchProfiler()
    {
    }

    RDTSCPStopWatch* GetStopWatch(unsigned int label, unsigned int duplicate = 0)
    {
        assert(label < numLabels);
        assert(duplicate < numDuplicates);
        return &m_stopWatches[label + duplicate * numLabels];
    }
    TimingAccumulator* GetAccumulator(unsigned int label, unsigned int duplicate = 0)
    {
        assert(label < numLabels);
        assert(duplicate < numDuplicates);
        return &m_accumulators[label + duplicate * numLabels];
    }

    FORCEINLINE void Start(unsigned int label, unsigned int duplicate = 0)
    {
        m_stopWatches[label + duplicate * numLabels].Start();
    }
    FORCEINLINE void Stop(unsigned int label, unsigned int duplicate = 0)
    {
        m_stopWatches[label + duplicate * numLabels].Stop();
    }
    FORCEINLINE void Reset(unsigned int label, unsigned int duplicate = 0)
    {
        m_stopWatches[label + duplicate * numLabels].Reset();
    }

    FORCEINLINE void RecordCurrentTiming(unsigned int label, unsigned int duplicate = 0)
    {
        const unsigned int index = label + duplicate * numLabels;
        m_accumulators[index].PushValue(m_stopWatches[index].GetCurrentSeconds());
    }

protected:

    RDTSCPStopWatch     m_stopWatches[numLabels * numDuplicates];
    TimingAccumulator   m_accumulators[numLabels * numDuplicates];
};

}
