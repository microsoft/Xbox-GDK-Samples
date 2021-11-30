//--------------------------------------------------------------------------------------
// File: PerformanceTimers.h
//
// Helpers for doing CPU & GPU performance timing and statitics
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once


namespace DX
{
    //----------------------------------------------------------------------------------
    // CPU performance timer
    class CPUTimer
    {
    public:
        static constexpr size_t c_maxTimers = 8;

        CPUTimer();

        CPUTimer(const CPUTimer&) = delete;
        CPUTimer& operator=(const CPUTimer&) = delete;

        CPUTimer(CPUTimer&&) = default;
        CPUTimer& operator=(CPUTimer&&) = default;

        // Start/stop a particular performance timer (don't start same index more than once in a single frame)
        void Start(uint32_t timerid = 0);
        void Stop(uint32_t timerid = 0);

        // Should Update once per frame to compute timer results
        void Update();

        // Reset running average
        void Reset();

        // Returns delta time in milliseconds
        double GetElapsedMS(uint32_t timerid = 0) const;

        // Returns running average in milliseconds
        float GetAverageMS(uint32_t timerid = 0) const
        {
            return (timerid < c_maxTimers) ? m_avg[timerid] : 0.f;
        }

    private:
        double          m_qpfFreqInv;
        LARGE_INTEGER   m_start[c_maxTimers];
        LARGE_INTEGER   m_end[c_maxTimers];
        float           m_avg[c_maxTimers];
    };


#if defined(__d3d12_h__)
    //----------------------------------------------------------------------------------
    // DirectX 12 implementation of GPU timer
    class GPUTimer
    {
    public:
        static constexpr size_t c_maxTimers = 8;

        GPUTimer() :
            m_gpuFreqInv(1.f),
            m_avg{},
            m_used{},
            m_timing{}
        {}

        GPUTimer(ID3D12Device* device, ID3D12CommandQueue* commandQueue) :
            m_gpuFreqInv(1.f),
            m_avg{},
            m_used{},
            m_timing{}
        {
            RestoreDevice(device, commandQueue);
        }

        GPUTimer(const GPUTimer&) = delete;
        GPUTimer& operator=(const GPUTimer&) = delete;

        GPUTimer(GPUTimer&&) = default;
        GPUTimer& operator=(GPUTimer&&) = default;

        ~GPUTimer() { ReleaseDevice(); }

        // Indicate beginning & end of frame
        void BeginFrame(_In_ ID3D12GraphicsCommandList* commandList);
        void EndFrame(_In_ ID3D12GraphicsCommandList* commandList);

        // Allows GetElapsedMS to retrieve the most recently submitted timings, instead of the oldest ones
        // Requires the caller to block until the data is available
        void Flush(_In_ ID3D12GraphicsCommandList* context);

        // Start/stop a particular performance timer (don't start same index more than once in a single frame)
        void Start(_In_ ID3D12GraphicsCommandList* commandList, uint32_t timerid = 0);
        void Stop(_In_ ID3D12GraphicsCommandList* commandList, uint32_t timerid = 0);

        // Reset running average
        void Reset();

        // Returns delta time in milliseconds
        double GetElapsedMS(uint32_t timerid = 0) const;

        // Returns running average in milliseconds
        float GetAverageMS(uint32_t timerid = 0) const
        {
            return (timerid < c_maxTimers) ? m_avg[timerid] : 0.f;
        }

        // Device management
        void ReleaseDevice();
        void RestoreDevice(_In_ ID3D12Device* device, _In_ ID3D12CommandQueue* commandQueue);

    private:
        static constexpr size_t c_timerSlots = c_maxTimers * 2;

        Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_heap;
        Microsoft::WRL::ComPtr<ID3D12Resource>  m_buffer;
        double                                  m_gpuFreqInv;
        float                                   m_avg[c_maxTimers];
        bool                                    m_used[c_maxTimers];
        UINT64                                  m_timing[c_timerSlots];
    };

#elif defined(__d3d11_h__)
    //----------------------------------------------------------------------------------
    // DirectX 11 implementation of GPU timer
    class GPUTimer
    {
    public:
    public:
        static constexpr size_t c_maxTimers = 8;

        GPUTimer() :
            m_currentFrame(0),
            m_frame{}
        {}

        GPUTimer(ID3D11Device* device) :
            m_currentFrame(0),
            m_frame{}
        {
            RestoreDevice(device);
        }

        GPUTimer(const GPUTimer&) = delete;
        GPUTimer& operator=(const GPUTimer&) = delete;

        GPUTimer(GPUTimer&&) = default;
        GPUTimer& operator=(GPUTimer&&) = default;

        ~GPUTimer() { ReleaseDevice(); }

        // Indicate beginning & end of frame
        void BeginFrame(_In_ ID3D11DeviceContext* context);

        void EndFrame(_In_ ID3D11DeviceContext* context);

        // Allows GetElapsedMS to retrieve the most recently submitted timings, instead of the oldest ones
        // Requires the caller to block until the data is available
        void Flush(_In_ ID3D11DeviceContext* context);

        // Start/stop a particular performance timer (don't start same index more than once in a single frame)
        void Start(_In_ ID3D11DeviceContext* context, uint32_t timerid = 0);
        void Stop(_In_ ID3D11DeviceContext* context, uint32_t timerid = 0);

        // Reset running average
        void Reset();

        // Returns delta time in milliseconds
        double GetElapsedMS(uint32_t timerid = 0) const;

        // Returns running average in milliseconds
        float GetAverageMS(uint32_t timerid = 0) const;

        // Device management
        void ReleaseDevice();
        void RestoreDevice(_In_ ID3D11Device* device);

    private:
        static constexpr size_t c_bufferCount = 3;

        unsigned m_currentFrame;
        struct Frame
        {
            bool                                m_pending;
            Microsoft::WRL::ComPtr<ID3D11Query> m_disjoint;
            Microsoft::WRL::ComPtr<ID3D11Query> m_start[c_maxTimers];
            Microsoft::WRL::ComPtr<ID3D11Query> m_end[c_maxTimers];
            float                               m_avg[c_maxTimers];
            float                               m_timing[c_maxTimers];
            bool                                m_used[c_maxTimers];

            Frame() :
                m_pending(false),
                m_avg{},
                m_timing{},
                m_used{}
            {}

            void BeginFrame(_In_ ID3D11DeviceContext* context);
            void EndFrame(_In_ ID3D11DeviceContext* context);
            void ComputeFrame(_In_ ID3D11DeviceContext* context);

            void Start(_In_ ID3D11DeviceContext* context, uint32_t timerid = 0);
            void Stop(_In_ ID3D11DeviceContext* context, uint32_t timerid = 0);

        } m_frame[c_bufferCount];
    };

#else
#error Must include d3d11*.h or d3d12*.h before PerformanceTimers.h
#endif
}
