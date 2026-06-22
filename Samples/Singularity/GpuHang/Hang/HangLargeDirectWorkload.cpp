//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FindMedia.h"
#include "Hang.h"
#include "Util.h"

class HangLargeDirectWorkload final : public Hang
{
public:
    HangLargeDirectWorkload() :
        Hang(
            ((1U << QueueType::Graphics)),
#ifdef LIVE_DEBUGGING_SUPPORT
            ((1U << HangAction::Dump) | (1U << HangAction::LiveDebug) | (1U << HangAction::LiveDebugThenDump))
#else
            ((1U << HangAction::Dump))
#endif
        ),
        m_viewport{},
        m_scissorRect{}
    {
    }

    virtual ~HangLargeDirectWorkload() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Large direct workload";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"LargeDirectWorkload";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if the title accidentally submits a workload which takes several seconds. ");
        description.push_back(L"One possible scenario is a bad transform matrix, which causes every primitive to cover the screen. ");

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);

        // Create descriptor heaps.
        {
            // Describe and create a shader resource view (SRV) heap for the texture.
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.NumDescriptors = 1;
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            DX::ThrowIfFailed(
                device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_descriptorHeapSrv.ReleaseAndGetAddressOf())));
        }

        // Create a root signature with one sampler and one texture
        {
            CD3DX12_ROOT_PARAMETER rpGsSrv = {};
            rpGsSrv.InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_GEOMETRY);

            CD3DX12_DESCRIPTOR_RANGE descRange = {};
            descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

            CD3DX12_ROOT_PARAMETER rpPsSrvTable = {};
            rpPsSrvTable.InitAsDescriptorTable(1, &descRange, D3D12_SHADER_VISIBILITY_PIXEL);

            D3D12_ROOT_PARAMETER rp[] =
            {
                rpGsSrv, 
                rpPsSrvTable, 
            };

            // Use a static sampler that matches the defaults
            // https://msdn.microsoft.com/en-us/library/windows/desktop/dn913202(v=vs.85).aspx#static_sampler
            D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
            samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.MaxAnisotropy = 16;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.Init(_countof(rp), rp, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_NONE);

            Microsoft::WRL::ComPtr<ID3DBlob> signature;
            Microsoft::WRL::ComPtr<ID3DBlob> error;
            HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
            if (FAILED(hr))
            {
                if (error)
                {
                    OutputDebugStringA(reinterpret_cast<const char*>(error->GetBufferPointer()));
                }
                throw DX::com_exception(hr);
            }

            DX::ThrowIfFailed(
                device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                    IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
        }

        // Create the pipeline state, which includes loading shaders.
        auto vertexShaderBlob = DX::ReadData(L"PointSpriteVs.cso");
        auto geometryShaderBlob = DX::ReadData(L"PointSpriteGs.cso");
        auto pixelShaderBlob = DX::ReadData(L"PointSpritePs.cso");

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.GS = { geometryShaderBlob.data(), geometryShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        psoDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));

        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        // Create the scale factors buffer
        {
            typedef DirectX::XMVECTOR Scale;
            typedef Scale ScaleBuffer[m_primitiveCount];

            Scale* scaleBuffer = new ScaleBuffer;

            // Whoops, what if the previous contents were unlucky
            for (auto i = 0U; i < m_primitiveCount; ++i)
            {
                scaleBuffer[i] = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
            }

            auto descBufInput = CD3DX12_RESOURCE_DESC::Buffer(m_primitiveCount * sizeof(Scale));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&uploadHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descBufInput,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(m_bufScale.ReleaseAndGetAddressOf())));

            // Copy the linked list data to the buffer.
            void* bufData = nullptr;
            CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
            DX::ThrowIfFailed(m_bufScale->Map(0, &readRange, &bufData));
#pragma warning(suppress: 6386) // Buffer size is guaranteed by the matching resource desc
            memcpy(bufData, scaleBuffer, sizeof(ScaleBuffer));
            m_bufScale->Unmap(0, nullptr);

            delete[] scaleBuffer;
        }

        Microsoft::WRL::ComPtr<ID3D12Resource> textureUploadHeap;
        {
            D3D12_RESOURCE_DESC txtDesc = {};
            txtDesc.MipLevels = txtDesc.DepthOrArraySize = 1;
            txtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // sunset.jpg is in sRGB colorspace
            txtDesc.SampleDesc.Count = 1;
            txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            wchar_t strFilePath[MAX_PATH] = {};
            DX::FindMediaFile(strFilePath, MAX_PATH, L"sunset.jpg");

            UINT width, height;
            auto image = LoadBGRAImage(strFilePath, width, height);
            txtDesc.Width = width;
            txtDesc.Height = height;

            DX::ThrowIfFailed(
                device->CreateCommittedResource(
                    &defaultHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &txtDesc,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_PPV_ARGS(m_texture.ReleaseAndGetAddressOf())));

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
                    IID_PPV_ARGS(textureUploadHeap.GetAddressOf())));

            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = image.data();
            textureData.RowPitch = static_cast<LONG_PTR>(txtDesc.Width * sizeof(uint32_t));
            textureData.SlicePitch = LONG_PTR(image.size());

            UpdateSubresources(m_commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            m_commandList->ResourceBarrier(1, &barrier);

            // Describe and create a SRV for the texture.
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = txtDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_descriptorHeapSrv->GetCPUDescriptorHandleForHeapStart());
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
                IID_PPV_ARGS(m_renderTarget.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_renderTarget);

            auto numRtvDescriptors = 1U;
            D3D12_DESCRIPTOR_HEAP_DESC descRtvCpuDescriptorHeap =
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                         // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                numRtvDescriptors,                                      // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                        // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                0,                                                      // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descRtvCpuDescriptorHeap, IID_PPV_ARGS(m_descriptorHeapRtv.ReleaseAndGetAddressOf())));

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

        // Wait for previous uploads to complete before deleting the upload heap
        GpuFullStop(device, commandQueue);

        m_commandList->Reset(m_commandAllocator.Get(), nullptr);
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;

        m_descriptorHeapSrv = nullptr;
        m_rootSignature = nullptr;
        m_pipelineState = nullptr;
        m_texture = nullptr;
        m_renderTarget = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
    }

    virtual void Render(ID3D12Device* /* device */, ID3D12CommandQueue* commandQueue, UINT /*vendorId*/, bool hang) override
    {
        PIXBeginRetailEvent(m_commandList.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        // We should be populating with reasonable values
        if (!hang)
        {
            typedef DirectX::XMVECTOR Scale;
            typedef Scale ScaleBuffer[m_primitiveCount];

            Scale* scaleBuffer = new ScaleBuffer;

            for (auto i = 0U; i < m_primitiveCount; ++i)
            {
                scaleBuffer[i] = DirectX::XMVectorSet(0.001f, 0.001f, 0.001f, 0.001f);
            }

            // Copy the linked list data to the buffer.
            void* bufData = nullptr;
            CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
            DX::ThrowIfFailed(m_bufScale->Map(0, &readRange, &bufData));
#pragma warning(suppress: 6386) // Buffer size is guaranteed by the matching resource desc
            memcpy(bufData, scaleBuffer, sizeof(ScaleBuffer));
            m_bufScale->Unmap(0, nullptr);

            delete[] scaleBuffer;
        }

        // Set up the render target and depth buffer
        m_commandList->OMSetRenderTargets(1, &m_descriptorCpuRtv, FALSE, nullptr);
        m_commandList->ClearRenderTargetView(m_descriptorCpuRtv, DirectX::Colors::Black, 0, nullptr);

        // Set the viewport and scissor rect.
        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);

        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        m_commandList->SetPipelineState(m_pipelineState.Get());

        auto heap = m_descriptorHeapSrv.Get();
        m_commandList->SetDescriptorHeaps(1, &heap);

        m_commandList->SetGraphicsRootShaderResourceView(0, m_bufScale->GetGPUVirtualAddress());
        m_commandList->SetGraphicsRootDescriptorTable(1, m_descriptorHeapSrv->GetGPUDescriptorHandleForHeapStart());

        // Set necessary state.
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

        // Draw 1000000 instanced sprites.
        m_commandList->DrawInstanced(m_primitiveCount, 1, 0, 0);

        PIXEndEvent(m_commandList.Get());

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

        DX::ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufScale;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_texture;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_renderTarget;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapSrv;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE                           m_descriptorCpuRtv;
    D3D12_VIEWPORT                                          m_viewport;
    D3D12_RECT                                              m_scissorRect;

    static const uint32_t                                   m_primitiveCount = 1000000;
};

static HangLargeDirectWorkload hang;
