//--------------------------------------------------------------------------------------
// RDTSCPStopWatch.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <cstdint>

#include "intrin.h"

namespace ATG
{
	double CalcRDTSCPFrequency();
	extern double s_rdtscpFrequencySecs;
	extern double s_rdtscpFrequencyMS;
	extern double s_rdtscpFrequencyUS;

	class RDTSCPStopWatch
	{
	private:
		uint64_t m_startTimeRaw;
		uint64_t m_stopTimeRaw;
		bool m_running;

	public:

		RDTSCPStopWatch() :m_startTimeRaw(0), m_stopTimeRaw(0), m_running(false) {}
		void Start()
		{
			assert(!m_running);
			m_running = true;
			m_stopTimeRaw = 0;
			uint32_t tempAux;
			m_startTimeRaw = __rdtscp(&tempAux);
		}
		double Stop()
		{
			assert(m_running);
			uint32_t tempAux;
			m_stopTimeRaw = __rdtscp(&tempAux);
			m_running = false;
			return GetTotalSeconds();
		}

		void Reset()
		{
			m_startTimeRaw = 0;
			m_stopTimeRaw = 0;
		}

		static double GetFrequencyMilliseconds() { return s_rdtscpFrequencyMS; }

		double GetCurrentSeconds() const
		{
			assert(m_running);
			uint32_t tempAux;
			uint64_t temp;
			temp = __rdtscp(&tempAux);
			return static_cast<double>(temp - m_startTimeRaw) / s_rdtscpFrequencySecs;
		}
		double GetCurrentMilliseconds() const
		{
			assert(m_running);
			uint32_t tempAux;
			uint64_t temp;
			temp = __rdtscp(&tempAux);
			return static_cast<double>(temp - m_startTimeRaw) / s_rdtscpFrequencyMS;
		}
		double GetCurrentMicroseconds() const
		{
			assert(m_running);
			uint32_t tempAux;
			uint64_t temp;
			temp = __rdtscp(&tempAux);
			return static_cast<double>(temp - m_startTimeRaw) / s_rdtscpFrequencyUS;
		}
		uint64_t GetCurrentRaw() const
		{
			assert(m_running);
			uint32_t tempAux;
			uint64_t temp;
			temp = __rdtscp(&tempAux);
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
