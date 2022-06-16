//--------------------------------------------------------------------------------------
// DrawCullData.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "DrawCullData.h"

using namespace DX;
using namespace DirectX;
using namespace ATG;

namespace
{
    const wchar_t* s_boundingSphereMsFilename = L"BoundingSphereMS.cso";
    const wchar_t* s_normalConeMsFilename = L"NormalConeMS.cso";
    const wchar_t* s_debugDrawPsFilename = L"DebugDrawPS.cso";
}

void DrawCullDataEffect::CreateDeviceResources(DeviceResources& deviceResources)
{
    auto device = deviceResources.GetD3DDevice();

    // Load shaders and extract our root signature
    auto normalConeMs = DX::ReadData(s_normalConeMsFilename);
    auto boundingSphereMs = DX::ReadData(s_boundingSphereMsFilename);
    auto pixelShader = DX::ReadData(s_debugDrawPsFilename);

    DX::ThrowIfFailed(device->CreateRootSignature(0, normalConeMs.data(), normalConeMs.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

    // Disable culling
    CD3DX12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterDesc.CullMode = D3D12_CULL_MODE_NONE;

    // Disable depth test & writes
    CD3DX12_DEPTH_STENCIL_DESC dsDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    dsDesc.DepthEnable = false;
    dsDesc.StencilEnable = false;
    dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    // Populate the Mesh Shader PSO descriptor
    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature        = m_rootSignature.Get();
    psoDesc.PS                    = { pixelShader.data(), pixelShader.size() };
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = deviceResources.GetBackBufferFormat();
    psoDesc.DSVFormat             = deviceResources.GetDepthBufferFormat();
    psoDesc.RasterizerState       = rasterDesc;
    psoDesc.DepthStencilState     = dsDesc;
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.SampleDesc            = DefaultSampleDesc();

    // Create normal cone pipeline
    {
        // Cone lines are drawn opaquely
        psoDesc.MS = { normalConeMs.data(), normalConeMs.size() };
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // Opaque

        auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

        // Populate the stream desc with our defined PSO descriptor
        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
        streamDesc.SizeInBytes                   = sizeof(meshStreamDesc);
        streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

        DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_normalConePso.ReleaseAndGetAddressOf())));
    }

    // Create bounding sphere pipeline
    {
        // bounding sphere pipeline requires additive blending
        CD3DX12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        blendDesc.RenderTarget[0].BlendEnable   = true;
        blendDesc.RenderTarget[0].BlendOp        = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlend       = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend      = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha   = D3D12_BLEND_OP_MAX;
        blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;

        psoDesc.MS = { boundingSphereMs.data(), boundingSphereMs.size() };
        psoDesc.BlendState = blendDesc;

        auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

        // Populate the stream desc with our defined PSO descriptor
        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
        streamDesc.SizeInBytes                   = sizeof(meshStreamDesc);
        streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

        DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_boundingSpherePso.ReleaseAndGetAddressOf())));
    }

    // Create shared constant buffer
    auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignUp(sizeof(DrawCullData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &cbDesc,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_constantBuffer.ReleaseAndGetAddressOf()))
    );
}

void DrawCullDataEffect::ReleaseResources()
{
    m_rootSignature.Reset();
    m_boundingSpherePso.Reset();
    m_normalConePso.Reset();
    m_constantBuffer.Reset();
}

void DrawCullDataEffect::SetConstants(DirectX::FXMVECTOR color, DX::OrbitCamera& camera, const VQS& world)
{
    XMMATRIX view = camera.GetView();
    XMMATRIX proj = camera.GetProjection();

    XMMATRIX viewToWorld = XMMatrixTranspose(view);
    XMVECTOR camUp       = XMVector3TransformNormal(g_XMIdentityR1, viewToWorld);    // Y-axis is up direction
    XMVECTOR camForward  = XMVector3TransformNormal(g_XMNegIdentityR2, viewToWorld); // -Z-axis is forward direction

    XMStoreFloat4x4(&m_data.ViewProj, XMMatrixTranspose(view * proj));
    XMStoreFloat4x4(&m_data.World, XMMatrixTranspose(world.ToMatrix()));
    XMStoreFloat4(&m_data.Color, color);
    XMStoreFloat4(&m_data.ViewUp, camUp);
    XMStoreFloat3(&m_data.ViewForward, camForward);
    m_data.Scale = world.Scale;

    m_data.Color.w = 0.3f;
}

void DrawCullDataEffect::Draw(ID3D12GraphicsCommandList6* cmdList, MeshletSet& meshlet, uint32_t offset, uint32_t count)
{
    // Push constant data to GPU for our shader invocations
    assert(offset + count <= meshlet.GetMeshletCount());

    m_data.MeshletOffset = offset;
    m_data.MeshletCount = count;

    auto cbMem = GraphicsMemory::Get().AllocateConstant(m_data);

    TransitionResource(cmdList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyBufferRegion(m_constantBuffer.Get(), 0, cbMem.Resource(), cbMem.ResourceOffset(), cbMem.Size());
    TransitionResource(cmdList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // Shared root signature between two shaders
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, meshlet.GetMeshInfoResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(2, meshlet.GetCullDataResource()->GetGPUVirtualAddress());

    // Dispatch bounding sphere draw
    cmdList->SetPipelineState(m_boundingSpherePso.Get());
    cmdList->DispatchMesh(count, 1, 1);

    // Dispatch normal cone draw
    cmdList->SetPipelineState(m_normalConePso.Get());
    cmdList->DispatchMesh(count, 1, 1);
}
