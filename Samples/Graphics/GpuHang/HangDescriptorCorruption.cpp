//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

namespace
{
    struct Vertex
    {
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT2 texcoord;
    };
}

class HangDescriptorCorruption final : public Hang
{
public:
    HangDescriptorCorruption() :
        Hang(
            ((1U << QueueType::Graphics)),
#ifdef LIVE_DEBUGGING_SUPPORT
            ((1U << HangAction::Dump) | (1U << HangAction::LiveDebug) | (1U << HangAction::LiveDebugThenDump))
#else
            ((1U << HangAction::Dump))
#endif
        ),
        m_vertexBufferView{},
        m_indexBufferView{},
        m_viewport{},
        m_scissorRect{},
        m_descriptorMemory(nullptr)
    {
    }

    virtual ~HangDescriptorCorruption() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Descriptor corruption";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"DescriptorCorruption";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if a descriptor has invalid contents. ");
        description.push_back(L"Most commonly, the descriptor is either uninitialized or else freed too early. ");
        description.push_back(L"The exact outcome depends on exactly which bits are bad, and on the shader(s). ");
        description.push_back(L"Shaders will be stuck on either read/write instructions, or wait instructions following a read/write. ");

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);

        // Create the pipeline state, which includes loading shaders.
#ifdef _GAMING_XBOX_SCARLETT
        auto vertexShaderBlob = DX::ReadData(L"SimpleRWTextureVs.cso");
        auto pixelShaderBlob = DX::ReadData(L"SimpleRWTexturePs.cso");
#else
        auto vertexShaderBlob = DX::ReadData(L"SimpleTexture12Vs.cso");
        auto pixelShaderBlob = DX::ReadData(L"SimpleTexture12Ps.cso");
#endif

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        static const D3D12_INPUT_ELEMENT_DESC s_inputElementDesc[2] =
        {
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
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
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));

        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        // Create vertex buffer.
        {
            static const Vertex s_vertexData[4] =
            {
                { { -0.5f, -0.5f, 0.5f, 1.0f },{ 0.f, 1.f } },
                { { 0.5f, -0.5f, 0.5f, 1.0f },{ 1.f, 1.f } },
                { { 0.5f,  0.5f, 0.5f, 1.0f },{ 1.f, 0.f } },
                { { -0.5f,  0.5f, 0.5f, 1.0f },{ 0.f, 0.f } },
            };

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            auto descVertBuf = CD3DX12_RESOURCE_DESC::Buffer(sizeof(s_vertexData));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&uploadHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descVertBuf,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_vertexBuffer.ReleaseAndGetAddressOf())));

            // Copy the quad data to the vertex buffer.
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
            static const uint16_t s_indexData[6] =
            {
                3,1,0,
                2,1,3,
            };

            // See note above
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

        Microsoft::WRL::ComPtr<ID3D12Resource> textureUploadHeap;
        {
            D3D12_RESOURCE_DESC txtDesc = {};
            txtDesc.MipLevels = txtDesc.DepthOrArraySize = 1;
            txtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // sunset.jpg is in sRGB colorspace
            txtDesc.SampleDesc.Count = 1;
            txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            UINT width, height;
            auto image = LoadBGRAImage(L"sunset.jpg", width, height);
            txtDesc.Width = width;
            txtDesc.Height = height;
            txtDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
#ifdef _GAMING_XBOX_SCARLETT
            txtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;    // Just a hack to allow UAVs
#endif

            DX::ThrowIfFailed(
                device->CreateCommittedResource(
                    &defaultHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &txtDesc,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_texture.ReleaseAndGetAddressOf())));

            const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

            // Create the GPU upload buffer.
            auto descTexBuf = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
            DX::ThrowIfFailed(
                device->CreateCommittedResource(
                    &uploadHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descTexBuf,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(textureUploadHeap.GetAddressOf())));

            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = image.data();
            textureData.RowPitch = static_cast<LONG_PTR>(txtDesc.Width * sizeof(uint32_t));
            textureData.SlicePitch = LONG_PTR(image.size());

            UpdateSubresources(m_commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
#ifdef _GAMING_XBOX_SCARLETT
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#else
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
#endif
            m_commandList->ResourceBarrier(1, &barrier);

            // Describe and create a descriptor heap for the texture.
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.NumDescriptors = 1;
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            DX::ThrowIfFailed(
                device->CreateDescriptorHeap(&srvHeapDesc, IID_GRAPHICS_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf())));

#ifdef _GAMING_XBOX_SCARLETT
            // Describe and create a UAV for the texture.
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = txtDesc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            device->CreateUnorderedAccessView(m_texture.Get(), nullptr, &uavDesc, m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());
#else
            // Describe and create a SRV for the texture.
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = txtDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());
#endif
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

        // Create the depth buffer
        {
            auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_D32_FLOAT,
                1024,
                1024, 
                1, 
                1, 
                1, 
                0, 
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

            auto clearValueDepth = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0.0f, 0);

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearValueDepth,
                IID_GRAPHICS_PPV_ARGS(m_depthBuffer.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_depthBuffer);

            auto numDsvDescriptors = 1U;
            D3D12_DESCRIPTOR_HEAP_DESC descDsvCpuDescriptorHeap =
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_DSV,                         // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                numDsvDescriptors,                                      // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                        // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                0,                                                      // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descDsvCpuDescriptorHeap, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapDsv.ReleaseAndGetAddressOf())));

            D3D12_CPU_DESCRIPTOR_HANDLE hDsvCpuDescriptorHeapStart = m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart();
            uint32_t dsvHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            uint32_t dsvCpuDescriptorIndex = 0;
            m_descriptorCpuDsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(hDsvCpuDescriptorHeapStart, int(dsvCpuDescriptorIndex++), dsvHandleIncrementSize);

            if (numDsvDescriptors != dsvCpuDescriptorIndex)
            {
                throw std::exception("Mismatch in count of descriptors");
            }

            device->CreateDepthStencilView(m_depthBuffer.Get(), nullptr, m_descriptorCpuDsv);
        }

        // Create viewport, scissor rect
        m_viewport = CD3DX12_VIEWPORT(m_renderTarget.Get());
        m_scissorRect = CD3DX12_RECT(0, 0, 1024, 1024);

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

        // Wait for previous uploads to complete before deleting the upload heap
        GpuFullStop(device, commandQueue);

        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        // Implementation dependent: This is where the descriptor data is stored
        auto handle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_descriptorMemory = D3D12XboxGetWritePointer(handle);
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;

        m_descriptorHeap = nullptr;
        m_descriptorHeapRtv = nullptr;
        m_descriptorHeapDsv = nullptr;
        m_rootSignature = nullptr;
        m_pipelineState = nullptr;
        m_vertexBuffer = nullptr;
        m_indexBuffer = nullptr;
        m_texture = nullptr;
        m_renderTarget = nullptr;
        m_depthBuffer = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
    }

    virtual void Render(ID3D12Device* /* device */, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        PIXBeginRetailEvent(m_commandList.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        // Set up the render target and depth buffer
        m_commandList->OMSetRenderTargets(1, &m_descriptorCpuRtv, FALSE, &m_descriptorCpuDsv);
        m_commandList->ClearRenderTargetView(m_descriptorCpuRtv, DirectX::Colors::Black, 0, nullptr);
        m_commandList->ClearDepthStencilView(m_descriptorCpuDsv, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);

        // Set the viewport and scissor rect.
        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);

        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        m_commandList->SetPipelineState(m_pipelineState.Get());

        auto heap = m_descriptorHeap.Get();
        m_commandList->SetDescriptorHeaps(1, &heap);

        m_commandList->SetGraphicsRootDescriptorTable(0, m_descriptorHeap->GetGPUDescriptorHandleForHeapStart());

        // Set necessary state.
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->IASetIndexBuffer(&m_indexBufferView);

        // Draw quad.
        m_commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

        PIXEndRetailEvent(m_commandList.Get());

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));
        
        // Force validation now, before anything is corrupt
        commandQueue->KickoffX();

        if (hang)
        {
            // Simulate a typical case of descriptor corruption.
            // Pretend the underlying memory was reallocated and re-used while the command list was still in flight.
            // We can't actually Release everything, because that will unmap the pages.
            // But a title with its own memory manager might Release the descriptors and reclaim the pages.
#ifdef _GAMING_XBOX_SCARLETT
            // For Scarlett, it's hard to make a hanging descriptor.
            // The case below is a UAV with a BC format, which is illegal.
            static_cast<uint32_t*>(m_descriptorMemory)[0] = 0x04799B00;
            static_cast<uint32_t*>(m_descriptorMemory)[1] = 0xCAE00000;
            static_cast<uint32_t*>(m_descriptorMemory)[2] = 0x803FC03F;
            static_cast<uint32_t*>(m_descriptorMemory)[3] = 0xD0580FAC;
            static_cast<uint32_t*>(m_descriptorMemory)[4] = 0x00000000;
            static_cast<uint32_t*>(m_descriptorMemory)[5] = 0x80400080;
            static_cast<uint32_t*>(m_descriptorMemory)[6] = 0x00000000;
            static_cast<uint32_t*>(m_descriptorMemory)[7] = 0x00000000;
#else
            // For XboxOne, various types of garbage data will hang.
            static_cast<uint32_t*>(m_descriptorMemory)[0] = 0x10101010;
            static_cast<uint32_t*>(m_descriptorMemory)[1] = 0x10101010;
            static_cast<uint32_t*>(m_descriptorMemory)[2] = 0x10101010;
            static_cast<uint32_t*>(m_descriptorMemory)[3] = 0x10101010;
            static_cast<uint32_t*>(m_descriptorMemory)[4] = 0x10101010;
            static_cast<uint32_t*>(m_descriptorMemory)[5] = 0x10101010;
            static_cast<uint32_t*>(m_descriptorMemory)[6] = 0x10101010;
            static_cast<uint32_t*>(m_descriptorMemory)[7] = 0x10101010;
#endif
        }
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;

    // Use the graphics resources from the SimpleTriangle12 sample
    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_texture;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_renderTarget;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_depthBuffer;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapRtv;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapDsv;

    D3D12_VERTEX_BUFFER_VIEW                                m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW                                 m_indexBufferView;
    CD3DX12_CPU_DESCRIPTOR_HANDLE                           m_descriptorCpuRtv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE                           m_descriptorCpuDsv;
    D3D12_VIEWPORT                                          m_viewport;
    D3D12_RECT                                              m_scissorRect;

    void*                                                   m_descriptorMemory;
};

static HangDescriptorCorruption hang;
