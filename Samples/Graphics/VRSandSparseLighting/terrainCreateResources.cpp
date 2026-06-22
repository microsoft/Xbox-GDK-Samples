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


void BuildComputePSO(ID3D12Device* device, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pso, const wchar_t* fileName, ID3D12RootSignature* pRootSignature)
{
    auto csBlob = DX::ReadData(fileName);

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = pRootSignature;
    desc.CS.pShaderBytecode = csBlob.data();
    desc.CS.BytecodeLength = csBlob.size();

    DX::ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(pso.ReleaseAndGetAddressOf())));
    pso->SetName(fileName);
}


void Terrain::Initialize(std::unique_ptr<DX::DeviceResources>& deviceResources, std::unique_ptr<GraphicsMemory>& graphicsMemory)
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
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // descriptor heaps
    m_resourceDescriptorsCBV_SRV_UAV = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    m_resourceDescriptorRTV = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        RenderTargets::Count);

    UINT width = UINT(deviceResources->GetScissorRect().right);
    UINT height = UINT(deviceResources->GetScissorRect().bottom);

    // depth texture
    {
        // Must match DeviceResources::CreateWindowSizeDependentResources()
        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_TYPELESS,
            width,
            height,
            1, // Use a single array entry.
            1  // Use a single mipmap level.
        );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        XG_RESOURCE_LAYOUT Layout = ComputeResourceLayout(depthStencilDesc);

        D3D12_SHADER_RESOURCE_VIEW_DESC depthStencilSrvDesc = {};
        depthStencilSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        depthStencilSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        depthStencilSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        depthStencilSrvDesc.Texture2D.MipLevels = 1;

        device->CreateShaderResourceView(deviceResources->GetDepthStencil(), &depthStencilSrvDesc, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::DepthSRV));

        const XG_PLANE_LAYOUT* HTilePlane = nullptr;

        for (uint32_t i = 0; i < Layout.Planes; ++i)
        {
            const XG_PLANE_LAYOUT& Plane = Layout.Plane[i];
            if(Plane.Usage == XG_PLANE_USAGE_HTILE)
            {
                HTilePlane = &Plane;
                break;
            }
        }
        assert(HTilePlane != nullptr);
        _Analysis_assume_(HTilePlane != nullptr);  // Tell static analyzer HTilePlane is non-nullptr
        {
            D3D12_GPU_VIRTUAL_ADDRESS HTileAddr = deviceResources->GetDepthStencil()->GetGPUVirtualAddress() + HTilePlane->BaseOffsetBytes;

            D3D12_RESOURCE_DESC Desc = {};
            Desc.Alignment = 0;
            Desc.DepthOrArraySize = 1;
            Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            Desc.Format = DXGI_FORMAT_UNKNOWN;
            Desc.Height = 1;
            Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            Desc.MipLevels = 1;
            Desc.SampleDesc.Count = 1;
            Desc.SampleDesc.Quality = 0;
            Desc.Width = HTilePlane->SizeBytes;

            DX::ThrowIfFailed(device->CreatePlacedResourceX(
                HTileAddr,
                &Desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_HTileBuffer.GetAddressOf())));
            {
                m_HTileBuffer->SetName(L"m_HTileBuffer");

                srvDescBuffer.Format = DXGI_FORMAT_R32_UINT;
                srvDescBuffer.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDescBuffer.Buffer.NumElements = UINT(HTilePlane->SizeBytes / 4);
                srvDescBuffer.Buffer.StructureByteStride = 0;
                device->CreateShaderResourceView(m_HTileBuffer.Get(), &srvDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::HTileSRV));
            }
            // Generate an HTile descriptor for the compute decompression system
            {
                D3D12XBOX_GPU_HARDWARE_CONFIGURATION hwConfig;
                device->GetGpuHardwareConfigurationX(&hwConfig);

                uint32_t TileCountX = AlignUp(width, 8) / 8;
                uint32_t TileCountY = AlignUp(height, 8) / 8;
#ifdef _GAMING_XBOX_SCARLETT
                constexpr uint32_t IsHTileLinear = 0u;
                const uint32_t PipeCount = hwConfig.HardwareVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 8u : 32u;
                const uint32_t MacroTileWidth = hwConfig.HardwareVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 64u : 128u;
                const uint32_t MacroTileHeight = hwConfig.HardwareVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 64u : 128u;
#else
                constexpr uint32_t IsHTileLinear = width * height < 0x200000 ? 1u : 0u;
                const uint32_t PipeCount = hwConfig.HardwareVersion >= D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X ? 8u : 4u;
                constexpr uint32_t MacroTileWidth = 64u;
                const uint32_t MacroTileHeight = 8u * PipeCount;
#endif
                if (!IsHTileLinear)
                {
                    TileCountX = AlignUp(TileCountX, MacroTileWidth);
                    TileCountY = AlignUp(TileCountY, MacroTileHeight);
                    assert(HTilePlane->SizeBytes == TileCountX * TileCountY * 4);
                }
                m_HTileInfo = IsHTileLinear << 31 | PipeCount << 24 | TileCountX | TileCountY << 12;
            }
        }
        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        uavDescTex2D.Format = desc.Format;

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_LinearDepth.GetAddressOf())));
        {
            m_LinearDepth->SetName(L"m_LinearDepth");

            CreateShaderResourceView(device, m_LinearDepth.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::LinearDepthSRV));
            device->CreateUnorderedAccessView(m_LinearDepth.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::LinearDepthUAV));
        }
    }
    // render targets
    {
        // Back buffer UAV
        {
            uavDescTex2D.Format = deviceResources->GetBackBufferFormat();
            device->CreateUnorderedAccessView(deviceResources->GetRenderTarget(0), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::BackBufferUAV0));
            device->CreateUnorderedAccessView(deviceResources->GetRenderTarget(1), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::BackBufferUAV1));
        }
        // disabling CMask simply because the target is never cleared and every pixel written, so no point in having the meta data surface, VRS does work with CMask
        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(deviceResources->GetBackBufferFormat(), UINT(deviceResources->GetScissorRect().right), UINT(deviceResources->GetScissorRect().bottom), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_DENY_COLOR_COMPRESSION_DATA);
            rtvDesc.Format = desc.Format;
            uavDescTex2D.Format = desc.Format;

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_litOutput.GetAddressOf())));
            {
                m_litOutput->SetName(L"m_LitOutput");

                CreateShaderResourceView(device, m_litOutput.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::LitOutputSRV));
                device->CreateUnorderedAccessView(m_litOutput.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::LitOutputUAV));
                device->CreateRenderTargetView(m_litOutput.Get(), &rtvDesc, m_resourceDescriptorRTV->GetCpuHandle(RenderTargets::LitOutput));
            }
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_postProcessOutput.GetAddressOf())));
            {
                m_postProcessOutput->SetName(L"m_postProcessOutput");

                CreateShaderResourceView(device, m_postProcessOutput.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::PostProcessSRV));
                device->CreateUnorderedAccessView(m_postProcessOutput.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::PostProcessUAV));
                device->CreateRenderTargetView(m_postProcessOutput.Get(), &rtvDesc, m_resourceDescriptorRTV->GetCpuHandle(RenderTargets::PostProcessOutput));
            }
        }
        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R9G9B9E5_SHAREDEXP, UINT(deviceResources->GetScissorRect().right), UINT(deviceResources->GetScissorRect().bottom), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_DENY_COLOR_COMPRESSION_DATA);
            rtvDesc.Format = desc.Format;
            uavDescTex2D.Format = desc.Format;

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_GbufferAlbedo.GetAddressOf())));
            {
                m_GbufferAlbedo->SetName(L"m_GbufferAlbedo");

                CreateShaderResourceView(device, m_GbufferAlbedo.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::GbufferAlbedoSRV));
                device->CreateUnorderedAccessView(m_GbufferAlbedo.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::GbufferAlbedoUAV));
                device->CreateRenderTargetView(m_GbufferAlbedo.Get(), &rtvDesc, m_resourceDescriptorRTV->GetCpuHandle(RenderTargets::GBufferAlbedo));
            }
        }
        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R10G10B10A2_UNORM, UINT(deviceResources->GetScissorRect().right), UINT(deviceResources->GetScissorRect().bottom), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_DENY_COLOR_COMPRESSION_DATA);
            rtvDesc.Format = desc.Format;
            uavDescTex2D.Format = desc.Format;

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_GbufferNormalRoughness.GetAddressOf())));
            {
                m_GbufferNormalRoughness->SetName(L"m_GbufferNormalRoughness");

                CreateShaderResourceView(device, m_GbufferNormalRoughness.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::GBufferRoughnessNormalSRV));
                device->CreateUnorderedAccessView(m_GbufferNormalRoughness.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::GBufferRoughnessNormalUAV));
                device->CreateRenderTargetView(m_GbufferNormalRoughness.Get(), &rtvDesc, m_resourceDescriptorRTV->GetCpuHandle(RenderTargets::GBufferRoughnessNormal));
            }
        }
        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, UINT(deviceResources->GetScissorRect().right), UINT(deviceResources->GetScissorRect().bottom), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_DENY_COLOR_COMPRESSION_DATA);
            rtvDesc.Format = desc.Format;
            uavDescTex2D.Format = desc.Format;

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_GbufferDebugAndShadingRate.GetAddressOf())));
            {
                m_GbufferDebugAndShadingRate->SetName(L"m_GbufferDebugAndShadingRate");

                CreateShaderResourceView(device, m_GbufferDebugAndShadingRate.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::GBufferDebugAndShadingRateSRV));
                device->CreateUnorderedAccessView(m_GbufferDebugAndShadingRate.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::GBufferDebugAndShadingRateUAV));
                device->CreateRenderTargetView(m_GbufferDebugAndShadingRate.Get(), &rtvDesc, m_resourceDescriptorRTV->GetCpuHandle(RenderTargets::GBufferDebugAndShadingRate));
            }
        }
    }
    // shading rate map for VRS
    {
        UINT tileSize = 8;  // this would be queried on PC
        m_shadingRateImageVRSWidth = AlignUp(width, tileSize) / tileSize;
        m_shadingRateImageVRSHeight = AlignUp(height, tileSize) / tileSize;

        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UINT, m_shadingRateImageVRSWidth, m_shadingRateImageVRSHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_shadingRateImageVRS.GetAddressOf())));
        {
            m_shadingRateImageVRS->SetName(L"m_shadingRateImageVRS");

            CreateShaderResourceView(device, m_shadingRateImageVRS.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::ShadingRateImage8x8_SRV));
            uavDescTex2D.Format = desc.Format;
            device->CreateUnorderedAccessView(m_shadingRateImageVRS.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::ShadingRateImage8x8_UAV));

            D3D12_RESOURCE_ALLOCATION_INFO info = device->GetResourceAllocationInfo(0, 1, &desc);
            m_shadingRateImageVRSSizeBytes = UINT(info.SizeInBytes);
        }
    }
    // specific shading rate map for sparse lighting
    {
        UINT tileSize = 2;
        m_shadingRateImageSparseLightingWidth = AlignUp(width, tileSize) / tileSize;
        m_shadingRateImageSparseLightingHeight = AlignUp(height, tileSize) / tileSize;

        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UINT, m_shadingRateImageSparseLightingWidth, m_shadingRateImageSparseLightingHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_shadingRateImageSparseLighting.GetAddressOf())));
        {
            m_shadingRateImageSparseLighting->SetName(L"m_shadingRateImageSparseLighting");

            CreateShaderResourceView(device, m_shadingRateImageSparseLighting.Get(), m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::ShadingRateImage2x2_SRV));
            uavDescTex2D.Format = desc.Format;
            device->CreateUnorderedAccessView(m_shadingRateImageSparseLighting.Get(), nullptr, &uavDescTex2D, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::ShadingRateImage2x2_UAV));

            D3D12_RESOURCE_ALLOCATION_INFO info = device->GetResourceAllocationInfo(0, 1, &desc);
            m_shadingRateImageSparseLightingSizeBytes = UINT(info.SizeInBytes);
        }
    }
    // sparse lighting resources
    {
        UINT tileSize = 16;  // this has nothing to do with VRS, 16 seems to be a sweet point and allows us to use 
        m_sparseLightingCountMapWidth = AlignUp(width, tileSize) / tileSize;
        m_sparseLightingCountMapHeight = AlignUp(height, tileSize) / tileSize;
        {
            // using a full uint32_t for count is a bit wasteful, a byte would do the job, however we want scalar loads which required use of a buffer
            UINT numElements = m_sparseLightingCountMapWidth * m_sparseLightingCountMapHeight;
            UINT stride = sizeof(uint32_t);
            UINT countBufferSize = numElements * stride;
            auto desc = CD3DX12_RESOURCE_DESC::Buffer(countBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_sparseLightingCountMap.GetAddressOf())
            ));
            {
                m_sparseLightingCountMap->SetName(L"m_sparseLightingCountMap");

                srvDescBuffer.Format = DXGI_FORMAT_UNKNOWN;
                srvDescBuffer.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDescBuffer.Buffer.NumElements = numElements;
                srvDescBuffer.Buffer.StructureByteStride = stride;
                device->CreateShaderResourceView(m_sparseLightingCountMap.Get(), &srvDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::SparseLightingCountSRV));

                uavDescBuffer.Format = DXGI_FORMAT_UNKNOWN;
                uavDescBuffer.Buffer.NumElements = numElements;
                uavDescBuffer.Buffer.StructureByteStride = stride;
                device->CreateUnorderedAccessView(m_sparseLightingCountMap.Get(), nullptr, &uavDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::SparseLightingCountUAV));
            }
        }
        {
            /*
                (256-32) = 224 entries per tile

                This is a 1/8th memory saving (~1.75mb at 1440p) vs having memory for 256 coordinates per 16x16 tile. Since if we have
                more than 224 coordinates, we are not saving any wave execution, so we light every pixel and don't read any coordinates
            */
            UINT32 numCoords = m_sparseLightingCountMapWidth * m_sparseLightingCountMapHeight * 224;

            auto desc = CD3DX12_RESOURCE_DESC::Buffer(numCoords * sizeof(UINT32), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_sparseLightingCoordBuffer.GetAddressOf())));
            {
                m_sparseLightingCoordBuffer->SetName(L"m_sparseLightingCoordBuffer");

                srvDescBuffer.Format = DXGI_FORMAT_R32_UINT;
                srvDescBuffer.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDescBuffer.Buffer.NumElements = numCoords;
                srvDescBuffer.Buffer.StructureByteStride = 0;
                device->CreateShaderResourceView(m_sparseLightingCoordBuffer.Get(), &srvDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::SparseLightingCoordsSRV));

                uavDescBuffer.Format = DXGI_FORMAT_R32_UINT;
                uavDescBuffer.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDescBuffer.Buffer.NumElements = numCoords;
                uavDescBuffer.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                uavDescBuffer.Buffer.StructureByteStride = 0;
                device->CreateUnorderedAccessView(m_sparseLightingCoordBuffer.Get(), nullptr, &uavDescBuffer, m_resourceDescriptorsCBV_SRV_UAV->GetCpuHandle(Descriptors::SparseLightingCoordsUAV));
            }
        }
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

            // Sky
            {
                EffectPipelineStateDescription pd(nullptr,
                    CommonStates::Opaque,
                    CommonStates::DepthReverseZ,
                    CommonStates::CullClockwise,
                    rtState);

                auto vsBlob = DX::ReadData(L"FullScreenTriangleVS.cso");
                auto psBlob = DX::ReadData(L"SkyRenderPS.cso");
                D3D12_SHADER_BYTECODE vs = { vsBlob.data(), vsBlob.size() };
                D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_skyRenderPSO.ReleaseAndGetAddressOf());
                m_skyRenderPSO->SetName(L"m_skyRenderPSO");
            }
            // Terrain
            {
                // G-buffer formats
                rtState.numRenderTargets = 3;
                rtState.rtvFormats[0] = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
                rtState.rtvFormats[1] = DXGI_FORMAT_R10G10B10A2_UNORM;
                rtState.rtvFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;

                EffectPipelineStateDescription pd(nullptr,
                    CommonStates::Opaque,
                    CommonStates::DepthReverseZ,
                    CommonStates::CullClockwise,
                    rtState);

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
            }
        }
    }
    // Visibility culling
    BuildComputePSO(device, m_terrainVisibilityPSO, L"TerrainVisibilityCS.cso", m_globalRS.Get());

    // Depth Decompress
    BuildComputePSO(device, m_decompressDepthPSO, L"DepthDecompress.cso", m_globalRS.Get());
    BuildComputePSO(device, m_buildSparseBuffersPSO, L"BuildSparseBuffersCS.cso", m_globalRS.Get());
    BuildComputePSO(device, m_decompressDepthAndBuildSparseBuffersPSO, L"DepthDecompressAndBuildSparseBuffersCS.cso", m_globalRS.Get());

    // Deferred lighting
    BuildComputePSO(device, m_deferredLightingPSO, L"DeferredLightingCS.cso", m_globalRS.Get());
//    BuildComputePSO(device, m_sparseLightingPSO, L"SSAO.cso", m_globalRS.Get());
    BuildComputePSO(device, m_sparseLightingPSO, L"SparseLightingCS.cso", m_globalRS.Get());

    // Calculate shading rate
    BuildComputePSO(device, m_shadingRatePSO, L"GenerateShadingRateCS.cso", m_globalRS.Get());
    BuildComputePSO(device, m_shadingRateDepthDiscontinuityCheckPSO, L"GenerateShadingRateDepthDiscontinuityCheckCS.cso", m_globalRS.Get());

    // Sparse Lighting Debug
    BuildComputePSO(device, m_sparseLightingShowPixelCopiesPSO, L"SparseLightingShowPixelCopiesCS.cso", m_globalRS.Get());

    // Post processing
    BuildComputePSO(device, m_toneMapPSO, L"ToneMapCS.cso", m_globalRS.Get());
    BuildComputePSO(device, m_deblockPSO, L"DeblockCS.cso", m_globalRS.Get());
    BuildComputePSO(device, m_deblockAndToneMapPSO, L"DeblockAndTonemapCS.cso", m_globalRS.Get());    
    
    // Copy to back buffer with optional zoom
    // unfortunately we need two of these, one for each UAV. The validation layer cannot detect that we are not accessing the UAV for the other back buffer thats in present state
    BuildComputePSO(device, m_zoomCopy0PSO, L"ZoomCopy0CS.cso", m_globalRS.Get());
    BuildComputePSO(device, m_zoomCopy1PSO, L"ZoomCopy1CS.cso", m_globalRS.Get());
    
    // Debug Shaders
    {
        RenderTargetState rtState(deviceResources->GetBackBufferFormat(), deviceResources->GetDepthBufferFormat());
        {
            auto vsBlob = DX::ReadData(L"FullScreenTriangleVS.cso");
            D3D12_SHADER_BYTECODE vs = { vsBlob.data(), vsBlob.size() };

            // Shading rate debug
            {
                EffectPipelineStateDescription pd(nullptr,
                    CommonStates::AlphaBlend,
                    CommonStates::DepthNone,
                    CommonStates::CullClockwise,
                    rtState);

                {
                    auto psBlob = DX::ReadData(L"VisualizeShadingRatePS.cso");
                    D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                    pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_visualizeShadingRatePSO.ReleaseAndGetAddressOf());
                    m_visualizeShadingRatePSO->SetName(L"m_visualizeShadingRatePSO");
                }
                {
                    auto psBlob = DX::ReadData(L"VisualizeCoveragePS.cso");
                    D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                    pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_visualizeCoveragePSO.ReleaseAndGetAddressOf());
                    m_visualizeCoveragePSO->SetName(L"m_visualizeCoveragePSO");
                }
            }
            // Count map debug
            {
                EffectPipelineStateDescription pd(nullptr,
                    CommonStates::Opaque,
                    CommonStates::DepthNone,
                    CommonStates::CullClockwise,
                    rtState);

                auto psBlob = DX::ReadData(L"SparseLightingShowCountMapPS.cso");
                D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
                pd.CreatePipelineState(device, m_globalRS.Get(), vs, ps, m_visualizeCountMapPSO.ReleaseAndGetAddressOf());
                m_visualizeShadingRatePSO->SetName(L"m_visualizeShadingRatePSO");
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

            auto drawIndexedArguments = static_cast<D3D12_DRAW_INDEXED_ARGUMENTS*>(drawIndexedArgumentsUpload.Memory());

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

    m_firstFrame = true;
    m_frameCount = 0;
}

