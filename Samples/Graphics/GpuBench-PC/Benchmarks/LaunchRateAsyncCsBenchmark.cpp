//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"
#include <sstream>

//#define TEST_SIMD_WALK_ALGORITHM
//#define TEST_ORDER_MODE

using Microsoft::WRL::ComPtr;


static std::wstring MakeNameWithPipeAndQueueSuffix(LPCWSTR name, uint32_t pipe, uint32_t queue)
{
    std::wstringstream wss;
    wss << name << L"_pipe_" << pipe << L"_queue_" << queue;
    return wss.str();
}

#define SET_NAME_TO_SELF_WITH_PIPE_AND_QUEUE_SUFFIX(obj, pipe, queue) SetNameWithPipeAndQueueSuffix(obj, L#obj, pipe, queue)
template <typename T>
static void SetNameWithPipeAndQueueSuffix(T& obj, LPCWSTR name, uint32_t pipe, uint32_t queue)
{
    const std::wstring fullname = MakeNameWithPipeAndQueueSuffix(name, pipe, queue);
    obj->SetName(fullname.c_str());
}

#define SET_FENCE_NAME_TO_SELF_WITH_PIPE_AND_QUEUE_SUFFIX(device, addr, pipe, queue) SetFenceNameWithPipeAndQueueSuffix(device, addr, L#addr, pipe, queue)
static void SetFenceNameWithPipeAndQueueSuffix(ID3D12Device* device, D3D12_GPU_VIRTUAL_ADDRESS address, LPCWSTR name, uint32_t pipe, uint32_t queue)
{
    const std::wstring fullname = MakeNameWithPipeAndQueueSuffix(name, pipe, queue);
    device->RegisterCustomFenceLocationX(address, fullname.c_str());
}

template <typename T, typename U = T, size_t Size = 0>
static constexpr U SumArray(T(&arr)[Size])
{
    typename std::remove_const<U>::type sum = 0;
    for (size_t i = 0; i < Size; ++i)
    {
        sum += arr[i];
    }
    return sum;
}


class LaunchRateAsyncCsBenchmark final : public Benchmark
{
public:
    LaunchRateAsyncCsBenchmark() :
        Benchmark(),
        m_fenceAddress(nullptr),
        m_scopedErrorFilter(nullptr)
    {
    }

    ~LaunchRateAsyncCsBenchmark() override
    {
        if (m_fenceAddress)
        {
            VirtualFree(m_fenceAddress, 0ULL, MEM_RELEASE);
        }
    }

    const wchar_t* GetName() const override
    {
        return L"LaunchRate(ACS)";
    }

    void Initialize(ID3D12Device* device) override
    {
        // Values derived from D3D12XBOX_COMMAND_QUEUE_DESC GDK documentation
#ifdef _GAMING_XBOX_SCARLETT
        constexpr uint32_t COMPUTE_PIPE_COUNT = 4;
        constexpr uint32_t COMPUTE_QUEUE_COUNT_PER_PIPE[] = { 6, 3, 4, 4 };
#else
        constexpr uint32_t COMPUTE_PIPE_COUNT = 2;
        constexpr uint32_t COMPUTE_QUEUE_COUNT_PER_PIPE[] = { 6, 3 };
#endif
        constexpr uint32_t COMPUTE_QUEUE_COUNT = SumArray(COMPUTE_QUEUE_COUNT_PER_PIPE);
        static_assert(ARRAYSIZE(COMPUTE_QUEUE_COUNT_PER_PIPE) == COMPUTE_PIPE_COUNT, "The length of COMPUTE_PER_PIPE_QUEUE_COUNT must be the same as COMPUTE_PIPE_COUNT");

        // All possible compute queues arranged as [pipeIndex][queueIndex]
        // Use a static for init since queues can only ever be constructed once on Xbox
        // MSVC 2022 errors without the explicit capture and clang warns with it. Silence clang's warning
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-lambda-capture"
#endif
        static std::vector<std::vector<ComputeQueue>> s_allComputeQueues = [&, COMPUTE_QUEUE_COUNT_PER_PIPE]() {
#ifdef __clang__
#pragma clang diagnostic pop
#endif

            std::vector<std::vector<ComputeQueue>> allComputeQueues;

            // Initialize all possible compute queues   
            for (uint32_t pipeIndex = 0; pipeIndex < COMPUTE_PIPE_COUNT; ++pipeIndex)
            {
                const uint32_t pipeQueueCount = COMPUTE_QUEUE_COUNT_PER_PIPE[pipeIndex];

                std::vector<ComputeQueue> pipeQueues;
                for (uint32_t queueIndex = 0; queueIndex < pipeQueueCount; ++queueIndex)
                {
                    ComputeQueue queue = {};
                    DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
                        IID_GRAPHICS_PPV_ARGS(queue.m_commandAllocatorCompute.ReleaseAndGetAddressOf())));
                    SET_NAME_TO_SELF_WITH_PIPE_AND_QUEUE_SUFFIX(queue.m_commandAllocatorCompute, pipeIndex, queueIndex);

                    DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, queue.m_commandAllocatorCompute.Get(), nullptr,
                        IID_GRAPHICS_PPV_ARGS(queue.m_commandListCompute.ReleaseAndGetAddressOf())));
                    SET_NAME_TO_SELF_WITH_PIPE_AND_QUEUE_SUFFIX(queue.m_commandListCompute, pipeIndex, queueIndex);

                    const D3D12XBOX_COMMAND_QUEUE_DESC desc = { D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_QUEUE_FLAG_NONE, pipeIndex, queueIndex };
                    DX::ThrowIfFailed(device->CreateCommandQueueX(&desc, IID_GRAPHICS_PPV_ARGS(queue.m_commandQueueCompute.ReleaseAndGetAddressOf())));
                    SET_NAME_TO_SELF_WITH_PIPE_AND_QUEUE_SUFFIX(queue.m_commandQueueCompute, pipeIndex, queueIndex);

                    pipeQueues.push_back(std::move(queue));
                }
                allComputeQueues.push_back(std::move(pipeQueues));
            }

            return allComputeQueues;
        }();

        // Allocate GPU memory for manual fences.
        using Fence = uint64_t;
        constexpr uint32_t computeWritebackAlignment = 8;
#pragma warning(push)
#pragma warning(disable:4324)   // structure was padded due to alignment specifier
        struct alignas(computeWritebackAlignment) PaddedFence { Fence fence; };
#pragma warning(pop)
        static_assert(sizeof(PaddedFence) == computeWritebackAlignment, "Not getting expected alignment");
        static_assert(alignof(PaddedFence) == computeWritebackAlignment, "Not getting expected alignment");
        static_assert(alignof(PaddedFence[1]) == computeWritebackAlignment, "Not getting expected alignment");
        const auto allocationSizeInBytes = (2U * COMPUTE_QUEUE_COUNT) * sizeof(PaddedFence);
        m_fenceAddress = XMemVirtualAlloc(nullptr,
            allocationSizeInBytes,
            MEM_RESERVE | MEM_COMMIT | MEM_64K_PAGES,
            XMEM_GRAPHICS,
            PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE);
        if (nullptr == m_fenceAddress)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        ZeroMemory(m_fenceAddress, allocationSizeInBytes);
        auto fenceAddress = reinterpret_cast<Fence*>(m_fenceAddress);

        uint32_t flatIndex = 0;
        for (uint32_t pipeIndex = 0; pipeIndex < COMPUTE_PIPE_COUNT; ++pipeIndex)
        {
            const uint32_t pipeQueueCount = COMPUTE_QUEUE_COUNT_PER_PIPE[pipeIndex];
            for (uint32_t queueIndex = 0; queueIndex < pipeQueueCount; ++queueIndex)
            {
                ComputeQueue& queue = s_allComputeQueues[pipeIndex][queueIndex];

                queue.m_fenceComputeToGfx = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&fenceAddress[0U * COMPUTE_QUEUE_COUNT + flatIndex]);
                SET_FENCE_NAME_TO_SELF_WITH_PIPE_AND_QUEUE_SUFFIX(device, queue.m_fenceComputeToGfx, pipeIndex, queueIndex);
                assert(queue.m_fenceComputeToGfx % computeWritebackAlignment == 0U);
                queue.m_fenceValueComputeToGfx = 0ULL;

                queue.m_fenceGfxToCompute = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&fenceAddress[1U * COMPUTE_QUEUE_COUNT + flatIndex]);
                SET_FENCE_NAME_TO_SELF_WITH_PIPE_AND_QUEUE_SUFFIX(device, queue.m_fenceGfxToCompute, pipeIndex, queueIndex);
                assert(queue.m_fenceGfxToCompute % computeWritebackAlignment == 0U);
                queue.m_fenceValueGfxToCompute = 0ULL;

                ++flatIndex;
            }
        }

#ifdef _GAMING_XBOX_SCARLETT
        for (auto waveSize : { 64U, 32U, })
#else
        for (auto waveSize : { 64U, })
#endif
        {
            for (auto wavePerTG : { 1U, 2U, 4U, })
            {
                if (4U == wavePerTG && 64U == waveSize)
                {
                    continue;
                }

                // for (auto numThreadgroupsWalkedPerCu : { 1U, 2U, 3U, 4U, 5U, 6U, 7U, })
                for (auto numThreadgroupsWalkedPerCu : { 1U, 2U, 4U, })
                {
                    for (auto simdWalkAlgorithm : { D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM_DEFAULT,
#ifdef TEST_SIMD_WALK_ALGORITHM
                    D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM_GLOBAL_BALANCED,
                    D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM_GLOBAL_STRICT,
                    D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM_PER_CU,
#endif
                        })
                    {
#ifdef TEST_ORDER_MODE
                        for (auto orderMode : { false, true })
#else
                        auto orderMode = false;
#endif
                        {
                            for (auto populatedVgprs : { 1U, 2U, 3U, })
                            {
                                AddTest(new LaunchRateAsyncCsTest(
                                    waveSize,
                                    wavePerTG,
                                    numThreadgroupsWalkedPerCu,
                                    simdWalkAlgorithm,
                                    populatedVgprs,
                                    orderMode,
                                    // Can add more/different queues here to test
                                    // queuing work on multiple queues at once
                                    // Format is [pipeIdx][queueIdx]
                                    {
                                        &s_allComputeQueues[0][0],
                                    }));
                            }
                        }
                    }
                }
            }
        }

        // Validation layer warns that manual fences may lead to incorrect barrier
        // validation errors. Turn off the debug break so we run clean in debug
#ifdef _GAMING_XBOX_SCARLETT
        constexpr uint32_t barrierValidationErrorId = 0x153FFF2A;
#else
        constexpr uint32_t barrierValidationErrorId = 0x4A07E690;
#endif
        m_scopedErrorFilter.reset(
            new ScopedErrorFilter(
                device,
                barrierValidationErrorId,
                D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_BREAKS));

        ComputeTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        auto success = VirtualFree(m_fenceAddress, 0ULL, MEM_RELEASE);
        if (!success)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        m_fenceAddress = nullptr;

        // Clean up error filtering
        m_scopedErrorFilter.reset();
    }

private:
    struct ComputeQueue
    {
        ComPtr<ID3D12CommandAllocator>    m_commandAllocatorCompute;
        ComPtr<ID3D12GraphicsCommandList> m_commandListCompute;
        ComPtr<ID3D12CommandQueue>        m_commandQueueCompute;

        // ID3D12Fences require access to the graphics command queue, which we don't have.
        // So we use manual fences instead.
        D3D12_GPU_VIRTUAL_ADDRESS         m_fenceGfxToCompute;
        D3D12_GPU_VIRTUAL_ADDRESS         m_fenceComputeToGfx;

        uint64_t                          m_fenceValueGfxToCompute;
        uint64_t                          m_fenceValueComputeToGfx;
    };
    void*                              m_fenceAddress;
    std::unique_ptr<ScopedErrorFilter> m_scopedErrorFilter;

    class ComputeTest : public Test
    {
    public:
        static constexpr uint32_t m_threadGroupX = 8;
        static constexpr uint32_t m_threadGroupY = 8;
        static constexpr uint32_t m_threadGroupZ = 1;

        ComputeTest()
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Wave size", L"", 10, 0);
            report->AddColumn(L"Wave per TG", L"", 12, 0);
            report->AddColumn(L"TG per CU", L"", 10, 0);
#ifdef TEST_SIMD_WALK_ALGORITHM
            report->AddColumn(L"SIMD walk", L"", 10, 0);
#endif
#ifdef TEST_ORDER_MODE
            report->AddColumn(L"Order mode", L"", 10, 0);
#endif
            report->AddColumn(L"#VGPR", L"", 10, 0);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Wave(API)", L"", 15, 0);
            report->AddColumn(L"Wave/clock", L"", 10, 2);

            report->AddHeader();
        }
    };


    class LaunchRateAsyncCsTest final : public ComputeTest
    {
    public:
        LaunchRateAsyncCsTest(uint32_t waveSize,
            uint32_t wavePerTG,
            uint32_t numThreadgroupsWalkedPerCu,
            D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM simdWalkAlgorithm,
            uint32_t populatedVgprs,
            bool orderMode,
            const std::vector<ComputeQueue*>& computeQueues
        ) :
            m_waveSize(waveSize),
            m_wavePerTG(wavePerTG),
            m_numThreadgroupsWalkedPerCu(numThreadgroupsWalkedPerCu),
            m_simdWalkAlgorithm(simdWalkAlgorithm),
            m_populatedVgprs(populatedVgprs),
            m_orderMode(orderMode),
            m_computeQueues(computeQueues)
        {
            const auto& gpuProperties = GpuProperties::Get();

            m_dispatchSizeX = 1024U / m_wavePerTG;
            m_dispatchSizeY = 256U * (uint32_t)gpuProperties.m_numSe;
            m_dispatchSizeZ = 1U;
        }

        void Initialize(ID3D12Device* device) override
        {
            std::wostringstream shaderName;
            shaderName << L"LaunchRateCsWave" << m_waveSize << L"Tg" << m_wavePerTG << L"Vgpr" << m_populatedVgprs << L"Cs.cso";
            auto computeShaderBlob = DX::ReadData(shaderName.str().c_str());
            DX::ThrowIfFailed(device->CreateRootSignature(0,
                computeShaderBlob.data(),
                computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_rootSignature);

            D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignature.Get(),                          // ID3D12RootSignature* pRootSignature;
                {
                    computeShaderBlob.data(),
                    computeShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE CS;
                0,                                              // UINT NodeMask;
                { nullptr, 0, },                                // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            };
            D3D12XBOX_COMPUTE_SHADER_LIMITS_DESC descComputeShaderLimits = {};
#ifdef _GAMING_XBOX_SCARLETT
            D3D12XboxInitializeDefaultComputeShaderLimits(&descComputeShaderLimits);
#else
            D3D12XboxInitializeDefaultComputeShaderLimits(FALSE, &descComputeShaderLimits);
#endif
            descComputeShaderLimits.NumThreadgroupsWalkedPerCu = m_numThreadgroupsWalkedPerCu;
            descComputeShaderLimits.SimdWalkAlgorithm = m_simdWalkAlgorithm;
            D3D12XBOX_EXTENDED_COMPUTE_PIPELINE_STATE_DESC descExtendedPipelineState[] =
            {
                {
                    D3D12XBOX_EXTENDED_COMPUTE_PIPELINE_STATE_SHADER_LIMITS,
                    descComputeShaderLimits,
                },
            };
            DX::ThrowIfFailed(device->CreateComputePipelineStateX(&descPipelineState,
                _countof(descExtendedPipelineState),
                descExtendedPipelineState,
                IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);

            D3D12_DESCRIPTOR_HEAP_DESC descHeap =
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                1U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_GRAPHICS_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeap);

            NullDescriptor(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), device->GetDescriptorHandleIncrementSize(descHeap.Type));
        }

        void Uninitialize() override
        {
            m_rootSignature.Reset();
            m_pipelineState.Reset();

            m_descriptorHeap.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Wave size = ";
            name << m_waveSize;
            name << L", ";
            name << L"Wave per TG = ";
            name << m_wavePerTG;
            name << L", ";
            name << L"TG per CU = ";
            name << m_numThreadgroupsWalkedPerCu;
#ifdef TEST_SIMD_WALK_ALGORITHM
            name << L", ";
            name << L"SIMD walk = ";
            name << m_simdWalkAlgorithm;
#endif
#ifdef TEST_ORDER_MODE
            name << L", ";
            name << L"Order mode = ";
            name << m_orderMode;
#endif
            name << L", ";
            name << L"#VGPR = ";
            name << m_populatedVgprs;

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = Test::m_elapsedTime;
            auto waveAPI = m_wavePerTG * m_dispatchSizeX * m_dispatchSizeY * m_dispatchSizeZ * m_computeQueues.size();
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto wavePerClock = (1000.0f * waveAPI) / (timeMs * clockSpeed);

            report->AddRowData(m_waveSize);
            report->AddRowData(m_wavePerTG);
            report->AddRowData(m_numThreadgroupsWalkedPerCu);
#ifdef TEST_SIMD_WALK_ALGORITHM
            report->AddRowData(m_simdWalkAlgorithm);
#endif
#ifdef TEST_ORDER_MODE
            report->AddRowData(m_orderMode);
#endif
            report->AddRowData(m_populatedVgprs);
            report->AddRowData(timeMs);
            report->AddRowData(waveAPI);
            report->AddRowData(wavePerClock);

            report->EndRow();
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            // Reset fences 
            // - fence writes must be Executed now, or we will think the GPU is hung
            // - all engines wait in Run() until they see the clear value before proceeding
            for (auto qIndex = 0U; qIndex < m_computeQueues.size(); ++qIndex)
            {
                auto& queue = *m_computeQueues[qIndex];

                commandList->Write64BitValueBottomOfPipeX(queue.m_fenceGfxToCompute, ++queue.m_fenceValueGfxToCompute, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);

                queue.m_commandListCompute->Write64BitValueBottomOfPipeX(queue.m_fenceComputeToGfx, ++queue.m_fenceValueComputeToGfx, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);

                DX::ThrowIfFailed(queue.m_commandListCompute->Close());
                queue.m_commandQueueCompute->ExecuteCommandLists(1U, CommandListCast(queue.m_commandListCompute.GetAddressOf()));
                queue.m_commandQueueCompute->KickoffX();
            }
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            ID3D12DescriptorHeap* descriptorHeaps[] =
            {
                m_descriptorHeap.Get(),
            };

            for (auto qIndex = 0U; qIndex < m_computeQueues.size(); ++qIndex)
            {
                auto& queue = *m_computeQueues[qIndex];

                // Wait for completion of the compute queue before calling reset on the command allocator
                while (*reinterpret_cast<uint64_t*>(queue.m_fenceComputeToGfx) < queue.m_fenceValueComputeToGfx);

                DX::ThrowIfFailed(queue.m_commandAllocatorCompute->Reset());
                DX::ThrowIfFailed(queue.m_commandListCompute->Reset(queue.m_commandAllocatorCompute.Get(), nullptr));

                queue.m_commandListCompute->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

                queue.m_commandListCompute->SetComputeRootSignature(m_rootSignature.Get());
                queue.m_commandListCompute->SetPipelineState(m_pipelineState.Get());

                // Unbind all views, so our CS outputs are dropped.
                queue.m_commandListCompute->SetComputeRootDescriptorTable(ROOT_PARAMETER_INDEX_UAV, m_descriptorHeap->GetGPUDescriptorHandleForHeapStart());

                // Wait until all engines see the "reset" signal
                commandList->Wait64BitValueX(queue.m_fenceComputeToGfx, D3D12_COMPARISON_FUNC_GREATER_EQUAL, queue.m_fenceValueComputeToGfx, D3D12XBOX_WAIT_FLAG_NONE);
                queue.m_commandListCompute->Wait64BitValueX(queue.m_fenceGfxToCompute, D3D12_COMPARISON_FUNC_GREATER_EQUAL, queue.m_fenceValueGfxToCompute, D3D12XBOX_WAIT_FLAG_NONE);

                // Signal that data collection has started on the graphics pipe
                commandList->Write64BitValueBottomOfPipeX(queue.m_fenceGfxToCompute, ++queue.m_fenceValueGfxToCompute, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);
            }

            for (auto qIndex = 0U; qIndex < m_computeQueues.size(); ++qIndex)
            {
                auto& queue = *m_computeQueues[qIndex];

                // Block async compute activity until after data collection has started on the graphics pipe
                queue.m_commandListCompute->Wait64BitValueX(queue.m_fenceGfxToCompute, D3D12_COMPARISON_FUNC_GREATER_EQUAL, queue.m_fenceValueGfxToCompute, D3D12XBOX_WAIT_FLAG_NONE);

#ifdef TEST_ORDER_MODE
                D3D12XBOX_DISPATCH_FLAGS flags = D3D12XBOX_DISPATCH_FLAG_NONE;

#ifdef _GAMING_XBOX_SCARLETT
                if (m_waveSize <= 32)
                {
                    flags |= D3D12XBOX_DISPATCH_FLAG_W32;
                }
#endif
                if (m_orderMode)
                {
                    flags |= D3D12XBOX_DISPATCH_FLAG_ENABLE_OUT_OF_ORDER;
                }
                queue.m_commandListCompute->DispatchX(m_dispatchSizeX, m_dispatchSizeY, m_dispatchSizeZ, flags);
#else // TEST_ORDER_MODE
                queue.m_commandListCompute->Dispatch(m_dispatchSizeX, m_dispatchSizeY, m_dispatchSizeZ);
#endif

                // Signal that async compute activity is finished
                queue.m_commandListCompute->Write64BitValueBottomOfPipeX(queue.m_fenceComputeToGfx, ++queue.m_fenceValueComputeToGfx, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);

                // Submit the async compute work
                DX::ThrowIfFailed(queue.m_commandListCompute->Close());
                queue.m_commandQueueCompute->ExecuteCommandLists(1U, CommandListCast(queue.m_commandListCompute.GetAddressOf()));
                queue.m_commandQueueCompute->KickoffX();
            }

            for (auto qIndex = 0U; qIndex < m_computeQueues.size(); ++qIndex)
            {
                auto& queue = *m_computeQueues[qIndex];

                // Wait until async compute activity is finished
                commandList->Wait64BitValueX(queue.m_fenceComputeToGfx, D3D12_COMPARISON_FUNC_GREATER_EQUAL, queue.m_fenceValueComputeToGfx, D3D12XBOX_WAIT_FLAG_NONE);
            }
        }

        void Stop(ID3D12GraphicsCommandList* /*commandList*/) const override
        {
            for (auto qIndex = 0U; qIndex < m_computeQueues.size(); ++qIndex)
            {
                auto& queue = *m_computeQueues[qIndex];

                // In this case we don't need to explicitly wait for compute to idle. The
                // gfx waits on compute and the test framework makes the CPU wait on gfx
                DX::ThrowIfFailed(queue.m_commandAllocatorCompute->Reset());
                DX::ThrowIfFailed(queue.m_commandListCompute->Reset(queue.m_commandAllocatorCompute.Get(), nullptr));
            }
        }

    protected:
        // Resources which are unique per-test
        uint32_t                                            m_waveSize;
        uint32_t                                            m_wavePerTG;
        uint32_t                                            m_numThreadgroupsWalkedPerCu;
        D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM             m_simdWalkAlgorithm;
        uint32_t                                            m_populatedVgprs;
        bool                                                m_orderMode;
        std::vector<ComputeQueue*>                          m_computeQueues;

        ComPtr<ID3D12RootSignature>                         m_rootSignature;
        ComPtr<ID3D12PipelineState>                         m_pipelineState;

        ComPtr<ID3D12DescriptorHeap>                        m_descriptorHeap;

        uint32_t                                            m_dispatchSizeX;
        uint32_t                                            m_dispatchSizeY;
        uint32_t                                            m_dispatchSizeZ;
    };
};

LaunchRateAsyncCsBenchmark asyncBenchmark;

