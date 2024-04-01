//--------------------------------------------------------------------------------------
// DrawFrustum.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "DrawFrustum.h"

using namespace DX;
using namespace DirectX;
using namespace ATG;

namespace
{
    const wchar_t* s_drawFrustumMsFilename = L"FrustumMS.cso";
    const wchar_t* s_debugDrawPsFilename = L"DebugDrawPS.cso";
}

void DrawFrustumEffect::CreateDeviceResources(DeviceResources& deviceResources)
{
    auto device = deviceResources.GetD3DDevice();

    // Load shader bytecode and extract root signature
    auto meshShader  = DX::ReadData(s_drawFrustumMsFilename);
    auto pixelShader = DX::ReadData(s_debugDrawPsFilename);

    // Extract root signature directly from the mesh shader bytecode
    DX::ThrowIfFailed(device->CreateRootSignature(0, meshShader.data(), meshShader.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature        = m_rootSignature.Get();
    psoDesc.MS                    = { meshShader.data(), meshShader.size() };
    psoDesc.PS                    = { pixelShader.data(), pixelShader.size() };
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = deviceResources.GetBackBufferFormat();
    psoDesc.DSVFormat             = deviceResources.GetDepthBufferFormat();
    psoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
    psoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
    psoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.SampleDesc            = DefaultSampleDesc();

    auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

    // Populate the stream desc with our defined PSO descriptor
    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
    streamDesc.SizeInBytes                   = sizeof(meshStreamDesc);
    streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

    // Create the MS PSO
    DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_pso.ReleaseAndGetAddressOf())));

    // Create the constant buffer
    auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignUp(sizeof(DrawFrustumData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &cbDesc,
#ifdef _GAMING_XBOX
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
#else
        // on PC buffers are created in the common state and can be promoted without a barrier to VERTEX_AND_CONSTANT_BUFFER on first access
        D3D12_RESOURCE_STATE_COMMON,
#endif
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_constantBuffer.ReleaseAndGetAddressOf()))
    );
}

void DrawFrustumEffect::ReleaseResources()
{
    m_rootSignature.Reset();
    m_pso.Reset();
    m_constantBuffer.Reset();
}

void DrawFrustumEffect::Update(FXMMATRIX vp, XMVECTOR (&planes)[6])
{
    XMStoreFloat4(&m_data.LineColor, DirectX::Colors::Purple);
    XMStoreFloat4x4(&m_data.ViewProj, XMMatrixTranspose(vp));

    for (uint32_t i = 0; i < _countof(m_data.Planes); ++i)
    {
        XMStoreFloat4(&m_data.Planes[i], planes[i]);
    }
}

void DrawFrustumEffect::Draw(ID3D12GraphicsCommandList6* cmdList)
{
    // Push constants to GPU
    auto cbMem = GraphicsMemory::Get().AllocateConstant(m_data);

    TransitionResource(cmdList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyBufferRegion(m_constantBuffer.Get(), 0, cbMem.Resource(), cbMem.ResourceOffset(), cbMem.Size());
    TransitionResource(cmdList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // Set root signature, resources, pipeline state and dispatch
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

    cmdList->SetPipelineState(m_pso.Get());
    cmdList->DispatchMesh(1, 1, 1);
}
