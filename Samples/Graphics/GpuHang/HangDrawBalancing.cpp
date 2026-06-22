//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class HangDrawBalancing final : public Hang
{
public:
    HangDrawBalancing() :
        Hang(
            ((1U << QueueType::Graphics)),
            ((1U << HangAction::Dump))
        ),
        m_vertexBufferView{},
        m_indexBufferView{},
        m_viewport{},
        m_scissorRect{}
    {
    }

    virtual ~HangDrawBalancing() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Draw Balancing";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"DrawBalancing";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs only on Xbox One X (Scorpio) hardware. ");
        description.push_back(L"");
        description.push_back(L"If the title enables draw balancing and makes a draw with a large number of indices, ");
        description.push_back(L"then the following draw may hang. ");

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);

        // Create the pipeline state, which includes loading shaders.
        auto vertexShaderBlob = DX::ReadData(L"SimpleTriangle12Vs.cso");
        auto pixelShaderBlob = DX::ReadData(L"SimpleTriangle12Ps.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, pixelShaderBlob.data(), pixelShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        static const D3D12_INPUT_ELEMENT_DESC s_inputElementDesc[2] =
        {
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
            { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { s_inputElementDesc, _countof(s_inputElementDesc) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        psoDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateDrawBalancing.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_pipelineStateDrawBalancing);

        if (IsScorpioClass(device))
        {
            psoDesc.Flags |= D3D12XBOX_PIPELINE_STATE_FLAG_DISABLE_DRAW_BALANCING;
        }

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateNoDrawBalancing.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_pipelineStateNoDrawBalancing);

        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        // Create vertex buffer.
        {
            struct Vertex
            {
                DirectX::XMVECTOR position;
                DirectX::XMVECTOR color;
            };
            const auto scale = 0.001f;

            static Vertex s_vertexData[3] =
            {
                { { scale * 0.0f,  scale * 0.5f,  0.5f, 1.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },  // Top / Red
                { { scale * 0.5f,  scale * -0.5f, 0.5f, 1.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },  // Right / Green
                { { scale * -0.5f, scale * -0.5f, 0.5f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }   // Left / Blue
            };

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
            auto const resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(s_vertexData));

            DX::ThrowIfFailed(
                device->CreateCommittedResource(&heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_vertexBuffer.ReleaseAndGetAddressOf())));

            // Copy the triangle data to the vertex buffer.
            UINT8* pVertexDataBegin;
            CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
            DX::ThrowIfFailed(
                m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, s_vertexData, sizeof(s_vertexData));
            m_vertexBuffer->Unmap(0, nullptr);

            // Initialize the vertex buffer view.
            m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.StrideInBytes = sizeof(Vertex);
            m_vertexBufferView.SizeInBytes = sizeof(s_vertexData);
        }

        // Create index buffer.
        {
            static uint16_t s_indexData[std::max(m_primitiveCount1, m_primitiveCount2)][3] =
            {
            };

            for (auto primitive = s_indexData; primitive < s_indexData + _countof(s_indexData); ++primitive)
            {
                (*primitive)[0] = 0;
                (*primitive)[1] = 1;
                (*primitive)[2] = 2;
            };

            auto descIndexBuf = CD3DX12_RESOURCE_DESC::Buffer(sizeof(s_indexData));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&uploadHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descIndexBuf,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_indexBuffer.ReleaseAndGetAddressOf())));

            // Copy the data to the index buffer.
            UINT8* pVertexDataBegin;
            CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
            DX::ThrowIfFailed(
                m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, s_indexData, sizeof(s_indexData));
            m_indexBuffer->Unmap(0, nullptr);

            // Initialize the index buffer view.
            m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
            m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
            m_indexBufferView.SizeInBytes = sizeof(s_indexData);
        }

        // Create the render target
        {
            auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                1024,
                1024,
                1,
                1,
                1,
                0,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

            auto clearValueColor = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DirectX::Colors::Black);

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &clearValueColor,
                IID_GRAPHICS_PPV_ARGS(m_renderTarget.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_renderTarget);

            auto numRtvDescriptors = 1U;
            D3D12_DESCRIPTOR_HEAP_DESC descRtvCpuDescriptorHeap =
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                         // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                numRtvDescriptors,                                      // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                        // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                0,                                                      // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descRtvCpuDescriptorHeap, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeapRtv);

            D3D12_CPU_DESCRIPTOR_HANDLE hRtvCpuDescriptorHeapStart = m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart();
            uint32_t rtvHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            uint32_t rtvCpuDescriptorIndex = 0;
            m_descriptorCpuRtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(hRtvCpuDescriptorHeapStart, int(rtvCpuDescriptorIndex++), rtvHandleIncrementSize);

            if (numRtvDescriptors != rtvCpuDescriptorIndex)
            {
                throw std::exception("Mismatch in count of descriptors");
            }

            device->CreateRenderTargetView(m_renderTarget.Get(), nullptr, m_descriptorCpuRtv);
        }

        // Create viewport, scissor rect
        m_viewport = CD3DX12_VIEWPORT(m_renderTarget.Get());
        m_scissorRect = CD3DX12_RECT(0, 0, 1024, 1024);

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

        commandQueue->KickoffX();

        m_commandList->Reset(m_commandAllocator.Get(), nullptr);
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;

        m_rootSignature = nullptr;
        m_pipelineStateDrawBalancing = nullptr;
        m_pipelineStateNoDrawBalancing = nullptr;
        m_vertexBuffer = nullptr;
        m_renderTarget = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
    }

    virtual void Render(ID3D12Device* device, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        PIXBeginRetailEvent(m_commandList.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        // Set up the render target and depth buffer
        m_commandList->OMSetRenderTargets(1, &m_descriptorCpuRtv, FALSE, nullptr);
        m_commandList->ClearRenderTargetView(m_descriptorCpuRtv, DirectX::Colors::Black, 0, nullptr);

        // Set the viewport and scissor rect.
        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);

        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        if (IsScorpioClass(device) && hang)
        {
            m_commandList->SetPipelineState(m_pipelineStateDrawBalancing.Get());
        }
        else
        {
            m_commandList->SetPipelineState(m_pipelineStateNoDrawBalancing.Get());
        }

        // Set necessary state.
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetIndexBuffer(&m_indexBufferView);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

        // Draw the magic numbers of primitives.
        m_commandList->DrawIndexedInstanced(3 * m_primitiveCount1, 1, 0, 0, 0);
        m_commandList->DrawIndexedInstanced(3 * m_primitiveCount2, 1, 0, 0, 0);

        PIXEndRetailEvent(m_commandList.Get());

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

        DX::ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_pipelineStateNoDrawBalancing;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_pipelineStateDrawBalancing;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_renderTarget;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapRtv;

    D3D12_VERTEX_BUFFER_VIEW                                m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW                                 m_indexBufferView;
    CD3DX12_CPU_DESCRIPTOR_HANDLE                           m_descriptorCpuRtv;
    D3D12_VIEWPORT                                          m_viewport;
    D3D12_RECT                                              m_scissorRect;

    // These are the successive primitive counts which produced the hang
    static const uint32_t                                   m_primitiveCount1 = 0x00100100;
    static const uint32_t                                   m_primitiveCount2 = 0x000fff00;
};

static HangDrawBalancing hang;
