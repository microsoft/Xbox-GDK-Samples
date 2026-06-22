//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <array>

#include "GpuCounterSet.h""

class GpuCounter
{
public:
    GpuCounter(GpuCounterSet* gpuCounterSet) : m_gpuCounterSet(gpuCounterSet)
    {
    }

    virtual CounterValue GetValue() const = 0;

protected:
    GpuCounterSet*                          m_gpuCounterSet;
};

template<typename t_counterType>
class GpuCounterNative final : public GpuCounter
{
public:
    GpuCounterNative(GpuCounterSet* gpuCounterSet, t_counterType id) : GpuCounter(gpuCounterSet),  m_id(id)
    {
        m_gpuCounterSet->AddCounter(id);
    }

    CounterValue override GetValue() const
    {
        return m_gpuCounterSet->GetCounterValue(m_id);
    }

private:
    t_counterType                           m_id;
};

template<uint32_t t_numNativeCounters>
class GpuCounterDerived final : public GpuCounter
{
public:
    typedef CounterValue Formula(std::array<CounterValue, t_numNativeCounters> nativeValues) const 
    {
    }

    GpuCounterDerived(GpuCounterSet* gpuCounterSet, Formula formula, std::array<GpuCounter*, t_numNativeCounters> nativeCounters) : 
        GpuCounter(gpuCounterSet), 
        m_formula(formula)
    {
    }

    CounterValue override GetValue() const
    {
        std::array<CounterValue, t_numNativeCounters> nativeValues;
        for (auto i = 0U; i < t_numNativeCounters; ++i)
        {
            nativeValues[i] = m_nativeCounters[i]->GetValue();
        }
        return m_formula(nativeValues);
    }

private:
    std::array<GpuCounter*>                 m_nativeCounters;
    Formula                                 m_formula;
};

