//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class BarrierBenchmark final : public Benchmark
{
public:
    BarrierBenchmark() = default;

    ~BarrierBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Barrier";
    }

    void Initialize(ID3D12Device* /*device*/) override
    {
        AddTest(new AliasingBarrierTest());

        for (const auto& params : TransitionBarrierTest::m_allTransitionBarrierParams)
        {
            AddTest(new TransitionBarrierTest(params));
        }

        AddTest(new UavBarrierTest());

        BarrierTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
    }

private:
    class BarrierTest : public Test
    {
    protected:
        static constexpr uint32_t m_count = 1024U;
    public:
        BarrierTest() = default;

        void Initialize(ID3D12Device* /*device*/) override
        {
        }

        void Uninitialize() override
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Barrier type", L"", 40);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Per", L" us", 5, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;

            report->AddRowData(GetName());
            report->AddRowData(timeMs);
            report->AddRowData(1000.f * timeMs / m_count);

            report->EndRow();
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->ResourceBarrier((UINT)m_barriers.size(), m_barriers.data());
        }

    protected:
        // Resources which are unique per-test
        std::vector<D3D12_RESOURCE_BARRIER>                         m_barriers;
    };

    class AliasingBarrierTest final : public BarrierTest
    {
        const static auto m_sizeBytes = 256U;

    public:
        AliasingBarrierTest() : BarrierTest()
        {
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Aliasing Barrier";

            return name.str();
        }

        void Initialize(ID3D12Device* device) override
        {
            // Create resources
            for (auto i = 0U; i < m_count; ++i)
            {
                CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

                auto descBuf = CD3DX12_RESOURCE_DESC::Buffer(
                    UINT(m_sizeBytes),
                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                );
                ComPtr<ID3D12Resource> bufferBefore;
                DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,             
                    D3D12_HEAP_FLAG_NONE, 
                    &descBuf, 
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(bufferBefore.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(bufferBefore);
                m_buffersBefore.push_back(bufferBefore);

                ComPtr<ID3D12Resource> bufferAfter;
                DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,             
                    D3D12_HEAP_FLAG_NONE, 
                    &descBuf, 
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(bufferAfter.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(bufferAfter);
                m_buffersAfter.push_back(bufferAfter);

                m_barriers.push_back(CD3DX12_RESOURCE_BARRIER::Aliasing(m_buffersBefore[i].Get(), m_buffersAfter[i].Get()));
            }
        }

    private:
        std::vector<ComPtr<ID3D12Resource>> m_buffersBefore;
        std::vector<ComPtr<ID3D12Resource>> m_buffersAfter;
    };

    class TransitionBarrierTest final : public BarrierTest
    {
        const static auto m_sizeBytes = 256U;

    public:
        struct TransitionBarrierParams
        {
            D3D12_RESOURCE_FLAGS                            m_resourceFlags;
            D3D12_RESOURCE_STATES                           m_stateBefore;
            const wchar_t*                                  m_stateBeforeName;
            D3D12_RESOURCE_STATES                           m_stateAfter;
            const wchar_t*                                  m_stateAfterName;
        };

        static const std::vector<TransitionBarrierParams>   m_allTransitionBarrierParams;

        TransitionBarrierTest(const TransitionBarrierParams& params) : BarrierTest(),
            m_params(params)
        {
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Transition ";
            name << m_params.m_stateBeforeName;
            name << L" --> ";
            name << m_params.m_stateAfterName;

            return name.str();
        }

        void Initialize(ID3D12Device* device) override
        {
            // Create resources
            for (auto i = 0U; i < m_count; ++i)
            {
                CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

                auto descBuf = CD3DX12_RESOURCE_DESC::Buffer(
                    UINT(m_sizeBytes),
                    m_params.m_resourceFlags
                );
                ComPtr<ID3D12Resource> buffer;
                DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,             
                    D3D12_HEAP_FLAG_NONE, 
                    &descBuf, 
                    m_params.m_stateBefore,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(buffer.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(buffer);
                m_buffers.push_back(buffer);

                m_barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_buffers[i].Get(),
                    m_params.m_stateBefore,
                    m_params.m_stateAfter));

                m_inverseBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_buffers[i].Get(),
                    m_params.m_stateAfter,
                    m_params.m_stateBefore));
            }
        }

        void Stop(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->ResourceBarrier((UINT)m_inverseBarriers.size(), m_inverseBarriers.data());
        }

    private:
        TransitionBarrierParams                     m_params;

        std::vector<D3D12_RESOURCE_BARRIER>         m_inverseBarriers;

        std::vector<ComPtr<ID3D12Resource>>         m_buffers;
    };

    class UavBarrierTest final : public BarrierTest
    {
        const static auto m_sizeBytes = 256U;

    public:
        UavBarrierTest() : BarrierTest()
        {
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"UAV Barrier";

            return name.str();
        }

        void Initialize(ID3D12Device* device) override
        {
            // Create resources
            for (auto i = 0U; i < m_count; ++i)
            {
                CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

                auto descBuf = CD3DX12_RESOURCE_DESC::Buffer(
                    UINT(m_sizeBytes),
                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                );
                ComPtr<ID3D12Resource> buffer;
                DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,             
                    D3D12_HEAP_FLAG_NONE, 
                    &descBuf, 
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(buffer.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(buffer);
                m_buffers.push_back(buffer);

                m_barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(m_buffers[i].Get()));
            }
        }

    private:
        std::vector<ComPtr<ID3D12Resource>> m_buffers;
    };
};

const std::vector<BarrierBenchmark::TransitionBarrierTest::TransitionBarrierParams> BarrierBenchmark::TransitionBarrierTest::m_allTransitionBarrierParams =
{
    // UAV --> SRV
    {
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        L"UAV",
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        L"SRV",
    },
    // SRV --> UAV
    {
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        L"SRV",
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        L"UAV",
    },
    // UAV --> IndirectArgs
    {
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        L"UAV",
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        L"IndirectArgs",
    },
    // IndirectArgs --> UAV
    {
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        L"IndirectArgs",
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        L"UAV",
    },
};

BarrierBenchmark benchmark;

