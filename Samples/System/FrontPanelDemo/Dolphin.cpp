//--------------------------------------------------------------------------------------
// Dolphin.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "Dolphin.h"
#include "ReadData.h"

using namespace DirectX;
using namespace ATG;

Dolphin::Dolphin()
    : m_animationTime(0)
    , m_blendWeight(0)
    , m_primitiveType(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
    , m_indexCount(0)
    , m_vertexBufferView{}
    , m_indexBufferView{}
    , m_mappedVSConstantData(nullptr)
    , m_VSConstantDataGpuAddr{}
{
    m_animationTime = float(rand() % 100);
}

void Dolphin::Load(ID3D12Device *device, DirectX::RenderTargetState rtState, unsigned int backbufferCount)
{
    // Load models and textures
    {
        m_dolphinModel1 = Model::CreateFromSDKMESH(device, L"assets\\mesh\\Dolphin1.sdkmesh");
        m_dolphinModel2 = Model::CreateFromSDKMESH(device, L"assets\\mesh\\Dolphin2.sdkmesh");
        m_dolphinModel3 = Model::CreateFromSDKMESH(device, L"assets\\mesh\\Dolphin3.sdkmesh");

        {
            auto& meshes = m_dolphinModel1->meshes;
            auto& meshParts = meshes[0]->alphaMeshParts;
            auto& part = *meshParts[0];
            m_primitiveType = part.primitiveType;
            m_indexCount = part.indexCount;

            m_vertexBufferView[0].BufferLocation = part.vertexBuffer.GpuAddress();
            m_vertexBufferView[0].StrideInBytes = part.vertexStride;
            m_vertexBufferView[0].SizeInBytes = part.vertexBufferSize;

            m_indexBufferView.BufferLocation = part.indexBuffer.GpuAddress();
            m_indexBufferView.Format = part.indexFormat;
            m_indexBufferView.SizeInBytes = part.indexBufferSize;
        }

        {
            auto& meshes = m_dolphinModel2->meshes;
            auto& meshParts = meshes[0]->alphaMeshParts;
            auto& part = *meshParts[0];

            m_vertexBufferView[1].BufferLocation = part.vertexBuffer.GpuAddress();
            m_vertexBufferView[1].StrideInBytes = part.vertexStride;
            m_vertexBufferView[1].SizeInBytes = part.vertexBufferSize;
        }

        {
            auto& meshes = m_dolphinModel3->meshes;
            auto& meshParts = meshes[0]->alphaMeshParts;
            auto& part = *meshParts[0];

            m_vertexBufferView[2].BufferLocation = part.vertexBuffer.GpuAddress();
            m_vertexBufferView[2].StrideInBytes = part.vertexStride;
            m_vertexBufferView[2].SizeInBytes = part.vertexBufferSize;
        }
    }

    // Setup rendering
    {
        auto dolphinVSBlob = DX::ReadData(L"DolphinVS.cso");
        auto dolphinPSBlob = DX::ReadData(L"CausticsPS.cso");

        // Xbox One best practice is to use HLSL-based root signatures to support shader precompilation.

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, dolphinVSBlob.data(), dolphinPSBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        const D3D12_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "POSITION",  1, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",    1, DXGI_FORMAT_R32G32B32_FLOAT, 1, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  1, DXGI_FORMAT_R32G32_FLOAT,    1, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "POSITION",  2, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",    2, DXGI_FORMAT_R32G32B32_FLOAT, 2, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  2, DXGI_FORMAT_R32G32_FLOAT,    2, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { layout, _countof(layout) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { dolphinVSBlob.data(), dolphinVSBlob.size() };
        psoDesc.PS = { dolphinPSBlob.data(), dolphinPSBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = rtState.dsvFormat;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = rtState.rtvFormats[0];
        psoDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframePsoDesc = psoDesc;
        wireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&wireframePsoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateWireframe.ReleaseAndGetAddressOf())));
    }

    // Create the constant buffer memory and map the CPU and GPU addresses
    {
        const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        size_t cbSize = backbufferCount * AlignUp(sizeof(VS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_VSConstants.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(m_VSConstants->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedVSConstantData)));

        m_VSConstantDataGpuAddr = m_VSConstants->GetGPUVirtualAddress();
    }
}

void Dolphin::Update(float, float elapsedTime)
{
    // update the animation time for the dolphin
    m_animationTime += elapsedTime;

    // store the blend weight (this determines how fast the tale wags)
    m_blendWeight = sinf(6 * m_animationTime);

    // compute our rotation matrices and combine them with scale into our world matrix
    m_world = XMMatrixScaling(0.01f, 0.01f, 0.01f);

    // This rotation adds a little wiggle
    m_world = XMMatrixMultiply(XMMatrixRotationZ(cos(4 * m_animationTime) / 6.0f), m_world);

    // Translate and then rotate to make the dolphin swim in a circle
    m_world = XMMatrixMultiply(m_world, XMMatrixTranslation(0.0f, 0.0f, 8.0f));
    m_world = XMMatrixMultiply(m_world, XMMatrixRotationY(-m_animationTime / 2.0f));

    // Perturb the dolphin's position in vertical direction so that it looks more "floaty"
    m_world = XMMatrixMultiply(m_world, XMMatrixTranslation(0.0f, cos(4 * m_animationTime) / 3.0f, 0.0f));
}

VS_CONSTANT_BUFFER* Dolphin::MapVSConstants(unsigned int bufferIndex)
{
    return reinterpret_cast<VS_CONSTANT_BUFFER*>(
        reinterpret_cast<BYTE*>(m_mappedVSConstantData) + 
        AlignUp(sizeof(VS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) * bufferIndex);
}

void Dolphin::UnmapAndSetVSConstants(ID3D12GraphicsCommandList* 
    commandList, 
    unsigned int bufferIndex, 
    bool wireframe,
    ID3D12DescriptorHeap *Heap, 
    D3D12_GPU_DESCRIPTOR_HANDLE dolphinResourceView, 
    D3D12_GPU_DESCRIPTOR_HANDLE causticResourceView)
{
    // Set the root signature and pipeline state for the command list
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetPipelineState(wireframe ? m_pipelineStateWireframe.Get() : m_pipelineState.Get());

    // Shader parameters
    commandList->SetDescriptorHeaps(1, &Heap);
    commandList->SetGraphicsRootDescriptorTable(2, dolphinResourceView);
    commandList->SetGraphicsRootDescriptorTable(3, causticResourceView);

    commandList->SetGraphicsRootConstantBufferView(0, 
        m_VSConstantDataGpuAddr + AlignUp(sizeof(VS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) * bufferIndex);
}

void Dolphin::Render(ID3D12GraphicsCommandList * commandList)
{
    // Set up the input assembler
    commandList->IASetPrimitiveTopology(m_primitiveType);
    commandList->IASetVertexBuffers(0, 3, m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    // Draw
    commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
}

void Dolphin::Translate(DirectX::XMVECTOR t)
{
    m_translation = XMVectorAdd(m_translation, t);
}

DirectX::XMMATRIX Dolphin::GetWorld()
{
    // combine our existing world matrix with the translation
    XMMATRIX result = XMMatrixMultiply(m_world, XMMatrixTranslationFromVector(m_translation));
    return result;
}

float Dolphin::GetBlendWeight()
{
    return m_blendWeight;
}
