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
using namespace DirectX::SimpleMath;
using namespace ATG;

namespace
{
    const wchar_t* s_drawFrustumMsFilename = L"DrawFrustumMS.cso";
    const wchar_t* s_debugDrawPsFilename = L"DebugDrawPS.cso";
}

DrawFrustumEffect::DrawFrustumEffect()
    : m_data{ }
{
    XMStoreFloat4(&m_data.LineColor, DirectX::Colors::Blue);
    m_data.Thickness = 1.0f;
}

void DrawFrustumEffect::CreateDeviceResources(DeviceResources& deviceResources, CommonStates& commonStates)
{
    auto device = deviceResources.GetD3DDevice();

    // Load shader bytecode and extract root signature
    auto frustumMS = DX::ReadData(s_drawFrustumMsFilename);
    auto debugDrawPS = DX::ReadData(s_debugDrawPsFilename);

    // Extract root signature directly from the mesh shader bytecode
    DX::ThrowIfFailed(device->CreateRootSignature(0, frustumMS.data(), frustumMS.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
    
    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature        = m_rootSignature.Get();
    psoDesc.MS                    = { frustumMS.data(), frustumMS.size() };
    psoDesc.PS                    = { debugDrawPS.data(), debugDrawPS.size() };
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = deviceResources.GetBackBufferFormat();
    psoDesc.DSVFormat             = deviceResources.GetDepthBufferFormat();
    psoDesc.RasterizerState       = commonStates.CullNone; 
    psoDesc.BlendState            = commonStates.Opaque; 
    psoDesc.DepthStencilState     = commonStates.DepthDefault;
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
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignUp(sizeof(DrawFrustumData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
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

void DrawFrustumEffect::Update(FXMMATRIX viewProj, DirectX::CXMMATRIX debugViewProj)
{
    XMStoreFloat4x4(&m_data.ViewProj, XMMatrixTranspose(viewProj));

    XMMATRIX vp = XMMatrixTranspose(debugViewProj);
    Vector4 rows[4] = { vp.r[0], vp.r[1], vp.r[2], vp.r[3] };

    XMVECTOR planes[6] =
    {
        XMPlaneNormalize(rows[3] + rows[0]), // Left
        XMPlaneNormalize(rows[3] - rows[0]), // Right
        XMPlaneNormalize(rows[3] + rows[1]), // Bottom
        XMPlaneNormalize(rows[3] - rows[1]), // Top
        XMPlaneNormalize(rows[2]),           // Near
        XMPlaneNormalize(rows[3] - rows[2]), // Far
    };

    for (uint32_t i = 0; i < 6; ++i)
    {
        XMStoreFloat4(&m_data.Planes[i], planes[i]);
    }
}

void DrawFrustumEffect::Draw(ID3D12GraphicsCommandList6* commandList)
{
    auto cbMem = GraphicsMemory::Get().AllocateConstant(m_data);

    TransitionResource(commandList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->CopyBufferRegion(m_constantBuffer.Get(), 0, cbMem.Resource(), cbMem.ResourceOffset(), cbMem.Size());
    TransitionResource(commandList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

    commandList->SetPipelineState(m_pso.Get());
    commandList->DispatchMesh(1, 1, 1);
}
