//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DirectXMath.h"

namespace FramePacingUtils
{
    template<typename T>
    inline T Saturate(T value, T minimum, T maximum)
    {
        return std::clamp(value, minimum, maximum);
    }

    inline float Saturate(float value, float minimum = 0.0f, float maximum = 1.0f)
    {
        return Saturate<float>(value, minimum, maximum);
    }

    enum class FrameRate : uint32_t
    {
        Fps120,
        Fps60,
        Fps40,
        Fps30,

        Count
    };

    inline constexpr const wchar_t* c_frameRateNames[] =
    {
        L"120 fps",
        L"60 fps",
        L"40 fps",
        L"30 fps",
    };
    static_assert(_countof(c_frameRateNames) == static_cast<uint32_t>(FrameRate::Count), "Mismatch between enum and reflected names");

    enum class FrameLoadSequenceType : uint32_t
    {
        GpuSawtooth,
        GpuJitter,
        GpuGlitch,
        GpuSpike,

        Count
    };
    inline constexpr const wchar_t* c_frameLoadSequenceNames[] =
    {
        L"Gpu Sawtooth",
        L"Gpu Jitter",
        L"Gpu Glitch",
        L"Gpu Spike",
    };
    static_assert(_countof(c_frameLoadSequenceNames) == static_cast<uint32_t>(FrameLoadSequenceType::Count), "Mismatch between enum and reflected names");
    inline constexpr const wchar_t* c_frameLoadSequenceFileNames[] =
    {
        L"GpuSawtooth",
        L"GpuJitter",
        L"GpuGlitch",
        L"GpuSpike",
    };
    static_assert(_countof(c_frameLoadSequenceFileNames) == static_cast<uint32_t>(FrameLoadSequenceType::Count), "Mismatch between enum and reflected names");

    // Convenience class for a ring buffer (assumes you only add using push_back or push_front)
    template<typename T, uint32_t ringSize>
    class Ring : public std::deque<T>
    {
    public:
        void push_back(const T& element)
        {
            if (std::deque<T>::size() == ringSize)
            {
                std::deque<T>::pop_front();
            }
            std::deque<T>::push_back(element);
        }

        void push_front(const T& element)
        {
            if (std::deque<T>::size() == ringSize)
            {
                std::deque<T>::pop_back();
            }
            std::deque<T>::push_front(element);
        }
    };

    // Frame statistics tracked to monitor frame rate, latency, etc.
    struct FrameStatistics
    {
        D3D12XBOX_FRAME_INTERVAL_STATISTICS interval;
        D3D12XBOX_FRAME_EVENT_STATISTICS origin;
        D3D12XBOX_RENDER_STATISTICS render;
        D3D12XBOX_PRESENT_STATISTICS present;
        D3D12XBOX_DISPLAY_STATISTICS display;
    };

    // History of frame timestamps
    struct FrameTimestamps
    {
        uint32_t                                    m_frameBuffer;

        // Need an anchor time, to align all the graphs to a single start point
        uint64_t                                    m_flipTime;

        LARGE_INTEGER                               m_startCpuUpdate;
        LARGE_INTEGER                               m_stopCpuUpdate;
        LARGE_INTEGER                               m_startCpuRender;
        LARGE_INTEGER                               m_stopCpuRender;
        uint64_t                                    m_startGpuGraphics;
        uint64_t                                    m_stopGpuGraphics;
        uint64_t                                    m_startGpuCompute;
        uint64_t                                    m_stopGpuCompute;
    };
} // namespace FramePacingUtils
