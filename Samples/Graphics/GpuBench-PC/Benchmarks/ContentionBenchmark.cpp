//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

auto g_dummySum = 0ULL;

class ContentionBenchmark final : public Benchmark
{
public:
    ContentionBenchmark() = default;

    ~ContentionBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Contention";
    }

    void Initialize(ID3D12Device* device) override
    {
        auto esram = IsDurangoClass();

        auto allSizeBytes = IsDurangoClass() ? ContentionTest::m_allSizeBytesDurango : ContentionTest::m_allSizeBytesScorpio;

        for (auto memoryParamsSrc : ContentionTest::m_allMemoryParams)
        {
            // There should be no contention for ESRAM, but we may as well confirm
            if(!esram && memoryParamsSrc.m_type == MEMORY_TYPE_ESRAM)
            {
                continue;
            }
            for (auto memoryParamsDst : ContentionTest::m_allMemoryParams)
            {
                if(!esram && memoryParamsDst.m_type == MEMORY_TYPE_ESRAM)
                {
                    continue;
                }
                for (auto sizeBytes : allSizeBytes)
                {
                    for (auto contentionParams : ContentionTest::m_allContentionParams)
                    {
                        AddTest(new ContentionCopyTest(memoryParamsSrc, memoryParamsDst, sizeBytes, contentionParams));
                    }
                }
            }
        }

        // MC_ARB has 4 slots

        // DRAM
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH1);

        // Subtract off display bandwidth
        AddCounter(GPUPerfCounters::MC_CITF_PERF_MCC_MCB_READ_RETURN_EOP_MATCHING_CID);

        // Onion
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_READ_SEND_REQ_ONION_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_READ_SEND_REQ_ONION_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_ONION_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_ONION_WRITE_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_ONION_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_ONION_WRITE_CH1);

        // ESRAM
        AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH1);

        auto computeShaderBlob = DX::ReadData(L"MemoryCs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            computeShaderBlob.data(),
            computeShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(ContentionTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(ContentionTest::m_rootSignature);

        auto ThreadProc = [] (LPVOID lpParameter)->DWORD
        { 
            auto& data = *reinterpret_cast<const ContentionTest::ThreadData*>(lpParameter);

            while (true)
            {
                // Don't wait at all if the signal hasn't arrived --- we need to keep hitting memory
                auto startResult = WaitForSingleObject(data.m_start, INFINITE);
                if (WAIT_OBJECT_0 != startResult)
                {
                    throw std::exception("Expected thread to receive signal");
                }

                auto startedResult = SetEvent(data.m_started);
                if (!startedResult)
                {
                    throw std::exception("Expected event to get signaled");
                }
                {
                    std::wostringstream message;
                    message << L"Started CPU thread on core ";
                    message << data.m_core;
                    message << std::endl;
                    OutputDebugString(message.str().c_str());
                }

                bool ended = false;
                while (!ended)
                {
                    // Don't wait at all if the signal hasn't arrived --- we need to keep hitting memory.
                    auto endResult = WaitForSingleObject(data.m_end, 0U);
                    if (!SUCCEEDED(endResult))
                    {
                        throw std::exception("Expected thread to receive signal");
                    }

                    ended = WAIT_OBJECT_0 == endResult;

                    static auto iterations = 10U;

                    for (auto iteration = 0U; iteration < iterations; ++iteration)
                    {
                        switch (data.m_op)
                        {
                        case ContentionTest::CONTENTION_OP_READ:
                        {
                            for (auto i = 0U; i < ContentionTest::m_sizeBytes / sizeof(uint64_t); ++i)
                            {
                                g_dummySum += static_cast<const uint64_t*>(data.m_src)[i];
                            }
                        }
                        break;
                        case ContentionTest::CONTENTION_OP_WRITE:
                            memset(data.m_dst, 0, ContentionTest::m_sizeBytes);
                            break;
                        case ContentionTest::CONTENTION_OP_READWRITE:
                            memcpy(data.m_dst, data.m_src, ContentionTest::m_sizeBytes);
                            break;
                        default:
                            assert(false);
                            break;
                        }
                    }
                }

                auto endedResult = SetEvent(data.m_ended);
                if (!endedResult)
                {
                    throw std::exception("Expected event to get signaled");
                }
                {
                    std::wostringstream message;
                    message << L"Ended CPU thread on core ";
                    message << data.m_core;
                    message << std::endl;
                    OutputDebugString(message.str().c_str());
                }
            }

            return S_OK;
        };
        wchar_t threadName[256] = L"";
        wchar_t startName[256] = L"";
        wchar_t startedName[256] = L"";
        wchar_t endName[256] = L"";
        wchar_t endedName[256] = L"";

        for (auto i = 0U; i < _countof(ContentionTest::m_threadData); ++i)
        {
            auto& data = ContentionTest::m_threadData[i];

            data.m_core = i;

            if (0 == i)
            {
                // Keep the 0th core free for the sample
                data.m_src = nullptr;
                data.m_dst = nullptr;
                data.m_handle = INVALID_HANDLE_VALUE;
                data.m_start = INVALID_HANDLE_VALUE;
                data.m_started = INVALID_HANDLE_VALUE;
                data.m_end = INVALID_HANDLE_VALUE;
                data.m_ended = INVALID_HANDLE_VALUE;
                data.m_op = ContentionTest::CONTENTION_OP_COUNT;    // initialize to invalid value
            }
            else
            {
                auto allocationType = DWORD(MEM_RESERVE | MEM_COMMIT);
                auto xMemAllocationFlags = DWORD(XMEM_CPU);
                auto pageProtectSrc = DWORD(PAGE_READONLY);
                auto pageProtectDst = DWORD(PAGE_READWRITE | PAGE_WRITECOMBINE);
                data.m_src = XMemVirtualAlloc(nullptr, ContentionTest::m_sizeBytes, allocationType, xMemAllocationFlags, pageProtectSrc);
                if (nullptr == data.m_src)
                {
                    throw std::exception("Could not allocate dummy src buffer");
                }
                data.m_dst = XMemVirtualAlloc(nullptr, ContentionTest::m_sizeBytes, allocationType, xMemAllocationFlags, pageProtectDst);
                if (nullptr == data.m_dst)
                {
                    throw std::exception("Could not allocate dummy dst buffer");
                }

                data.m_handle = CreateThread(nullptr, 0, ThreadProc, &data, 0, nullptr);
                _snwprintf_s(threadName, _countof(threadName), _TRUNCATE, L"Thread %d", i);
                if (0 == SetThreadDescription(data.m_handle, threadName))
                {
                    throw std::exception("Could not set thread name");
                }
                if (0 == SetThreadAffinityMask(data.m_handle, 1ULL << i))   // Run on its own core
                {
                    throw std::exception("Could not set thread affinity");
                }
                if (0 == SetThreadPriority(data.m_handle, THREAD_PRIORITY_NORMAL))
                {
                    throw std::exception("Could not set thread priority");
                }

                _snwprintf_s(startName, _countof(startName), _TRUNCATE, L"Start %d", i);
                data.m_start = CreateEvent(nullptr, FALSE, FALSE, startName);
                _snwprintf_s(startedName, _countof(startedName), _TRUNCATE, L"Started %d", i);
                data.m_started = CreateEvent(nullptr, FALSE, FALSE, startedName);
                _snwprintf_s(endName, _countof(endName), _TRUNCATE, L"End %d", i);
                data.m_end = CreateEvent(nullptr, FALSE, FALSE, endName);
                _snwprintf_s(endedName, _countof(endedName), _TRUNCATE, L"Ended %d", i);
                data.m_ended = CreateEvent(nullptr, FALSE, FALSE, endedName);

                data.m_op = ContentionTest::CONTENTION_OP_COUNT;    // initialize to invalid value
            }
        }

        ContentionTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        ContentionTest::m_rootSignature.Reset();

        auto success = true;
        for (auto i = 1U; i < _countof(ContentionTest::m_threadData); ++i)
        {
            auto& data = ContentionTest::m_threadData[i];

            success = success && CloseHandle(data.m_handle);
            data.m_handle = INVALID_HANDLE_VALUE;

            if (data.m_src)
                success = success && VirtualFree(data.m_src, 0, MEM_RELEASE);
            data.m_src = nullptr;
            if (data.m_dst)
                success = success && VirtualFree(data.m_dst, 0, MEM_RELEASE);
            data.m_dst = nullptr;

            success = success && CloseHandle(data.m_start);
            data.m_start = INVALID_HANDLE_VALUE;
            success = success && CloseHandle(data.m_started);
            data.m_started = INVALID_HANDLE_VALUE;
            success = success && CloseHandle(data.m_end);
            data.m_end = INVALID_HANDLE_VALUE;
            success = success && CloseHandle(data.m_ended);
            data.m_ended = INVALID_HANDLE_VALUE;
        }

        if (!success)
        {
            throw std::exception("Failed to close a HANDLE or free memory");
        }
    }

private:
    class ContentionTest : public Test
    {
    public:
#ifdef BASE_2_BANDWIDTH
        static constexpr uint64_t KB = 1024UL;
        static constexpr uint64_t MB = 1024UL * 1024UL;
        static constexpr uint64_t GB = 1024UL * 1024UL * 1024UL;
#else
        static constexpr uint64_t KB = 1000UL;
        static constexpr uint64_t MB = 1000UL * 1000UL;
        static constexpr uint64_t GB = 1000UL * 1000UL * 1000UL;
#endif

        struct MemoryParams
        {
            MemoryType                                  m_type;
            const wchar_t*                              m_typeName;
        };

        static const std::vector<MemoryParams> m_allMemoryParams;
        static const std::vector<uint64_t> m_allSizeBytesDurango;
        static const std::vector<uint64_t> m_allSizeBytesScorpio;

        enum ContentionOp
        {
            CONTENTION_OP_READ, 
            CONTENTION_OP_WRITE, 
            CONTENTION_OP_READWRITE, 

            CONTENTION_OP_COUNT
        };

        struct ContentionParams
        {
            uint32_t                                    m_coreMask;

            ContentionOp                                m_op;
            const wchar_t*                              m_opName;
        };

        static const std::vector<ContentionParams> m_allContentionParams;

        ContentionTest(const ContentionParams& contentionParams) :
            m_resourceAllocator(),
            m_contentionParams(contentionParams)
        {
        }

        void Start(ID3D12GraphicsCommandList* /*commandList*/) const override
        {
            for (auto i = 0U; i < _countof(ContentionTest::m_threadData); ++i)
            { 
                auto coreBit = 1U << i;
                bool coreEnabled = coreBit & m_contentionParams.m_coreMask;

                if (coreEnabled)
                {
                    auto& data = m_threadData[i];

                    data.m_op = m_contentionParams.m_op;

                    auto startResult = SetEvent(data.m_start);
                    if (!startResult)
                    {
                        throw std::exception("Expected event to get signaled");
                    }

                    std::wostringstream message;
                    message << L"Starting CPU thread on core ";
                    message << data.m_core;
                    message << std::endl;
                    OutputDebugString(message.str().c_str());

                    auto startedResult = WaitForSingleObject(data.m_started, INFINITE);
                    if (WAIT_OBJECT_0 != startedResult)
                    {
                        throw std::exception("Expected thread to receive signal");
                    }
                }
            }
        }

        void Stop(ID3D12GraphicsCommandList* /*commandList*/) const override
        {
            for (auto i = 0U; i < _countof(ContentionTest::m_threadData); ++i)
            {
                auto coreBit = 1U << i;
                bool coreEnabled = coreBit & m_contentionParams.m_coreMask;

                if (coreEnabled)
                {
                    auto& data = m_threadData[i];

                    auto endResult = SetEvent(data.m_end);
                    if (!endResult)
                    {
                        throw std::exception("Expected event to get signaled");
                    }

                    std::wostringstream message;
                    message << L"Ending CPU thread on core ";
                    message << data.m_core;
                    message << std::endl;
                    OutputDebugString(message.str().c_str());

                    auto endedResult = WaitForSingleObject(data.m_ended, INFINITE);
                    if (WAIT_OBJECT_0 != endedResult)
                    {
                        throw std::exception("Expected thread to receive signal");
                    }

                    data.m_op = ContentionTest::CONTENTION_OP_COUNT;    // set to invalid value
                }
            }
        }

        virtual ~ContentionTest()
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Test", L"", 16);
            report->AddColumn(L"Cores", L"", 8);
            report->AddColumn(L"CPU Op", L"", 8);
            report->AddColumn(L"Src", L"", 8);
            report->AddColumn(L"Dst", L"", 8);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Bytes (API)", L"", 12);
            report->AddColumn(L"Bytes (GPU)", L"", 12);
#ifdef BASE_2_BANDWIDTH
            report->AddColumn(L"Bandwidth", L" GiB/s", 6, 2);
#else
            report->AddColumn(L"Bandwidth", L" GB/s", 6, 2);
#endif
            report->AddColumn(L"% of \"max\"", L"%", 6, 2);

            report->AddHeader();
        }

    protected:
        float GetGBPerSecReadIdeal(MemoryType type) const
        {
            const auto& gpuProperties = GpuProperties::Get();
            auto gpuFrequency = g_gpuHardwareConfiguration.GpuFrequency;
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
                return float(gpuProperties.m_bytesPerMemTransGddr * gpuProperties.m_memTransPerSecondGddr) / GB;
            case MEMORY_TYPE_ONION:
                return float(gpuProperties.m_bytesPerNClkOnionRead * gpuProperties.m_nClkPerSecondNorthbridge) / GB;
            case MEMORY_TYPE_ESRAM:
                return float(gpuProperties.m_bytesPerSClkEsram * gpuFrequency) / GB;
            default:
                return 0.0f;
            }
        }

        float GetGBPerSecWrittenIdeal(MemoryType type) const 
        {
            const auto& gpuProperties = GpuProperties::Get();
            auto gpuFrequency = g_gpuHardwareConfiguration.GpuFrequency;
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
                return float(gpuProperties.m_bytesPerMemTransGddr * gpuProperties.m_memTransPerSecondGddr) / GB;
            case MEMORY_TYPE_ONION:
                return float(gpuProperties.m_bytesPerNClkOnionWrite * gpuProperties.m_nClkPerSecondNorthbridge) / GB;
            case MEMORY_TYPE_ESRAM:
                return float(gpuProperties.m_bytesPerSClkEsram * gpuFrequency) / GB;
            default:
                return 0.0f;
            }
        }

        uint64_t GetBytesRead(MemoryType type) const
        {
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
            {
                // These are total DRAM bytes, so we are assuming no Onion traffic
                auto reads32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH1);

                // Better to include display traffic, so we can see if it's affecting us
                //auto displayTraffic = GetCounterValue(GPUPerfCounters::MC_CITF_PERF_MCC_MCB_READ_RETURN_EOP_MATCHING_CID);

                return 32 * reads32;
            }

            case MEMORY_TYPE_ONION:
            {
                auto reads64 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_READ_SEND_REQ_ONION_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_READ_SEND_REQ_ONION_CH1);

                return 64 * reads64;
            }

            case MEMORY_TYPE_ESRAM:
            {
                auto reads32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH1);

                return 32 * reads32;
            }

            default:
                return 0;
            }
        }

        uint64_t GetBytesWritten(MemoryType type) const
        {
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
            {
                auto writes32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH1)
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH1);
                auto writes64 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH1)
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH1);

                return 32 * writes32 + 64 * writes64;
            }

            case MEMORY_TYPE_ONION:
            {
                auto writes32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_ONION_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_ONION_WRITE_CH1);
                auto writes64 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_ONION_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_ONION_WRITE_CH1);

                return 32 * writes32 + 64 * writes64;
            }

            case MEMORY_TYPE_ESRAM:
            {
                auto writes32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH1);

                return 32 * writes32;
            }

            default:
                return 0;
            }
        }

        GpuBenchAllocator                                       m_resourceAllocator;

        ContentionParams                                        m_contentionParams;

    public:
        // These are per thread, not per-test, because we'll run out of memory otherwise
        const static uint32_t                                   m_titleCores = 7U;
        const static uint32_t                                   m_sizeBytes = 4 * MB;
        struct ThreadData
        {
            uint32_t                                            m_core;

            HANDLE                                              m_handle;
            void*                                               m_src;
            void*                                               m_dst;

            HANDLE                                              m_start;
            HANDLE                                              m_started;
            HANDLE                                              m_end;
            HANDLE                                              m_ended;

            ContentionOp                                        m_op;
        };
        static ThreadData                                       m_threadData[m_titleCores];

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>                      m_rootSignature;
    };

    class ContentionCopyTest final : public ContentionTest
    {
    public:
        ContentionCopyTest(const MemoryParams& memoryParamsSrc, const MemoryParams& memoryParamsDst, uint64_t sizeBytes, const ContentionParams& contentionParams) :
            ContentionTest(contentionParams),
            m_memoryParamsSrc(memoryParamsSrc),
            m_memoryParamsDst(memoryParamsDst),
            m_sizeBytes(sizeBytes)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            auto esramSize = 32 * MB;
            if ((MEMORY_TYPE_ESRAM == m_memoryParamsSrc.m_type || MEMORY_TYPE_ESRAM == m_memoryParamsDst.m_type) && m_sizeBytes > esramSize)
            {
                throw std::exception("Can't do CopyResource test on a resource larger than ESRAM");
            }
            if ((MEMORY_TYPE_ESRAM == m_memoryParamsSrc.m_type && MEMORY_TYPE_ESRAM == m_memoryParamsDst.m_type) && m_sizeBytes > esramSize / 2)
            {
                throw std::exception("Can't do CopyResource test between resources larger than 1/2 of ESRAM");
            }

            auto width = 1024U;

            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R32G32B32A32_TYPELESS,
                width,
                UINT((m_sizeBytes / width) / 16),
                1U,
                1U);
            auto addressSrc = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParamsSrc.m_type);
            DX::ThrowIfFailed(device->CreatePlacedResourceX(addressSrc, 
                &desc, 
                D3D12_RESOURCE_STATE_COPY_SOURCE, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(m_src.ReleaseAndGetAddressOf())));

            auto addressDst = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParamsDst.m_type);
            DX::ThrowIfFailed(device->CreatePlacedResourceX(addressDst, 
                &desc, 
                D3D12_RESOURCE_STATE_COPY_DEST, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(m_dst.ReleaseAndGetAddressOf())));
        };

        void Uninitialize() override
        {
            m_resourceAllocator.FreeResourceMemory(m_src->GetGPUVirtualAddress(), m_memoryParamsSrc.m_type);
            m_resourceAllocator.FreeResourceMemory(m_dst->GetGPUVirtualAddress(), m_memoryParamsDst.m_type);

            m_src.Reset();
            m_dst.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"CopyResource ";
            name << L'(' << m_memoryParamsSrc.m_typeName << L"-->" << m_memoryParamsDst.m_typeName << L')';

            if (m_contentionParams.m_coreMask)
            {
                name << L" during CPU " << m_contentionParams.m_opName;
                name << L" on " << __popcnt(m_contentionParams.m_coreMask) << L" cores";
            }

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            std::wostringstream coresStr;
            for (auto coreBit = 1U << 0U; coreBit < 1U << m_titleCores; coreBit <<= 1)
            {
                bool coreEnabled = coreBit & m_contentionParams.m_coreMask;
                coresStr << (coreEnabled ? L'X' : L'_');
            }

            auto timeMs = m_elapsedTime;
            auto bytesExpected = m_sizeBytes;
            auto bytesRead = GetBytesRead(m_memoryParamsSrc.m_type);
            auto bytesWritten = GetBytesWritten(m_memoryParamsDst.m_type);
            auto gbPerSecRead = (bytesRead / float(GB)) / timeMs * 1000.0f;
            auto gbPerSecWritten = (bytesWritten / float(GB)) / timeMs * 1000.0f;
            auto gbPerSec = gbPerSecRead + gbPerSecWritten;
            auto gbPerSecReadIdeal = GetGBPerSecReadIdeal(m_memoryParamsSrc.m_type);
            auto gbPerSecWrittenIdeal = GetGBPerSecWrittenIdeal(m_memoryParamsDst.m_type);
            auto timeMsReadIdeal = bytesRead ? (bytesRead / float(gbPerSecReadIdeal * GB) * 1000.0f) : 0.0f;
            auto timeMsWrittenIdeal = bytesWritten ? (bytesWritten / float(gbPerSecWrittenIdeal * GB) * 1000.0f) : 0.0f;

            // Garlic is really bound by memory speed, rather than bus speed, and memory has shared read/write bandwidth.
            // Every other case has separate read/write pathways, however it's not always possible to fully exercise both.
            // For example, the GPU can only fully exercise ESRAM if it uses large transactions, and that's not under software control.
            auto parallelBandwidth = !((MEMORY_TYPE_GARLIC == m_memoryParamsSrc.m_type) && (MEMORY_TYPE_GARLIC == m_memoryParamsDst.m_type));
            auto timeMsIdeal = parallelBandwidth ? std::max(timeMsReadIdeal, timeMsWrittenIdeal) : (timeMsReadIdeal + timeMsWrittenIdeal);
            auto gbPerSecIdeal = (bytesRead + bytesWritten) / float(GB) / timeMsIdeal * 1000.0f;

            report->AddRowData(L"CopyResource");
            report->AddRowData(coresStr.str());
            report->AddRowData(m_contentionParams.m_opName);
            report->AddRowData(m_memoryParamsSrc.m_typeName);
            report->AddRowData(m_memoryParamsDst.m_typeName);
            report->AddRowData(timeMs);
            report->AddRowData(2 * bytesExpected);
            report->AddRowData(bytesRead + bytesWritten);
            report->AddRowData(gbPerSecRead + gbPerSecWritten);
            report->AddRowData(100.0f * gbPerSec / gbPerSecIdeal);

            DirectX::XMVECTOR color;
            switch (m_memoryParamsDst.m_type)
            {
            case MEMORY_TYPE_GARLIC:
                color = DirectX::Colors::Tan;
                break;
            case MEMORY_TYPE_ONION:
                color = DirectX::Colors::White;
                break;
            case MEMORY_TYPE_ESRAM:
            default:
                color = DirectX::Colors::Silver;
                break;
            }
            report->EndRow(color);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->CopyResource(m_dst.Get(), m_src.Get());

            // Include the full bandwidth by flushing caches
            commandList->FlushPipelineX(D3D12XBOX_FLUSH_BOP_MASK | D3D12XBOX_FLUSH_TOP_MASK, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
        }

    private:
        ComPtr<ID3D12Resource>          m_src;
        ComPtr<ID3D12Resource>          m_dst;

        MemoryParams                    m_memoryParamsSrc;
        MemoryParams                    m_memoryParamsDst;

        uint64_t                        m_sizeBytes;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>                     ContentionBenchmark::ContentionTest::m_rootSignature;
ContentionBenchmark::ContentionTest::ThreadData ContentionBenchmark::ContentionTest::m_threadData[ContentionBenchmark::ContentionTest::m_titleCores];

const std::vector<ContentionBenchmark::ContentionTest::MemoryParams> ContentionBenchmark::ContentionTest::m_allMemoryParams =
{
    { MEMORY_TYPE_GARLIC,  L"Garlic",  },
    { MEMORY_TYPE_ONION,   L"Onion",   },
    { MEMORY_TYPE_ESRAM,   L"ESRAM",   },
};

const std::vector<uint64_t> ContentionBenchmark::ContentionTest::m_allSizeBytesDurango = 
{
    16 * MB, 
};

const std::vector<uint64_t> ContentionBenchmark::ContentionTest::m_allSizeBytesScorpio = 
{
    64 * MB, 
};

const std::vector<ContentionBenchmark::ContentionTest::ContentionParams> ContentionBenchmark::ContentionTest::m_allContentionParams =
{
    // The main sample thread is running on core 0, so avoid that one if possible
    {0b00000000, ContentionBenchmark::ContentionTest::CONTENTION_OP_READ,       L"Read", }, 
    {0b00000000, ContentionBenchmark::ContentionTest::CONTENTION_OP_WRITE,      L"Write", }, 
    {0b00000000, ContentionBenchmark::ContentionTest::CONTENTION_OP_READWRITE,  L"RW", }, 

    {0b00000010, ContentionBenchmark::ContentionTest::CONTENTION_OP_READ,       L"Read", }, 
    {0b00000010, ContentionBenchmark::ContentionTest::CONTENTION_OP_WRITE,      L"Write", }, 
    {0b00000010, ContentionBenchmark::ContentionTest::CONTENTION_OP_READWRITE,  L"RW", }, 

    {0b00000110, ContentionBenchmark::ContentionTest::CONTENTION_OP_READWRITE,  L"RW", }, 

    {0b00001110, ContentionBenchmark::ContentionTest::CONTENTION_OP_READWRITE,  L"RW", }, 

    {0b01111110, ContentionBenchmark::ContentionTest::CONTENTION_OP_READWRITE,  L"RW", }, 
};

ContentionBenchmark benchmark;
