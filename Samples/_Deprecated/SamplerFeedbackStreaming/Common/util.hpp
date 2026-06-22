//--------------------------------------------------------------------------------------
// util.hpp
//
// Utility code for dev test common code.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <windows.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if( (x) != NULL ) { (x)->Release(); (x) = NULL; } }
#endif

#if defined(_DEBUG) || defined(DBG)
void AssertImplementation(bool cond, const CHAR* strFormat, ...);
#define ASSERT(x) AssertImplementation((x), #x "\n")
#define ASSERTMSG AssertImplementation
#else
#define ASSERT(x) __noop
#define ASSERTMSG __noop
#endif
#undef assert
#define assert ASSERT

class FrameTimer
{
protected:
    LARGE_INTEGER m_PerfFreq;
    LARGE_INTEGER m_StartTime;
    LARGE_INTEGER m_LastFrameTime;

    DOUBLE m_DeltaTime;
    DOUBLE m_AbsoluteTime;

    LARGE_INTEGER m_AccumTime;
    UINT m_AccumFrames;
    DOUBLE m_SmoothedDeltaTime;
    BOOL m_SmoothedDeltaTimeUpdated;

public:
    FrameTimer()
    {
        QueryPerformanceFrequency( &m_PerfFreq );
        Reset();
    }

    VOID Reset()
    {
        QueryPerformanceCounter( &m_StartTime );
        m_LastFrameTime = m_StartTime;
        m_AccumTime = m_StartTime;
        m_AccumFrames = 0;
        m_SmoothedDeltaTime = 0;
        m_SmoothedDeltaTimeUpdated = FALSE;
    }

    VOID MarkFrame()
    {
        LARGE_INTEGER CurrentTime;
        QueryPerformanceCounter( &CurrentTime );

        INT64 DeltaTicks = CurrentTime.QuadPart - m_LastFrameTime.QuadPart;
        INT64 AbsoluteTicks = CurrentTime.QuadPart - m_StartTime.QuadPart;

        m_DeltaTime = (DOUBLE)DeltaTicks / (DOUBLE)m_PerfFreq.QuadPart;
        m_AbsoluteTime = (DOUBLE)AbsoluteTicks / (DOUBLE)m_PerfFreq.QuadPart;

        m_LastFrameTime = CurrentTime;

        ++m_AccumFrames;
        m_SmoothedDeltaTimeUpdated = FALSE;
        if( ( CurrentTime.QuadPart - m_AccumTime.QuadPart ) >= m_PerfFreq.QuadPart )
        {
            INT64 AccumTicks = CurrentTime.QuadPart - m_AccumTime.QuadPart;
            m_SmoothedDeltaTime = (DOUBLE)AccumTicks / (DOUBLE)( m_PerfFreq.QuadPart * (INT64)m_AccumFrames );
            m_SmoothedDeltaTimeUpdated = TRUE;

            m_AccumTime = CurrentTime;
            m_AccumFrames = 0;
        }
    }

    DOUBLE GetDeltaTime() const { return m_DeltaTime; }
    DOUBLE GetAbsoluteTime() const { return m_AbsoluteTime; }

    DOUBLE GetAverageDeltaTime() const { return m_SmoothedDeltaTime; }
    BOOL IsAverageDeltaTimeUpdated() const { return m_SmoothedDeltaTimeUpdated; }

    UINT64 GetPerfFreq() const { return m_PerfFreq.QuadPart; }
};

VOID DebugSpew( const CHAR* strFormat, ... );

template<typename T>
inline T NextMultiple( T Value, T Multiple )
{
    return Multiple * ( ( Value + Multiple - 1 ) / Multiple );
}

template<typename T>
inline T PrevMultiple( T Value, T Multiple )
{
    return Multiple * ( Value / Multiple );
}

HRESULT LoadFile( const WCHAR* strFileName, void** ppBuffer, UINT32* pBufferSizeBytes );
void UnloadFile( void* pBuffer );

