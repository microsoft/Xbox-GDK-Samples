//--------------------------------------------------------------------------------------
// Utility.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DirectXMath.h"

namespace
{
    template<typename T>
    const T& Saturate(const T& a, const T& min, const T& max)
    {
        return std::max(std::min(a, max), min);
    }

    float Saturate(float a, float min = 0.0f, float max = 1.0f)
    {
        return Saturate<float>(a, min, max);
    }

    // Description of a repeating frame rate spike
    enum class SpikeShape
    {
        Square,
        Sin
    };

    template< SpikeShape shape >
    struct Spike
    {
        float operator()(float t)
        {
            switch (shape)
            {
            case SpikeShape::Sin:
                return sinf(DirectX::XM_PI * Saturate(fmodf(t, m_period) / m_thickness)) * m_amplitude + m_mean;
            case SpikeShape::Square:
            default:
                return (fmodf(t, m_period) < m_thickness) ? (m_amplitude + m_mean) : m_mean;
            }
        }

        float m_mean;
        float m_amplitude;
        float m_thickness;
        float m_period;
    };

    enum class SpikeProcessor : uint32_t
    {
        None,
        Cpu,
        Gpu,

        Count
    };

    static const WCHAR* c_spikeProcessorNames[] =
    {
        L"None",
        L"CPU",
        L"GPU",
    };
    static_assert(_countof(c_spikeProcessorNames) == static_cast<uint32_t>(SpikeProcessor::Count), "Mismatch between enum and reflected names");


    // Convenience class for a ring buffer (assumes you only add using push_back or push_front)
    using std::deque;

    template< typename T, uint32_t ringSize >
    class Ring : public std::deque< T >
    {
    public:
        void push_back(const T& element)
        {
            if (deque<T>::size() == ringSize)
            {
                deque<T>::pop_front();
            }
            deque<T>::push_back(element);
        }

        void push_front(const T& element)
        {
            if (deque<T>::size() == ringSize)
            {
                deque<T>::pop_back();
            }
            deque<T>::push_front(element);
        }
    };

    // Frame information for the dynamic resolution algorithm
    struct FrameData
    {
        uint32_t presentationThreshold;
        uint32_t width;
        uint32_t height;
    };

    struct FrameStatistics
    {
        D3D12XBOX_RENDER_STATISTICS render;
        D3D12XBOX_PRESENT_STATISTICS present;
        D3D12XBOX_DISPLAY_STATISTICS display;
        FrameData data;
    };
}
