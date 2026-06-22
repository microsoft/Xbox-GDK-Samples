//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

#ifdef _GAMING_XBOX
static constexpr D3D12_HEAP_TYPE kDefaultHeapType = D3D12_HEAP_TYPE_DEFAULT;
#else // #ifdef _GAMING_DESKTOP
static constexpr D3D12_HEAP_TYPE kDefaultHeapType = D3D12_HEAP_TYPE_UPLOAD;
#endif

// This benchmark only gives accurate results in Release, due to instrumentation overhead.
// Some results in Debug/Profile, and in PIX playback, have unrealistically long duration.
class DrawBenchmark final : public Benchmark
{
public:
    DrawBenchmark() = default;

    ~DrawBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Draw";
    }

    void Initialize(ID3D12Device* device) override
    {
        for (auto draws : {16U, 1024U,})
        {
            AddTest(new DrawTestDirect(draws, true));
            AddTest(new DrawTestPredicated(draws, true));
            AddTest(new DrawTestPredicated(draws, false));
            AddTest(new DrawTestIndirectSimple(draws, true));
            AddTest(new DrawTestIndirectComplex(draws, true));
            AddTest(new DrawTestIndirectComplex(draws, false));
            AddTest(new DrawTestIndirectBatched(draws, true));
#ifdef _GAMING_XBOX
            AddTest(new DrawTestIndirectBundle(draws, true));
#endif
        }

#ifdef _GAMING_DESKTOP
        const auto& gpuProps = GpuProperties::Get();
        AddCounter_ElapsedTime();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            AddCounter("gpu__cycles_elapsed.sum");
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            AddCounter("GPUBusyCycles");
        }
#endif // #ifdef _GAMING_DESKTOP

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(DrawTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DrawTest::m_rootSignature);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
        {
            DrawTest::m_rootSignature.Get(),                // ID3D12RootSignature* pRootSignature;
            {
                vertexShaderBlob.data(),
                vertexShaderBlob.size(),
            },                                              // D3D12_SHADER_BYTECODE VS;
            {},                                              // D3D12_SHADER_BYTECODE PS;
            {},                                             // D3D12_SHADER_BYTECODE DS;
            {},                                             // D3D12_SHADER_BYTECODE HS;
            {},                                              // D3D12_SHADER_BYTECODE GS;
            {},                                             // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CD3DX12_BLEND_DESC(D3D12_DEFAULT),              // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                       // UINT SampleMask;
            CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),         // D3D12_RASTERIZER_DESC RasterizerState;
            CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),      // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {},                                             // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,            // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            0U,                                             // UINT NumRenderTargets;
            {},                                             // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
            DXGI_FORMAT_UNKNOWN,                            // DXGI_FORMAT DSVFormat;
            {
                1U,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
            },                                              // DXGI_SAMPLE_DESC SampleDesc;
            0U,                                             // UINT NodeMask;
            {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                 // D3D12_PIPELINE_STATE_FLAGS Flags;
        };
#ifdef _GAMING_DESKTOP
        // without setting D3D12_GRAPHICS_PIPELINE_STATE_DESC::DepthStencilState.DepthEnable == FALSE,
        // PSO creation fails with [ STATE_CREATION ERROR #682: CREATEGRAPHICSPIPELINESTATE_POSITION_NOT_PRESENT]:
        // Rasterization Unit is enabled (PixelShader is not NULL or Depth/Stencil test is enabled and RasterizedStream is not D3D12_SO_NO_RASTERIZED_STREAM) but position is not provided by the last shader before the Rasterization Unit.
        descPipelineState.DepthStencilState.DepthEnable = FALSE;
        // without setting D3D12_GRAPHICS_PIPELINE_STATE_DESC::SampleDesc.Quality == 0,
        // PSO creation fails with [STATE_CREATION ERROR #685: CREATEGRAPHICSPIPELINESTATE_INVALID_SAMPLE_DESC]:
        // No render target or depth - stencil formats are specified and sample count is 1 or 0. Sample quality must be 0.
        descPipelineState.SampleDesc.Quality = 0;
#endif
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(DrawTest::m_pipelineState.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DrawTest::m_pipelineState);

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(kDefaultHeapType);

        // Create a 1-element index buffer
        typedef UINT16 Index;   // must match DXGI_FORMAT_R16_UINT 
        typedef Index IndexBuffer[1U];
        auto descBuf = CD3DX12_RESOURCE_DESC::Buffer(
            sizeof(IndexBuffer)
        );
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE, 
            &descBuf, 
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(DrawTest::m_indexBuffer.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DrawTest::m_indexBuffer);

        // Zero-initialize the index buffer
        // This code is only valid for Xbox, due to the buffer not being in an UPLOAD heap
        void* mapData = nullptr;
        DrawTest::m_indexBuffer->Map(0U, nullptr, &mapData);
        IndexBuffer* indexData = reinterpret_cast<IndexBuffer*>(mapData);
        ZeroMemory(indexData, sizeof(*indexData));
        D3D12_RANGE writtenRange = { 0U, sizeof(*indexData), };
        DrawTest::m_indexBuffer->Unmap(0U, &writtenRange);

        D3D12_INDEX_BUFFER_VIEW indexBufferView =
        {
            DrawTest::m_indexBuffer->GetGPUVirtualAddress(),        // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
            sizeof(IndexBuffer),                                    // UINT SizeInBytes;
            DXGI_FORMAT_R16_UINT /* must match type "Index"*/,      // DXGI_FORMAT Format;
        };
        DrawTest::m_indexBufferView = indexBufferView;

        DrawTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        DrawTest::m_rootSignature.Reset();
    }

private:
    class DrawTest : public Test
    {
    public:
        DrawTest(uint32_t draws, bool execute) 
            : m_draws(draws)
            , m_execute(execute)
        {
        }

        void Initialize(ID3D12Device* /*device*/) override
        {
        }

        void Uninitialize() override
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Method", L"", 25);
            report->AddColumn(L"#Draws", L"", 10);
            report->AddColumn(L"Skipped", L"", 5);
            report->AddColumn(L"Time", L" ms", 7, 4);
            report->AddColumn(L"Clocks/draw", L"", 5, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = GetElapsedTimeMs();
#ifdef _GAMING_XBOX
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto clocksPerDraw = (timeMs * clockSpeed) / (1000.0f * m_draws);
#else // #ifdef _GAMING_DESKTOP
            const auto & gpuProps = GpuProperties::Get();
            auto clocks = 0.0;
            if (gpuProps.IsSupportedNvidiaGpu())
            {
                clocks = GetCounterValue("gpu__cycles_elapsed.sum");
            }
            else if (gpuProps.IsSupportedAmdGpu())
            {
                clocks = GetCounterValue("GPUBusyCycles");
            }
            auto clocksPerDraw = clocks / m_draws;
#endif

            report->AddRowData(GetName());
            report->AddRowData(m_draws);
            report->AddRowData(m_execute ? L"no" : L"yes");
            report->AddRowData(timeMs);
            report->AddRowData(clocksPerDraw);

            report->EndRow();
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;
        static ComPtr<ID3D12PipelineState>      m_pipelineState;
        static ComPtr<ID3D12Resource>           m_indexBuffer;
        static D3D12_INDEX_BUFFER_VIEW          m_indexBufferView;

    protected:
        void DrawSetup(ID3D12GraphicsCommandList* commandList) const
        {
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());
#ifdef _GAMING_XBOX
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#else //#ifdef _GAMING_DESKTOP
            // Xbox version causes: [ EXECUTION ERROR #611: PRIMITIVE_TOPOLOGY_MISMATCH_PIPELINE_STATE]
            // due to PSO topology mismatch
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
#endif
            commandList->IASetIndexBuffer(&m_indexBufferView);

            // Doesn't matter what these are, as all draws will be nops
            D3D12_VIEWPORT viewport = {};
            D3D12_RECT scissorRect = {};
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);
        }

        const uint32_t                                          m_draws;
        const bool                                              m_execute;
    };

    class DrawTestDirect final : public DrawTest
    {
    public:
        DrawTestDirect(uint32_t draws, bool execute) 
            : DrawTest(draws, execute)
        {
        }

        void Initialize(ID3D12Device* /*device*/) override
        {
        }

        void Uninitialize() override
        {
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"DrawIndexedInstanced";

            return name.str();
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            DrawSetup(commandList);
            if (m_execute)
            {
                for (auto draw = 0U; draw < m_draws; ++draw)
                {
                    commandList->DrawIndexedInstanced(1U, 1U, 0U, 0U, 0U);
                }
            }
        }
    };

    class DrawTestPredicated final : public DrawTest
    {
    public:
        DrawTestPredicated(uint32_t draws, bool execute) 
            : DrawTest(draws, execute)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(kDefaultHeapType);

            // Driver currently expects D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER for predication buffers
            auto descBufPredication = CD3DX12_RESOURCE_DESC::Buffer(
                UINT(m_draws * sizeof(Predicate)),
#ifdef _GAMING_XBOX
                D3D12XBOX_RESOURCE_FLAG_PREFER_PREDICATION_BUFFER | D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER
#else  // #ifdef _GAMING_DESKTOP
                D3D12_RESOURCE_FLAG_NONE
#endif
            );
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE, 
                &descBufPredication, 
                D3D12_RESOURCE_STATE_PREDICATION,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_bufPredication.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufPredication);

            // Zero-initialize the predication buffer
            // This code is only valid for Xbox, due to the buffer not being in an UPLOAD heap
            void* mapData = nullptr;
            m_bufPredication->Map(0U, nullptr, &mapData);
            ZeroMemory(mapData, m_draws * sizeof(Predicate));
            D3D12_RANGE writtenRange = { 0U, m_draws * sizeof(Predicate), };
            m_bufPredication->Unmap(0U, &writtenRange);
        }

        void Uninitialize() override
        {
            m_bufPredication.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"SetPredication";

            return name.str();
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            DrawSetup(commandList);

            auto offset = 0ULL;
            for (auto draw = 0U; draw < m_draws; ++draw)
            {
                auto op = m_execute ? D3D12_PREDICATION_OP_NOT_EQUAL_ZERO : D3D12_PREDICATION_OP_EQUAL_ZERO;
                commandList->SetPredication(m_bufPredication.Get(), offset, op);
                commandList->DrawIndexedInstanced(1U, 1U, 0U, 0U, 0U);
#ifdef _GAMING_XBOX
                commandList->SetPredication(nullptr, offset, op);
#else // #ifdef _GAMING_DESKTOP
                // Xbox version causes: [ EXECUTION ERROR #734: SET_PREDICATION_INVALID_PARAMETERS]
                commandList->SetPredication(nullptr, 0U, D3D12_PREDICATION_OP_EQUAL_ZERO);
#endif
                offset += sizeof(Predicate);
            }
        }

    private:
        using Predicate = uint64_t;

        ComPtr<ID3D12Resource>  m_bufPredication;
    };

    class DrawTestIndirect : public DrawTest
    {
    public:
        DrawTestIndirect(uint32_t draws, bool execute) 
            : DrawTest(draws, execute)
        {
        }

        struct IndirectArgs
        {
            D3D12_DRAW_INDEXED_ARGUMENTS                        m_drawIndexed;
        };
        using IndirectCount = uint32_t;

        void Initialize(ID3D12Device* device) override
        {
            // Create command signature with only a DrawIndexed (this is required by the fast path)
            D3D12_INDIRECT_ARGUMENT_DESC descIndirectArgument[1];
            descIndirectArgument[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

            D3D12_COMMAND_SIGNATURE_DESC descCommandSignature =
            {
                sizeof(IndirectArgs),                               // UINT ByteStride; 
                _countof(descIndirectArgument),                     // UINT NumArgumentDescs;
                descIndirectArgument,                               // const D3D12_INDIRECT_ARGUMENT_DESC* pArgumentDescs;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateCommandSignature(&descCommandSignature,
#ifdef _GAMING_XBOX
                m_rootSignature.Get(),
#else // #ifdef _GAMING_DESKTOP
                // Xbox version causes: STATE_CREATION ERROR #743: CREATECOMMANDSIGNATURE_INVALID
                nullptr,
#endif
                IID_GRAPHICS_PPV_ARGS(m_commandSignature.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_commandSignature);

            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(kDefaultHeapType);

            // Create an argument buffer with the same draw arguments as the direct test
            auto descBufArgument = CD3DX12_RESOURCE_DESC::Buffer(
                UINT(m_draws * sizeof(IndirectArgs)),
#ifdef _GAMING_XBOX
                D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER
#else // #ifdef _GAMING_DESKTOP
                D3D12_RESOURCE_FLAG_NONE
#endif
            );
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE, 
                &descBufArgument, 
                D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_bufArgument.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufArgument);

            // Initialize the argument buffer
            // This code is only valid for Xbox, due to the buffer not being in an UPLOAD heap
            void* argData = nullptr;
            m_bufArgument->Map(0U, nullptr, &argData);
            for (auto draw = 0U; draw < m_draws; ++draw)
            {
                auto& args = static_cast<IndirectArgs*>(argData)[draw];
                args.m_drawIndexed.IndexCountPerInstance = 1U;
                args.m_drawIndexed.InstanceCount = 1U;
                args.m_drawIndexed.StartIndexLocation = 0U;
                args.m_drawIndexed.BaseVertexLocation = 0U;
                args.m_drawIndexed.StartInstanceLocation = 0U;
            }
            D3D12_RANGE writtenArgRange = { 0U, m_draws * sizeof(IndirectArgs), };
            m_bufArgument->Unmap(0U, &writtenArgRange);

            // Create an count buffer with every count equal to 1
            auto descBufCount = CD3DX12_RESOURCE_DESC::Buffer(
                UINT(m_draws * sizeof(IndirectCount)),
#ifdef _GAMING_XBOX
                D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER
#else // #ifdef _GAMING_DESKTOP
                D3D12_RESOURCE_FLAG_NONE
#endif
            );
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,             
                D3D12_HEAP_FLAG_NONE, 
                &descBufCount, 
                D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_bufCount.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufCount);

            // Initialize the count buffer
            // This code is only valid for Xbox, due to the buffer not being in an UPLOAD heap
            void* countData = nullptr;
            m_bufCount->Map(0U, nullptr, &countData);
            for (auto draw = 0U; draw < m_draws; ++draw)
            {
                auto& count = static_cast<IndirectCount*>(countData)[draw];
                count = m_execute ? 1U : 0U;
            }
            D3D12_RANGE writtenCountRange = { 0U, m_draws * sizeof(IndirectCount), };
            m_bufCount->Unmap(0U, &writtenCountRange);
        }

        void Uninitialize() override
        {
            m_commandSignature.Reset();
            m_bufArgument.Reset();
            m_bufCount.Reset();
        }

    protected:
        ComPtr<ID3D12CommandSignature>      m_commandSignature;
        ComPtr<ID3D12Resource>              m_bufArgument;
        ComPtr<ID3D12Resource>              m_bufCount;
    };

    class DrawTestIndirectSimple final : public DrawTestIndirect
    {
    public:
        DrawTestIndirectSimple(uint32_t draws, bool execute) 
            : DrawTestIndirect(draws, execute)
        {
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"ExecuteIndirect simple";

            return name.str();
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            DrawSetup(commandList);

            if (m_execute)
            {
                // Individual draws without count buffer. This is the "simple" fast path
                auto argumentOffsetInBytes = 0ULL;
                for (auto draw = 0U; draw < m_draws; ++draw)
                {
                    commandList->ExecuteIndirect(m_commandSignature.Get(), 1U, m_bufArgument.Get(), argumentOffsetInBytes, nullptr, 0ULL);
                    argumentOffsetInBytes += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
                }
            }
        }
    };

    class DrawTestIndirectComplex final : public DrawTestIndirect
    {
    public:
        DrawTestIndirectComplex(uint32_t draws, bool execute) 
            : DrawTestIndirect(draws, execute)
        {
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"ExecuteIndirect complex";

            return name.str();
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            DrawSetup(commandList);

            // Individual draws with count buffer. This puts us into the "complex" slow path.
            auto argumentOffsetInBytes = 0ULL;
            auto countOffsetInBytes = 0ULL;
            for (auto draw = 0U; draw < m_draws; ++draw)
            {
                commandList->ExecuteIndirect(m_commandSignature.Get(), 1U, m_bufArgument.Get(), argumentOffsetInBytes, m_bufCount.Get(), countOffsetInBytes);
                argumentOffsetInBytes += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
                countOffsetInBytes += sizeof(IndirectCount);
            }
        }
    };

    class DrawTestIndirectBatched final : public DrawTestIndirect
    {
    public:
        DrawTestIndirectBatched(uint32_t draws, bool execute) 
            : DrawTestIndirect(draws, execute)
        {
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"ExecuteIndirect batched";

            return name.str();
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            DrawSetup(commandList);

            if (m_execute)
            {
                // Single call with count, but no count buffer. This should be faster than the individual-draw fast path.
                commandList->ExecuteIndirect(m_commandSignature.Get(), m_draws, m_bufArgument.Get(), 0ULL, nullptr, 0ULL);
            }
        }
    };

#ifdef _GAMING_XBOX
    class DrawTestIndirectBundle final : public DrawTest
    {
    public:
        DrawTestIndirectBundle(uint32_t draws, bool execute)
            : DrawTest(draws, execute),
            m_sizeBundle(0)
        {
        }

        struct BundleArgs
        {
            uint32_t                                            m_packetHeader;
            D3D12_DRAW_INDEXED_ARGUMENTS                        m_drawIndexed;
        };

        void Initialize(ID3D12Device* device) override
        {
            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

            // Create an bundle buffer with the same draw arguments as the direct test
            m_sizeBundle = UINT(m_draws * sizeof(BundleArgs));
            auto descBufBundle = CD3DX12_RESOURCE_DESC::Buffer(
                m_sizeBundle,
                D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER
            );
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,             
                D3D12_HEAP_FLAG_NONE, 
                &descBufBundle, 
                D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_bufBundle.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufBundle);

            // Initialize the bundle buffer
            // This code is only valid for Xbox, due to the buffer not being in an UPLOAD heap
            void* bundleData = nullptr;
            m_bufBundle->Map(0U, nullptr, &bundleData);
            for (auto draw = 0U; draw < m_draws; ++draw)
            {
                auto& args = static_cast<BundleArgs*>(bundleData)[draw];
                args.m_packetHeader = D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED;
                args.m_drawIndexed.IndexCountPerInstance = 1U;
                args.m_drawIndexed.InstanceCount = 1U;
                args.m_drawIndexed.StartIndexLocation = 0U;
                args.m_drawIndexed.BaseVertexLocation = 0U;
                args.m_drawIndexed.StartInstanceLocation = 0U;
            }
            D3D12_RANGE writtenArgRange = { 0U, m_sizeBundle, };
            m_bufBundle->Unmap(0U, &writtenArgRange);
        }

        void Uninitialize() override
        {
            m_bufBundle.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"ExecuteIndirectBundleX";

            return name.str();
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            DrawSetup(commandList);

            commandList->ExecuteIndirectBundleX(m_bufBundle.Get(), 0U, m_sizeBundle, nullptr, 0U, 0U);
        }

    private:
        ComPtr<ID3D12Resource>              m_bufBundle;
        uint32_t                            m_sizeBundle;
    };
#endif // #ifdef _GAMING_XBOX
};

ComPtr<ID3D12RootSignature> DrawBenchmark::DrawTest::m_rootSignature;
ComPtr<ID3D12PipelineState> DrawBenchmark::DrawTest::m_pipelineState;
ComPtr<ID3D12Resource>      DrawBenchmark::DrawTest::m_indexBuffer;
D3D12_INDEX_BUFFER_VIEW     DrawBenchmark::DrawTest::m_indexBufferView;

DrawBenchmark benchmark;
