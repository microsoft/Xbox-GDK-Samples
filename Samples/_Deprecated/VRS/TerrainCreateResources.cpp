//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Terrain.h"
#include "CommonStates.h"
#include "EffectPipelineStateDescription.h"
#include "ReadData.h"
#include <xg_xs.h>
#include <xmem.h>

#pragma comment(lib,"xg_xs.lib")
#pragma comment(lib,"xmem.lib")

using namespace DirectX;


struct XG_RESOURCE_LAYOUT ComputeResourceLayout(const D3D12_RESOURCE_DESC& ResourceDesc)
{
    XG_RESOURCE_DESC XgDesc = (XG_RESOURCE_DESC&)ResourceDesc;

    Microsoft::WRL::ComPtr<XGTextureAddressComputer> pComputer;
    DX::ThrowIfFailed(XGCreateTextureComputer(&XgDesc, &pComputer));

    XG_RESOURCE_LAYOUT Layout;
    DX::ThrowIfFailed(pComputer->GetResourceLayout(&Layout));
    return Layout;
}


void BuildComputePSO(ID3D12Device *device, Microsoft::WRL::ComPtr<ID3D12PipelineState> &pso, const wchar_t* fileName, ID3D12RootSignature *pRootSignature)
{
    auto csBlob = DX::ReadData(fileName);

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = pRootSignature;
    desc.CS.pShaderBytecode = csBlob.data();
    desc.CS.BytecodeLength = csBlob.size();

    DX::ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(pso.ReleaseAndGetAddressOf())));
    pso->SetName(fileName);
}


void Terrain::Initialize(std::unique_ptr<DX::DeviceResources> &deviceResources, std::unique_ptr<GraphicsMemory> &graphicsMemory)
{
    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    auto device = deviceResources->GetD3DDevice();
    ResourceUploadBatch upload(device);
    upload.Begin();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescTex2D = {};
    uavDescTex2D.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescBuffer = {};
    srvDescBuffer.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescBuffer.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDescBuffer.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescBuffer = {};
    uavDescBuffer.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDescBuffer.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = deviceResources->GetBackBufferFormat();
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // descriptor heaps
    m_resourceDescriptorsCBV_SRV_UAV = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    m_resourceDescriptorRTV = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 2);
        
    // displays
    {
        // disabling CMask simply because the target is never cleared and every pixel written, so no point in having the meta data surface, VRS does work with CMask
        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(deviceResources->GetBackBufferFormat(), UINT(deviceResources->GetScissorRect().right), UINT(deviceResources->GetScissorRect().bottom), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_DENY_COLOR_COMPRESSION_DATA);

        for (UINT i = 0; i < 2; ++i)
        {
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_renderedImage[i].GetAddressOf())));
            {
                CreateShaderResourceView(device, m_renderedImage[i].Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::RenderedImage1x1SRV + i));
                uavDescTex2D.Format = desc.Format;
                device->CreateUnorderedAccessView(m_renderedImage[i].Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::RenderedImage1x1UAV + i));
                device->CreateRenderTargetView(m_renderedImage[i].Get(), &rtvDesc, m_resourceDescriptorRTV->GetCpuHandle(i));
            }
        }
        m_renderedImage[0]->SetName(L"m_renderedImage1x1");
        m_renderedImage[1]->SetName(L"m_renderedImageVRS");
    }
    // shading rate map
    {
        UINT tileSize = 8;  // this would be queried on PC
        UINT width = UINT(deviceResources->GetScissorRect().right) / tileSize;
        UINT height = UINT(deviceResources->GetScissorRect().bottom) / tileSize;

        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UINT, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_shadingRateImage.GetAddressOf())));
        {
            CreateShaderResourceView(device, m_shadingRateImage.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::ShadingRateMapSRV));
            uavDescTex2D.Format = desc.Format;
            device->CreateUnorderedAccessView(m_shadingRateImage.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::ShadingRateMapUAV));
        }        
        D3D12_RESOURCE_ALLOCATION_INFO info = device->GetResourceAllocationInfo(0, 1, &desc);
        m_shadingRateImageSizeBytes = UINT(info.SizeInBytes);
        m_shadingRateWidth = width;
        m_shadingRateHeight = height;
    }
    // height and normal maps
    {
        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16_FLOAT, m_heightMapSize, m_heightMapSize, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_displayHeightMap.GetAddressOf())));
            {
                m_displayHeightMap->SetName(L"m_heightMap");
                CreateShaderResourceView(device, m_displayHeightMap.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::HeightMapSRV));
                uavDescTex2D.Format = desc.Format;
                device->CreateUnorderedAccessView(m_displayHeightMap.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::HeightMapUAV));
            }
        }
        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, m_sourceHeightMapSize, m_sourceHeightMapSize, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            for (int i = 0; i < 2; ++i)
            {
                DX::ThrowIfFailed(device->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &desc,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_sourceHeightMap[i].GetAddressOf())));
                {
                    CreateShaderResourceView(device, m_sourceHeightMap[i].Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(size_t(Descriptors::SourceHeightMapSRV0 + i)));
                    uavDescTex2D.Format = desc.Format;
                    device->CreateUnorderedAccessView(m_sourceHeightMap[i].Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(size_t(Descriptors::SourceHeightMapUAV0 + i)));
                }
            }
            m_sourceHeightMap[0]->SetName(L"m_sourceHeightMap[0]");
            m_sourceHeightMap[1]->SetName(L"m_sourceHeightMap[1]");
        }
        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8_SNORM, m_sourceHeightMapSize, m_sourceHeightMapSize, 1, 4, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_normalMap.GetAddressOf())));
            {
                m_normalMap->SetName(L"m_normalMap");
                CreateShaderResourceView(device, m_normalMap.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::NormalMapSRV));
                uavDescTex2D.Format = desc.Format;
                device->CreateUnorderedAccessView(m_normalMap.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::NormalMapUAV_Mip0));
                uavDescTex2D.Texture2D.MipSlice = 1;
                device->CreateUnorderedAccessView(m_normalMap.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::NormalMapUAV_Mip1));
                uavDescTex2D.Texture2D.MipSlice = 2;
                device->CreateUnorderedAccessView(m_normalMap.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::NormalMapUAV_Mip2));
                uavDescTex2D.Texture2D.MipSlice = 3;
                device->CreateUnorderedAccessView(m_normalMap.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::NormalMapUAV_Mip3));

                // put this back for further use
                uavDescTex2D.Texture2D.MipSlice = 0;
            }
        }
    }
    // AABB minY/maxY buffer per tile for frustum culling
    {
        UINT32 aabbBufferSize = m_tileCount * sizeof(UINT32) * 2;

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(aabbBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_AABBTileMinMaxBuffer.GetAddressOf())));
        {
            m_AABBTileMinMaxBuffer->SetName(L"m_AABBTileMinMaxBuffer");

            srvDescBuffer.Format = DXGI_FORMAT_R32_UINT;
            srvDescBuffer.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDescBuffer.Buffer.NumElements = m_tileCount * 2;
            srvDescBuffer.Buffer.StructureByteStride = 0;
            device->CreateShaderResourceView(m_AABBTileMinMaxBuffer.Get(), &srvDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::AABBTileMinMaxSRV));

            uavDescBuffer.Format = DXGI_FORMAT_R32_UINT;
            uavDescBuffer.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDescBuffer.Buffer.NumElements = m_tileCount * 2;
            uavDescBuffer.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
            device->CreateUnorderedAccessView(m_AABBTileMinMaxBuffer.Get(), nullptr, &uavDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::AABBTileMinMaxUAV));
        }
    }
    // Low resolution maxY perform ray marching
    {
        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16_FLOAT, m_lowResMaxHeightSize, m_lowResMaxHeightSize, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_lowResMaxHeight.GetAddressOf())));
        {
            m_lowResMaxHeight->SetName(L"m_LowResMaxHeight");
            CreateShaderResourceView(device, m_lowResMaxHeight.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::LowResMaxHeightSRV));
            uavDescTex2D.Format = desc.Format;
            device->CreateUnorderedAccessView(m_lowResMaxHeight.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::LowResMaxHeightUAV));
        }
    }
    // instance buffers
    {
        UINT instanceBufferSize = m_tileCount * sizeof(UINT32);

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        for (UINT i = 0; i < LODs::Count; ++i)
        {
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_instanceBuffer[i].GetAddressOf())));
            {
                uavDescBuffer.Format = DXGI_FORMAT_R32_UINT;
                uavDescBuffer.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDescBuffer.Buffer.NumElements = m_tileCount;
                uavDescBuffer.Buffer.StructureByteStride = 0;
                device->CreateUnorderedAccessView(m_instanceBuffer[i].Get(), nullptr, &uavDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::InstanceBufferVeryLowResUAV + i));

                srvDescBuffer.Format = DXGI_FORMAT_R32_UINT;
                srvDescBuffer.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDescBuffer.Buffer.NumElements = m_tileCount;
                srvDescBuffer.Buffer.StructureByteStride = 0;
                device->CreateShaderResourceView(m_instanceBuffer[i].Get(), &srvDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::InstanceBufferVeryLowResSRV + i));
            }
        }
        m_instanceBuffer[LODs::VeryLow]->SetName(L"m_instanceBufferVeryLowRes");
        m_instanceBuffer[LODs::Low]->SetName(L"m_instanceBufferLowRes");
        m_instanceBuffer[LODs::Medium]->SetName(L"m_instanceBufferMediumRes");
        m_instanceBuffer[LODs::High]->SetName(L"m_instanceBufferHighRes");		
    }
    CreateIndexBuffers(deviceResources, graphicsMemory, upload);

    // PSO time!
    {
        // Terrain Generation Shaders
        {
            {
                auto csBlob = DX::ReadData(L"TerrainGenerateCS.cso");

                DX::ThrowIfFailed(device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_globalRS.ReleaseAndGetAddressOf())));
                m_globalRS->SetName(L"m_terrainRS");

                D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
                desc.CS.BytecodeLength = csBlob.size();
                desc.CS.pShaderBytecode = csBlob.data();
                desc.pRootSignature = m_globalRS.Get();

                DX::ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_terrainGenerateHeightsPSO.GetAddressOf())));
                m_terrainGenerateHeightsPSO->SetName(L"m_terrainGenerateHeightsPSO");
            }
            BuildComputePSO(device, m_terrainComputeAABBandNormalsPSO, L"TerrainComputeAABBandNormalsCS.cso", m_globalRS.Get());
            {
                switch (m_heightMapToLowResMaxSizeLog2)
                {
                case 4:
                    BuildComputePSO(device, m_terrainComputeLowResMaxHeight, L"TerrainCompute16x16maxY.cso", m_globalRS.Get());
                    break;
                case 3:
                    BuildComputePSO(device, m_terrainComputeLowResMaxHeight, L"TerrainCompute8x8maxY.cso", m_globalRS.Get());
                    break;

                default:
                    assert(0 && "no shader to build this size!");
                    break;
                }
            }
            BuildComputePSO(device, m_terrainInitAABBsPSO, L"TerrainInitAABBCS.cso", m_globalRS.Get());
            BuildComputePSO(device, m_terrainSmoothPSO, L"TerrainSmoothCS.cso", m_globalRS.Get());
            BuildComputePSO(device, m_terrainComputeAABBandNormalsPSO, L"TerrainComputeAABBandNormalsCS.cso", m_globalRS.Get());
        }
        // Rasterization
        {
            RenderTargetState rtState(deviceResources->GetBackBufferFormat(), deviceResources->GetDepthBufferFormat());

            EffectPipelineStateDescription pd(nullptr,
                CommonStates::Opaque,
                CommonStates::DepthDefault,
                CommonStates::CullClockwise,
                rtState);

            // Terrain
            {
                // common VS
                auto vsBlob = DX::ReadData(L"TerrainRenderVS.cso");
                D3D12_SHADER_BYTECODE vs = { vsBlob.data(), vsBlob.size() };

                // pixel shaders
                {
                    auto psBlob = DX::ReadData(L"TerrainRenderPS.cso");
                    D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                    pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_terrainRenderPSO.ReleaseAndGetAddressOf());
                    m_terrainRenderPSO->SetName(L"m_terrainRenderPSO");
                }
                {
                    auto psBlob = DX::ReadData(L"TerrainRenderAOOnlyPS.cso");
                    D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                    pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_terrainRenderAOOnlyPSO.ReleaseAndGetAddressOf());
                    m_terrainRenderAOOnlyPSO->SetName(L"m_terrainRenderAOOnlyPSO");
                }
            }
            // Sky
            {
                auto vsBlob = DX::ReadData(L"FullScreenTriangleVS.cso");
                auto psBlob = DX::ReadData(L"SkyRenderPS.cso");
                D3D12_SHADER_BYTECODE vs = { vsBlob.data(), vsBlob.size() };
                D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_skyRenderPSO.ReleaseAndGetAddressOf());
                m_skyRenderPSO->SetName(L"m_skyRenderPSO");
            }
        }
    }
    // Visibility culling
    BuildComputePSO(device, m_terrainVisibilityPSO, L"TerrainVisibilityCS.cso", m_globalRS.Get());

    // Calculate shading rate
    BuildComputePSO(device, m_shadingRatePSOs[ShadingRateCalculation::Wave64_HalfRes], L"GenerateShadingRateHalfRes8x8.cso", m_globalRS.Get());
    BuildComputePSO(device, m_shadingRatePSOs[ShadingRateCalculation::Wave64_FullRes], L"GenerateShadingRateFullRes8x8.cso", m_globalRS.Get());
    BuildComputePSO(device, m_shadingRatePSOs[ShadingRateCalculation::Wave32_HalfRes], L"GenerateShadingRateHalfRes8x8Wave32.cso", m_globalRS.Get());
    BuildComputePSO(device, m_shadingRatePSOs[ShadingRateCalculation::Wave32_FullRes], L"GenerateShadingRateFullRes8x8Wave32.cso", m_globalRS.Get());
    
    // Shading Rate Debug
    {
        RenderTargetState rtState(deviceResources->GetBackBufferFormat(), deviceResources->GetDepthBufferFormat());
        {
            auto vsBlob = DX::ReadData(L"FullScreenTriangleVS.cso");
            D3D12_SHADER_BYTECODE vs = { vsBlob.data(), vsBlob.size() };

            // solid
            {
                EffectPipelineStateDescription pd(nullptr,
                    CommonStates::Opaque,
                    CommonStates::DepthNone,
                    CommonStates::CullClockwise,
                    rtState);
                {
                    auto psBlob = DX::ReadData(L"ABSDifferencePS.cso");
                    D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                    pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_debugVisualizeAbsDiffPSO.ReleaseAndGetAddressOf());
                    m_debugVisualizeAbsDiffPSO->SetName(L"m_debugVisualizeAbsDiffPSO");
                }
                {
                    auto psBlob = DX::ReadData(L"VisualizeShadingRatePS.cso");
                    D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                    pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_debugVisualizeShadingRatePSO.ReleaseAndGetAddressOf());
                    m_debugVisualizeShadingRatePSO->SetName(L"m_debugVisualizeShadingRatePSO");
                }
            }
            // transparent
            {
                EffectPipelineStateDescription pd(nullptr,
                    CommonStates::AlphaBlend,
                    CommonStates::DepthNone,
                    CommonStates::CullClockwise,
                    rtState);
                {
                    auto psBlob = DX::ReadData(L"VisualizeShadingRatePS.cso");
                    D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                    pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_debugVisualizeShadingRateOverlayPSO.ReleaseAndGetAddressOf());
                    m_debugVisualizeShadingRateOverlayPSO->SetName(L"m_debugVisualizeShadingRateOverlayPSO");
                }
            }
        }
    }
    // Execute Indirect Resources
    {
        {
            D3D12_INDIRECT_ARGUMENT_DESC argDesc;
            ZeroMemory(&argDesc, sizeof(argDesc));
            argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

            D3D12_COMMAND_SIGNATURE_DESC desc;
            desc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
            desc.NumArgumentDescs = 1;
            desc.pArgumentDescs = &argDesc;
            desc.NodeMask = 0;

            DX::ThrowIfFailed(device->CreateCommandSignature(&desc, nullptr, IID_GRAPHICS_PPV_ARGS(m_commandSignature.ReleaseAndGetAddressOf())));
            m_commandSignature->SetName(L"m_commandSignature");
        }
        {
            UINT numIndirectArgs = LODs::Count * 2;
            UINT indirectArgsSize = numIndirectArgs * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
            auto desc = CD3DX12_RESOURCE_DESC::Buffer(indirectArgsSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_drawIndexedInstancedArgsBuffer.GetAddressOf())
            ));
            m_drawIndexedInstancedArgsBuffer->SetName(L"m_drawIndexedInstancedArgsBuffer");

            uavDescBuffer.Format = desc.Format;
            uavDescBuffer.Buffer.NumElements = numIndirectArgs;
            uavDescBuffer.Buffer.StructureByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
            device->CreateUnorderedAccessView(m_drawIndexedInstancedArgsBuffer.Get(), nullptr, &uavDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::DrawArgsUAV));

            SharedGraphicsResource drawIndexedArgumentsUpload = graphicsMemory->Allocate(indirectArgsSize);

            auto drawIndexedArguments = static_cast<D3D12_DRAW_INDEXED_ARGUMENTS *>(drawIndexedArgumentsUpload.Memory());

            for (UINT i = 0; i < numIndirectArgs; ++i)
            {
                drawIndexedArguments[i].InstanceCount = 0;
                drawIndexedArguments[i].StartIndexLocation = 0;
                drawIndexedArguments[i].BaseVertexLocation = 0;
                drawIndexedArguments[i].StartInstanceLocation = 0;
                drawIndexedArguments[i].IndexCountPerInstance = m_numIndices[i >> 1];
            }
            upload.Upload(m_drawIndexedInstancedArgsBuffer.Get(), drawIndexedArgumentsUpload);
            upload.Transition(m_drawIndexedInstancedArgsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }
    }
    auto finish = upload.End(deviceResources->GetCommandQueue());

    deviceResources->WaitForGpu();

    finish.wait();
}

