//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Terrain.h"
#include "CommonStates.h"
#include "EffectPipelineStateDescription.h"
#include "ReadData.h"


#define BUILD_WRITE_CODE_COPY_H                             1u
#define BUILD_WRITE_CODE_COPY_V                             2u
#define BUILD_WRITE_CODE_COPY_D                             4u
#define BUILD_WRITE_CODE_WRITE_PIXEL                        8u

// build nibble for a pixel
#define BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD)        (BUILD_WRITE_CODE_WRITE_PIXEL | ((copyH) ? BUILD_WRITE_CODE_COPY_H: 0) | ((copyV) ? BUILD_WRITE_CODE_COPY_V: 0) | ((copyD) ? BUILD_WRITE_CODE_COPY_D: 0))

// build nibble for a pixel
#define BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD)        (BUILD_WRITE_CODE_WRITE_PIXEL | ((copyH) ? BUILD_WRITE_CODE_COPY_H: 0) | ((copyV) ? BUILD_WRITE_CODE_COPY_V: 0) | ((copyD) ? BUILD_WRITE_CODE_COPY_D: 0))

// build nibbles for 2x2 pixels
#define BUILD_WRITE_CODE_VERT_SHIFT                         4u
#define BUILD_WRITE_CODE_HORZ_SHIFT                         8u
#define BUILD_WRITE_CODE_DIAG_SHIFT                         (BUILD_WRITE_CODE_VERT_SHIFT | BUILD_WRITE_CODE_HORZ_SHIFT) // == 12

#define BUILD_WRITE_CODE_NIBBLE_00(copyH, copyV, copyD)     (BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD))
#define BUILD_WRITE_CODE_NIBBLE_01(copyH, copyV, copyD)     (BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD) << BUILD_WRITE_CODE_VERT_SHIFT)
#define BUILD_WRITE_CODE_NIBBLE_10(copyH, copyV, copyD)     (BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD) << BUILD_WRITE_CODE_HORZ_SHIFT)
#define BUILD_WRITE_CODE_NIBBLE_11(copyH, copyV, copyD)     (BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD) << BUILD_WRITE_CODE_DIAG_SHIFT) 


using namespace DirectX;

#pragma warning(disable : 4061)


void NormalizePlane(float plane[4])
{
    float invMag = 1.0f / sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
    plane[0] = plane[0] * invMag;
    plane[1] = plane[1] * invMag;
    plane[2] = plane[2] * invMag;
    plane[3] = plane[3] * invMag;
}


void ExtractFrustumPlanes(Frustum &frustum, const XMFLOAT4X4 &vp)
{
    // There is a small performance advantage for ordering planes by most likely to cull to least likely
    // left, right and near planes typically create a useful half space, button and particularly top, less so
    
    // Left clipping plane
    frustum.planes[0][0] = vp._14 + vp._11;
    frustum.planes[0][1] = vp._24 + vp._21;
    frustum.planes[0][2] = vp._34 + vp._31;
    frustum.planes[0][3] = vp._44 + vp._41;
    // Right clipping plane
    frustum.planes[1][0] = vp._14 - vp._11;
    frustum.planes[1][1] = vp._24 - vp._21;
    frustum.planes[1][2] = vp._34 - vp._31;
    frustum.planes[1][3] = vp._44 - vp._41;
    // Near clipping plane
    frustum.planes[2][0] = vp._13;
    frustum.planes[2][1] = vp._23;
    frustum.planes[2][2] = vp._33;
    frustum.planes[2][3] = vp._43;
    // Bottom clipping plane
    frustum.planes[3][0] = vp._14 + vp._12;
    frustum.planes[3][1] = vp._24 + vp._22;
    frustum.planes[3][2] = vp._34 + vp._32;
    frustum.planes[3][3] = vp._44 + vp._42;
    // Top clipping plane
    frustum.planes[4][0] = vp._14 - vp._12;
    frustum.planes[4][1] = vp._24 - vp._22;
    frustum.planes[4][2] = vp._34 - vp._32;
    frustum.planes[4][3] = vp._44 - vp._42;

    NormalizePlane(frustum.planes[0]);
    NormalizePlane(frustum.planes[1]);
    NormalizePlane(frustum.planes[2]);
    NormalizePlane(frustum.planes[3]);
    NormalizePlane(frustum.planes[4]);
}



void Terrain::SwitchToCompute(std::unique_ptr<DX::DeviceResources> &deviceResources, GraphicsResource &cb)
{
    auto commandList = deviceResources->GetCommandList();
    commandList->SetComputeRootSignature(m_globalRS.Get());
    commandList->SetComputeRootConstantBufferView(TerrainRootSignature::CB0, cb.GpuAddress());
    commandList->SetComputeRootDescriptorTable(TerrainRootSignature::UAVs, m_resourceDescriptorsCBV_SRV_UAV->GetGpuHandle(Descriptors::SourceHeightMapUAV0));
    commandList->SetComputeRootDescriptorTable(TerrainRootSignature::SRVs, m_resourceDescriptorsCBV_SRV_UAV->GetGpuHandle(Descriptors::SourceHeightMapSRV0));
}



void Terrain::SwitchToGraphics(std::unique_ptr<DX::DeviceResources> &deviceResources, GraphicsResource &cb)
{
    auto commandList = deviceResources->GetCommandList();
    commandList->SetGraphicsRootSignature(m_globalRS.Get());
    commandList->SetGraphicsRootConstantBufferView(TerrainRootSignature::CB0, cb.GpuAddress());
    commandList->SetGraphicsRootDescriptorTable(TerrainRootSignature::UAVs, m_resourceDescriptorsCBV_SRV_UAV->GetGpuHandle(Descriptors::SourceHeightMapUAV0));
    commandList->SetGraphicsRootDescriptorTable(TerrainRootSignature::SRVs, m_resourceDescriptorsCBV_SRV_UAV->GetGpuHandle(Descriptors::SourceHeightMapSRV0));
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}



void Terrain::RenderTerrain(ID3D12GraphicsCommandList *commandList, UINT32 frameIndex, DX::GPUCommandListTimer<ID3D12GraphicsCommandList> &gpuTimer)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Terrain Render");
    gpuTimer.Start(commandList, GPUPasses::GBuffer);

    commandList->SetPipelineState(m_terrainRenderPSO.Get());
    commandList->IASetIndexBuffer(&m_indexBufferView[LODs::High]);
    commandList->SetGraphicsRoot32BitConstant(TerrainRootSignature::CB1, 3, 0);  // select high res instance buffer
    commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_drawIndexedInstancedArgsBuffer.Get(), (6 + frameIndex) * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), nullptr, 0);

    commandList->IASetIndexBuffer(&m_indexBufferView[LODs::Medium]);
    commandList->SetGraphicsRoot32BitConstant(TerrainRootSignature::CB1, 2, 0);  // select medium res instance buffer
    commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_drawIndexedInstancedArgsBuffer.Get(), (4 + frameIndex) * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), nullptr, 0);

    commandList->IASetIndexBuffer(&m_indexBufferView[LODs::Low]);
    commandList->SetGraphicsRoot32BitConstant(TerrainRootSignature::CB1, 1, 0);  // select low res instance buffer
    commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_drawIndexedInstancedArgsBuffer.Get(), (2 + frameIndex) * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), nullptr, 0);

    commandList->IASetIndexBuffer(&m_indexBufferView[LODs::VeryLow]);
    commandList->SetGraphicsRoot32BitConstant(TerrainRootSignature::CB1, 0, 0);  // select very low res instance buffer
    commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_drawIndexedInstancedArgsBuffer.Get(), (0 + frameIndex) * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), nullptr, 0);

    gpuTimer.Stop(commandList, GPUPasses::GBuffer);
    PIXEndEvent(commandList);
}


void Terrain::DeferredLight(std::unique_ptr<DX::DeviceResources>& deviceResources, GraphicsResource& cb, DX::GPUCommandListTimer<ID3D12GraphicsCommandList>& gpuTimer, bool sparseLightingEnabled)
{
    auto commandList = deviceResources->GetCommandList();
    SwitchToCompute(deviceResources, cb);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Deferred Lighting");
    gpuTimer.Start(commandList, GPUPasses::DeferredLighting);

    if (sparseLightingEnabled)
        commandList->SetPipelineState(m_sparseLightingPSO.Get());
    else
        commandList->SetPipelineState(m_deferredLightingPSO.Get());

    // using wave32 with 8x4 wave size
    UINT dispatchX = AlignUp(UINT(deviceResources->GetScissorRect().right), 8);
    UINT dispatchY = AlignUp(UINT(deviceResources->GetScissorRect().bottom), 4);

    // arguments are deliberately the wrong way round, see shader for details of why
    commandList->Dispatch(dispatchY / 4, dispatchX / 8, 1);

    gpuTimer.Stop(commandList, GPUPasses::DeferredLighting);
    PIXEndEvent(commandList);

    SwitchToGraphics(deviceResources, cb);
}


void Terrain::DecompressDepthAndBuildSparseBuffers(std::unique_ptr<DX::DeviceResources>& deviceResources, GraphicsResource& cb, DX::GPUCommandListTimer<ID3D12GraphicsCommandList>& gpuTimer, bool sparseLightingEnabled, Terrain::SparseBufferGeneration::Enum sparseBufferGeneration, Terrain::SparseLightingTileSize::Enum sparseLightingTileSize)
{
    auto commandList = deviceResources->GetCommandList();
    SwitchToCompute(deviceResources, cb);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Depth Decompress and Sparse Coordinate Generation");
    gpuTimer.Start(commandList, GPUPasses::DepthDecompressAndSparseBufferGeneration);

    UINT dispatchX = AlignUp(UINT(deviceResources->GetScissorRect().right), 16) / 16;
    UINT dispatchY = AlignUp(UINT(deviceResources->GetScissorRect().bottom), 16) / 16;

    if (sparseLightingEnabled)
    {
        commandList->SetComputeRoot32BitConstant(TerrainRootSignature::CB1, UINT(sparseLightingTileSize == SparseLightingTileSize::TileSize2x2 ? 1 : 0), 0);

        if (sparseBufferGeneration == SparseBufferGeneration::CombinedDepthDecompress)
        {
            commandList->SetPipelineState(m_decompressDepthAndBuildSparseBuffersPSO.Get());
        }
        else
        {
            commandList->SetPipelineState(m_decompressDepthPSO.Get());

            commandList->Dispatch(dispatchX, dispatchY, 1);
            {
                D3D12_RESOURCE_BARRIER barrier[1];
                QuickBarrier(barrier[0], m_LinearDepth.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
            commandList->SetPipelineState(m_buildSparseBuffersPSO.Get());
        }
    }
    else
    {
        commandList->SetPipelineState(m_decompressDepthPSO.Get());
    }
    commandList->Dispatch(dispatchX, dispatchY, 1);

    gpuTimer.Stop(commandList, GPUPasses::DepthDecompressAndSparseBufferGeneration);
    PIXEndEvent(commandList);

    SwitchToGraphics(deviceResources, cb);
}


void Terrain::RenderSky(ID3D12GraphicsCommandList* commandList, DX::GPUCommandListTimer<ID3D12GraphicsCommandList>& gpuTimer)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Sky Render");
    gpuTimer.Start(commandList, GPUPasses::Sky);
    commandList->SetPipelineState(m_skyRenderPSO.Get());
    commandList->IASetIndexBuffer(nullptr);
    commandList->DrawInstanced(3, 1, 0, 0);
    gpuTimer.Stop(commandList, GPUPasses::Sky);
    PIXEndEvent(commandList);
}


void Terrain::ToneMap(std::unique_ptr<DX::DeviceResources>& deviceResources, GraphicsResource& cb, Terrain::DeblockerOptions::Enum deblockerOptions, DX::GPUCommandListTimer<ID3D12GraphicsCommandList>& gpuTimer)
{
    auto commandList = deviceResources->GetCommandList();
    SwitchToCompute(deviceResources, cb);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Post Processing");
    
    if (deblockerOptions == DeblockerOptions::DeblockAndToneMap)
    {
        gpuTimer.Start(commandList, GPUPasses::ToneMap);
        commandList->SetPipelineState(m_deblockAndToneMapPSO.Get());
        UINT dispatchX = AlignUp(UINT(deviceResources->GetScissorRect().right), 16);
        UINT dispatchY = AlignUp(UINT(deviceResources->GetScissorRect().bottom), 16);

        commandList->Dispatch(dispatchX / 16, dispatchY / 16, 1);
        gpuTimer.Stop(commandList, GPUPasses::ToneMap);
    }
    else
    {
        UINT toneMapSRVReadIndex = 0;       // set tonemap to read from LitOutputSRV

        if (deblockerOptions == DeblockerOptions::DeblockStandAlonePass)
        {
            UINT dispatchX = AlignUp(UINT(deviceResources->GetScissorRect().right), 16);
            UINT dispatchY = AlignUp(UINT(deviceResources->GetScissorRect().bottom), 16);

            gpuTimer.Start(commandList, GPUPasses::Deblocker);
            commandList->SetPipelineState(m_deblockPSO.Get());
            commandList->Dispatch(dispatchX / 16, dispatchY / 16, 1);
            gpuTimer.Stop(commandList, GPUPasses::Deblocker);
                        
            toneMapSRVReadIndex = 1;        // set tonemap to read from PostProcessSRV, deblocker cannot write to the target its reading (tonemapping can)
        }
        {
            UINT dispatchX = AlignUp(UINT(deviceResources->GetScissorRect().right), 8);
            UINT dispatchY = AlignUp(UINT(deviceResources->GetScissorRect().bottom), 4);

            gpuTimer.Start(commandList, GPUPasses::ToneMap);
            commandList->SetPipelineState(m_toneMapPSO.Get());
            commandList->SetComputeRoot32BitConstant(TerrainRootSignature::CB1, toneMapSRVReadIndex, 0);
            commandList->Dispatch(dispatchX / 8, dispatchY / 4, 1);
            gpuTimer.Stop(commandList, GPUPasses::ToneMap);
        }
    }
    PIXEndEvent(commandList);

    SwitchToGraphics(deviceResources, cb);
}


void Terrain::CalculateShadingRate(std::unique_ptr<DX::DeviceResources> &deviceResources, GraphicsResource &cb, DX::GPUCommandListTimer<ID3D12GraphicsCommandList> &gpuTimer, bool VRSenabled, bool generateSparseLighting2x2TileSize, bool depthDiscontinuityCheck)
{
    auto commandList = deviceResources->GetCommandList();
    SwitchToCompute(deviceResources, cb);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"VRS Calc Shading Rate");
    gpuTimer.Start(commandList, GPUPasses::CalcShadingRate);
    {
        D3D12_RESOURCE_BARRIER barrier[2];
        QuickBarrier(barrier[0], m_shadingRateImageVRS.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[1], m_shadingRateImageSparseLighting.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }
    // Set which image(s) we're going to generate
    bool drsResolutionChangeThisFrame = false;
    UINT rootConstantValue = UINT(VRSenabled ? 1 : 0);
    rootConstantValue |= UINT(generateSparseLighting2x2TileSize ? 2 : 0);

    commandList->SetComputeRoot32BitConstant(TerrainRootSignature::CB1, rootConstantValue, 0);

    // don't process 16 pixel border around edge of the screen (safe area)
    // the left and top edges are memset to 2x2 rate just once and never change, the right and bottom need to write 2x2 only on DRS resolution change
    // compute the bottom right corner beyond which we do not have to calculate the shading rate, but just write 2x2 rate
    UINT tileSize = 16;
    UINT borderSizeToRemoveFromDispatch = tileSize * UINT(drsResolutionChangeThisFrame ? 1 : 2); // process right and bottom on resolution change, as they need clearing back to 2x2 rate
    UINT dispatchX = AlignUp(UINT(deviceResources->GetScissorRect().right), tileSize) - borderSizeToRemoveFromDispatch;
    UINT dispatchY = AlignUp(UINT(deviceResources->GetScissorRect().bottom), tileSize) - borderSizeToRemoveFromDispatch;

    if (depthDiscontinuityCheck)
        commandList->SetPipelineState(m_shadingRateDepthDiscontinuityCheckPSO.Get());
    else
        commandList->SetPipelineState(m_shadingRatePSO.Get());

    UINT tgSize = 8;
    commandList->Dispatch(dispatchX / tgSize, dispatchY / tgSize, 1);
    {
        D3D12_RESOURCE_BARRIER barrier[2];
        QuickBarrier(barrier[0], m_shadingRateImageVRS.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
        QuickBarrier(barrier[1], m_shadingRateImageSparseLighting.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }
    gpuTimer.Stop(commandList, GPUPasses::CalcShadingRate);
    PIXEndEvent(commandList);

    SwitchToGraphics(deviceResources, cb);
}


void Terrain::RenderScene(std::unique_ptr<DX::DeviceResources>& deviceResources, GraphicsResource& cb, UINT32 frameIndex, DX::GPUCommandListTimer<ID3D12GraphicsCommandList>& gpuTimer, bool VRSenabled, bool sparseLightingEnabled, Terrain::SparseBufferGeneration::Enum sparseBufferGeneration, Terrain::SparseLightingTileSize::Enum sparseLightingTileSize, bool depthDiscontinuityCheck, Terrain::DeblockerOptions::Enum deblockerOptions)
{
    auto commandList = deviceResources->GetCommandList();
    auto dsvDescriptor = deviceResources->GetDepthStencilView();

    // render g-buffers
    {
        D3D12_RESOURCE_BARRIER barrier[3];
        QuickBarrier(barrier[0], m_GbufferAlbedo.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        QuickBarrier(barrier[1], m_GbufferNormalRoughness.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        QuickBarrier(barrier[2], m_GbufferDebugAndShadingRate.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = m_resourceDescriptorRTV->GetCpuHandle(RenderTargets::GBufferAlbedo);
        commandList->OMSetRenderTargets(3, &rtvDescriptor, true, &dsvDescriptor);
    }
    gpuTimer.Start(commandList, GPUPasses::ClearDepth);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
    gpuTimer.Stop(commandList, GPUPasses::ClearDepth);

    // Set Shading Rate Image, this has a cost
    if (VRSenabled)
    {
        gpuTimer.Start(commandList, GPUPasses::VRSSetShadingRate);
        D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT] =
        {
            D3D12_SHADING_RATE_COMBINER_OVERRIDE,       // pass through is the render state, override is the provoking vertex
            D3D12_SHADING_RATE_COMBINER_OVERRIDE        // take the shading rate map, ignore the previous combiner
        };
        commandList->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
        commandList->RSSetShadingRateImage(m_shadingRateImageVRS.Get());
        gpuTimer.Stop(commandList, GPUPasses::VRSSetShadingRate);
    }
    RenderTerrain(commandList, frameIndex, gpuTimer);

    bool shaderReadsSRI = sparseLightingEnabled && (sparseBufferGeneration != SparseBufferGeneration::CombinedDepthDecompress);
    {
        D3D12_RESOURCE_BARRIER barrier[8];
        QuickBarrier(barrier[0], m_GbufferAlbedo.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        QuickBarrier(barrier[1], m_GbufferNormalRoughness.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        QuickBarrier(barrier[2], m_GbufferDebugAndShadingRate.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        QuickBarrier(barrier[3], m_LinearDepth.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[4], deviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL);
        QuickBarrier(barrier[5], m_sparseLightingCountMap.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[6], m_sparseLightingCoordBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        UINT numBarries = 7;
        if (shaderReadsSRI)
        {
            QuickBarrier(barrier[numBarries], m_shadingRateImageVRS.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            ++numBarries;
        }
        commandList->ResourceBarrier(numBarries, barrier);
    }
    DecompressDepthAndBuildSparseBuffers(deviceResources, cb, gpuTimer, sparseLightingEnabled, sparseBufferGeneration, sparseLightingTileSize);
    {
        D3D12_RESOURCE_BARRIER barrier[6];
        QuickBarrier(barrier[0], deviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        QuickBarrier(barrier[1], m_sparseLightingCountMap.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        QuickBarrier(barrier[2], m_sparseLightingCoordBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        QuickBarrier(barrier[3], m_litOutput.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        UINT numBarriers = 4;

        if (shaderReadsSRI)
        {
            QuickBarrier(barrier[numBarriers], m_shadingRateImageVRS.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
            ++numBarriers;
        }
        if (sparseLightingEnabled && (sparseBufferGeneration == SparseBufferGeneration::StandAlone))
        {
            // linear depth had to be transitioned between depth decompress and building sparse buffers
        }
        else
        {
            QuickBarrier(barrier[numBarriers], m_LinearDepth.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            ++numBarriers;
        }
        commandList->ResourceBarrier(numBarriers, barrier);
    }
    DeferredLight(deviceResources, cb, gpuTimer, sparseLightingEnabled);
    {
        D3D12_RESOURCE_BARRIER barrier[1];
        QuickBarrier(barrier[0], m_litOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = m_resourceDescriptorRTV->GetCpuHandle(RenderTargets::LitOutput);
        commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);
    }
    // just 2x2 the sky, doesn't save much since the sky generates 2x2 rate in the image anyway,
    // but we do get a small amount of 1x1 rate where the sky meets the horizon
    if (VRSenabled) 
    {
        D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT] =
        {
            D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,    // pass through is the render state, override is the provoking vertex
            D3D12_SHADING_RATE_COMBINER_PASSTHROUGH     // take the result of the previous combiner
        };
        commandList->RSSetShadingRate(D3D12_SHADING_RATE_2X2, combiners);
        commandList->RSSetShadingRateImage(nullptr);
    }
    RenderSky(commandList, gpuTimer);
    {
        D3D12_RESOURCE_BARRIER barrier[2];
        QuickBarrier(barrier[0], m_litOutput.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        QuickBarrier(barrier[1], m_postProcessOutput.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }
    if (VRSenabled)
    {
        D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT] =
        {
            D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,    // pass through is the render state, override is the provoking vertex
            D3D12_SHADING_RATE_COMBINER_PASSTHROUGH     // take the result of the previous combiner
        };
        commandList->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
        commandList->RSSetShadingRateImage(nullptr);
    }
    ToneMap(deviceResources, cb, deblockerOptions, gpuTimer);
    {
        D3D12_RESOURCE_BARRIER barrier[1];
        QuickBarrier(barrier[0], m_postProcessOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }
    if (VRSenabled || sparseLightingEnabled)
    {
        CalculateShadingRate(deviceResources, cb, gpuTimer, VRSenabled, sparseLightingEnabled && (sparseLightingTileSize == SparseLightingTileSize::TileSize2x2), depthDiscontinuityCheck);
    }
}



void Terrain::Render(
    const XMFLOAT4X4 &proj,
    const XMFLOAT4X4 &view,
    const XMFLOAT4X4 &viewProj,
    float farZ,
    float depthRange,
    float depthRangeNegNearZ,
    const XMFLOAT3& position,
    std::unique_ptr<DX::DeviceResources> &deviceResources,
    std::unique_ptr<GraphicsMemory> &graphicsMemory,
    const float elapsedTimeSeconds,
    DX::GPUCommandListTimer<ID3D12GraphicsCommandList> &gpuTimer,
    float shadingRateTolerance,
    float sunHeight,
    bool VRSenabled,
    bool sparseLightingEnabled,
    Terrain::SparseBufferGeneration::Enum sparseBufferGeneration,
    Terrain::SparseLightingTileSize::Enum sparseLightingTileSize,
    bool depthDiscontinuityCheck,
    Terrain::DeblockerOptions::Enum deblockerOptions,
    Terrain::RotateLitPixelOptions::Enum rotateLitPixelOptions,
    Terrain::Visualise::Enum debugVisualisation,
    Terrain::ZoomLevel::Enum zoomLevel
    )
{
    UINT showSparseLighting2x2Tiles = UINT(((debugVisualisation == Visualise::SparseLightingShadingRateOverlay) && (sparseLightingTileSize == SparseLightingTileSize::TileSize2x2)) ? 1 : 0);

    // Setup per-frame constant buffer
    GraphicsResource cbPerFrame;
    {
        float worldScale = 0.3f;
        float worldScaleY = 25.0f * 3.0f;
        float xzTranslation = (-0.5f * float(m_heightMapSize)) * worldScale;

        Constants cb = {};

        // terrain
        cb.invSourceHeightMapSize = 1.0f / float(m_sourceHeightMapSize);
        cb.invHeightMapSize = 1.0f / float(m_heightMapSize);
        cb.terrainTileSizeLog2 = m_tileSizeLog2;
        cb.terrainTileCountPerAxis = m_tileCountPerAxis;
        cb.frameIndex = deviceResources->GetCurrentFrameIndex();
        cb.worldScaleY = worldScaleY;
        cb.worldScale = worldScale;
        cb.xzTranslation = xzTranslation;
        cb.halfTexelHeightMapSize = 1.0 / float(m_sourceHeightMapSize * 2.0);
        cb.sourceToHeightMapSizeMultiplier = 1.0f / float(1 << m_sourceToHeightMapSizeLog2);
        cb.sourceToHeightMapSizeLog2 = m_sourceToHeightMapSizeLog2;
        cb.worldToHeightMapCoordMul = cb.invHeightMapSize / worldScale;

        // camera
        XMMATRIX viewProjMatrix = XMLoadFloat4x4(&viewProj);
        XMMATRIX viewMatrix = XMLoadFloat4x4(&view);
        //XMMATRIX wvp = XMMatrixMultiply(world, viewProjMatrix);   // terrain has no world matrix as such
        cb.worldViewProjectionMatrix = XMMatrixTranspose(viewProjMatrix);

        // construction frustum points in view space
        const float farX = farZ / proj._11;
        const float farY = farZ / proj._22;

        XMVECTOR p1 = XMVectorSet(-farX, -farY, farZ, 1.0);        // Far lower left
        XMVECTOR p2 = XMVectorSet( farX, -farY, farZ, 1.0);        // Far lower right
        XMVECTOR p3 = XMVectorSet(-farX,  farY, farZ, 1.0);        // Far upper left

        // transform frustum to world space
        XMMATRIX invView = XMMatrixInverse(nullptr, viewMatrix);
        p1 = XMVector4Transform(p1, invView);
        p2 = XMVector4Transform(p2, invView);
        p3 = XMVector4Transform(p3, invView);

        XMVECTOR horizontalDelta = XMVectorSubtract(p2, p1);
        XMVECTOR verticalDelta = XMVectorSubtract(p1, p3);
        XMVECTOR origin = XMVectorSubtract(p3, XMLoadFloat3(&position));

        XMStoreFloat3(&cb.frustumHDelta, horizontalDelta);
        XMStoreFloat3(&cb.frustumVDelta, verticalDelta);
        XMStoreFloat3(&cb.frustumOrigin, origin);
        cb.cameraPos = position;

        XMFLOAT4X4 temp;
        XMStoreFloat4x4(&temp, viewProjMatrix);
        ExtractFrustumPlanes(cb.viewFrustum, temp);

        cb.htileInfo = m_HTileInfo;
        cb.depthRange = depthRange;
        cb.depthRangeNegNearZDivFarZ = depthRangeNegNearZ / farZ;
        cb.farZ = farZ;

        XMStoreFloat3(&cb.cameraForwardVector, XMMatrixTranspose(viewMatrix).r[2]);

        cb.renderTargetDimX = UINT32(deviceResources->GetScissorRect().right);
        cb.renderTargetDimY = UINT32(deviceResources->GetScissorRect().bottom);
        cb.sparseLightingBufferWidth = AlignUp(UINT(deviceResources->GetScissorRect().right), 16) / 16;
        cb.sparseLightingBufferHeight = AlignUp(UINT(deviceResources->GetScissorRect().bottom), 16) / 16;

        // pixel.xy to UV
        float rtWidth = float(deviceResources->GetScissorRect().right);
        float rtHeight = float(deviceResources->GetScissorRect().bottom);

        cb.invRenderTargetDimX = 1.0f / rtWidth;
        cb.invRenderTargetDimY = 1.0f / rtHeight;
        cb.renderTargetHalfPixelOffsetX = 0.5f * cb.invRenderTargetDimX;
        cb.renderTargetHalfPixelOffsetY = 0.5f * cb.invRenderTargetDimY;

        // don't process 16 pixel border around edge of the screen (safe area)
        // the left and top edges are memset to 2x2 rate just once and never change, the right and bottom need to write 2x2 only on DRS resolution change
        // compute the bottom right corner beyond which we do not have to calculate the shading rate, but just write 2x2 rate
        UINT tileSize = 16;
        UINT tgSize = 8;
        cb.vrsRightTileSize  = (AlignUp(UINT(deviceResources->GetScissorRect().right ), tileSize) - tileSize) / tgSize;
        cb.vrsBottomTileSize = (AlignUp(UINT(deviceResources->GetScissorRect().bottom), tileSize) - tileSize) / tgSize;

        // for ray marching
        static float lightTime = XM_PI * 1.1f;
        lightTime += elapsedTimeSeconds * 0.025f;
        XMVECTOR lightDir = XMVectorSet(sinf(lightTime), sunHeight, cosf(lightTime), 1.0f);
        lightDir = XMVector3Normalize(lightDir);
        XMStoreFloat3(&cb.lightDir, lightDir);
        float invLengthXZ = (cb.lightDir.y != 1.0f) ? 1.0f / sqrtf((cb.lightDir.x * cb.lightDir.x) + (cb.lightDir.z * cb.lightDir.z)) : 1.0f;
        cb.rayMarchLightDir.x = cb.lightDir.x * invLengthXZ;
        cb.rayMarchLightDir.y = cb.lightDir.y * invLengthXZ;
        cb.rayMarchLightDir.z = cb.lightDir.z * invLengthXZ;
        cb.stepSize = lerp(0.5f, 5.0f, cb.lightDir.y * cb.lightDir.y);    // higher step size the lower the angle of the sun, needed to capture small details
        cb.lowResMaxHeightMapSize = float(m_lowResMaxHeightSize);
 
        // shading rate generation 
        cb.shadingRateTolerance = shadingRateTolerance;

        // sparse lighting frame rotation
        cb.sparseLightingFrameIndex = 0;

        switch (rotateLitPixelOptions)
        {
        default:
        case RotateLitPixelOptions::Off:
            break;

        case RotateLitPixelOptions::On:
            cb.sparseLightingFrameIndex = m_frameCount;
            break;

        case RotateLitPixelOptions::OncePerSecond:
            cb.sparseLightingFrameIndex = m_frameCount / 60;
            break;
        }
        cb.sparseLightingFrameIndex &= 3;

        // Period 4 lighting for L shapes where we have 1 pixel to light and two copies
        // Here we light the corner point every other frame, as this one has the least visual error when copied to the others (shorter distance than the diagonal)
        switch (cb.sparseLightingFrameIndex)
        {
            // Key:
            //  l is the lit pixel performing copies
            //  = is a pixel copy
            //  X is a pixel that must be uniquely lit (boo!), or is clear, not handled by the below copies
        case 0:
        case 2:
            // l=                                                                                                                                              
            // =X                                                                                                                                              
            cb.sparseLightingLCopyCodeTL = BUILD_WRITE_CODE_NIBBLE_00(true, true, false);         // top left

            // =l
            // X=
            cb.sparseLightingLCopyCodeTR = BUILD_WRITE_CODE_NIBBLE_10(true, true, false);         // top right

            // =X                                                                                                                                              
            // l=                                                                                                                                              
            cb.sparseLightingLCopyCodeBL = BUILD_WRITE_CODE_NIBBLE_01(true, true, false);         // bottom left

            // X=
            // =l
            cb.sparseLightingLCopyCodeBR = BUILD_WRITE_CODE_NIBBLE_11(true, true, false);         // bottom right
            break;

            // clockwise rotation of the above
        case 1:
            // =l                                                                                                                                              
            // =X                                                                                                                                              
            cb.sparseLightingLCopyCodeTL = BUILD_WRITE_CODE_NIBBLE_10(true, false, true);         // top left

            // ==
            // Xl
            cb.sparseLightingLCopyCodeTR = BUILD_WRITE_CODE_NIBBLE_11(false, true, true);         // top right

            // lX                                                                                                                                              
            // ==                                                                                                                                              
            cb.sparseLightingLCopyCodeBL = BUILD_WRITE_CODE_NIBBLE_00(false, true, true);         // bottom left

            // X=
            // l=
            cb.sparseLightingLCopyCodeBR = BUILD_WRITE_CODE_NIBBLE_01(true, false, true);         // bottom right
            break;

        case 3:
            // ==                                                                                                                                              
            // lX                                                                                                                                              
            cb.sparseLightingLCopyCodeTL = BUILD_WRITE_CODE_NIBBLE_01(false, true, true);         // top left

            // l=
            // X=
            cb.sparseLightingLCopyCodeTR = BUILD_WRITE_CODE_NIBBLE_00(true, false, true);         // top right

            // =X                                                                                                                                              
            // =l                                                                                                                                              
            cb.sparseLightingLCopyCodeBL = BUILD_WRITE_CODE_NIBBLE_11(true, false, true);         // bottom left

            // Xl
            // ==
            cb.sparseLightingLCopyCodeBR = BUILD_WRITE_CODE_NIBBLE_10(false, true, true);         // bottom right
            break;

        default:
            assert(0 && "Unhandled case");
            break;
        }
        // debug visualisation
        if (sparseLightingEnabled && (debugVisualisation == Visualise::LightingCountBuffer))
        {
            cb.debugSurfaceSizeFX = float(m_sparseLightingCountMapWidth);
            cb.debugSurfaceSizeFY = float(m_sparseLightingCountMapHeight);
            cb.debugSurfaceSizeUX = m_sparseLightingCountMapWidth;
            cb.debugSurfaceSizeUY = m_sparseLightingCountMapHeight;
        }
        else if (showSparseLighting2x2Tiles)
        {
            cb.debugSurfaceSizeFX = float(m_shadingRateImageSparseLightingWidth);
            cb.debugSurfaceSizeFY = float(m_shadingRateImageSparseLightingHeight);
            cb.debugSurfaceSizeUX = m_shadingRateImageSparseLightingWidth;
            cb.debugSurfaceSizeUY = m_shadingRateImageSparseLightingHeight;
        }
        else
        {
            cb.debugSurfaceSizeFX = float(m_shadingRateImageVRSWidth);
            cb.debugSurfaceSizeFY = float(m_shadingRateImageVRSHeight);
            cb.debugSurfaceSizeUX = m_shadingRateImageVRSWidth;
            cb.debugSurfaceSizeUY = m_shadingRateImageVRSHeight;
        }
        cbPerFrame = graphicsMemory->AllocateConstant(cb);
    }
    // ok ready for action (almost)
    auto commandList = deviceResources->GetCommandList();

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_resourceDescriptorsCBV_SRV_UAV->Heap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    SwitchToCompute(deviceResources, cbPerFrame);

    if (m_firstFrame)
    {
        GeneateTerrain(deviceResources);

        // set whole shading rate target to 2x2, including the edges
        UINT32 fillValue = (D3D12_SHADING_RATE_2X2 << 24) | (D3D12_SHADING_RATE_2X2 << 16) | (D3D12_SHADING_RATE_2X2 << 8) | D3D12_SHADING_RATE_2X2;
        commandList->FillMemoryWith32BitValueX(m_shadingRateImageVRS->GetGPUVirtualAddress(), m_shadingRateImageVRSSizeBytes, fillValue, D3D12XBOX_COPY_FLAG_NONE);
        commandList->FillMemoryWith32BitValueX(m_shadingRateImageSparseLighting->GetGPUVirtualAddress(), m_shadingRateImageSparseLightingSizeBytes, fillValue, D3D12XBOX_COPY_FLAG_NONE);
    }
    else
    {
        D3D12_RESOURCE_BARRIER barrier[5];
        QuickBarrier(barrier[0], m_instanceBuffer[LODs::VeryLow].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[1], m_instanceBuffer[LODs::Low].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[2], m_instanceBuffer[LODs::Medium].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[3], m_instanceBuffer[LODs::High].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[4], m_drawIndexedInstancedArgsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }
    // Visibility cull
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"m_terrainVisibilityPSO");
        {
            commandList->SetPipelineState(m_terrainVisibilityPSO.Get());
            commandList->Dispatch(m_tileCountPerAxis / 8, m_tileCountPerAxis / 8, 1);
        }
        {
            D3D12_RESOURCE_BARRIER barrier[5];
            QuickBarrier(barrier[0], m_instanceBuffer[LODs::VeryLow].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            QuickBarrier(barrier[1], m_instanceBuffer[LODs::Low].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            QuickBarrier(barrier[2], m_instanceBuffer[LODs::Medium].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            QuickBarrier(barrier[3], m_instanceBuffer[LODs::High].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            QuickBarrier(barrier[4], m_drawIndexedInstancedArgsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
        }
        PIXEndEvent(commandList);
    }
    // Rendering time!
    SwitchToGraphics(deviceResources, cbPerFrame);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Scene Render");
    RenderScene(deviceResources, cbPerFrame, deviceResources->GetCurrentFrameIndex(), gpuTimer, VRSenabled, sparseLightingEnabled, sparseBufferGeneration, sparseLightingTileSize, depthDiscontinuityCheck, deblockerOptions);
    PIXEndEvent(commandList);
        
    // copy to back buffer
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Debug & CopyToBackBuffer");
        commandList->IASetIndexBuffer(nullptr);

        if ((VRSenabled && ((debugVisualisation == Visualise::VRSShadingRateOverlay) || (debugVisualisation == Visualise::Coverage))) || (sparseLightingEnabled && (debugVisualisation == Visualise::SparseLightingShadingRateOverlay)))
        {
            {
                D3D12_RESOURCE_BARRIER barrier[2];
                QuickBarrier(barrier[0], m_shadingRateImageVRS.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                QuickBarrier(barrier[1], m_postProcessOutput.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
            // set back buffer
            auto dsvDescriptor = deviceResources->GetDepthStencilView();
            auto rtvDescriptor = m_resourceDescriptorRTV->GetCpuHandle(RenderTargets::PostProcessOutput);
            commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);
            commandList->SetGraphicsRoot32BitConstant(TerrainRootSignature::CB1, showSparseLighting2x2Tiles, 0);
            if(debugVisualisation == Visualise::Coverage)
                commandList->SetPipelineState(m_visualizeCoveragePSO.Get());
            else
                commandList->SetPipelineState(m_visualizeShadingRatePSO.Get());
            commandList->DrawInstanced(3, 1, 0, 0);
            {
                D3D12_RESOURCE_BARRIER barrier[2];
                QuickBarrier(barrier[0], m_shadingRateImageVRS.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
                QuickBarrier(barrier[1], m_postProcessOutput.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
        }
        if (sparseLightingEnabled && (debugVisualisation == Visualise::LightingCountBuffer))
        {
            {
                D3D12_RESOURCE_BARRIER barrier[1];
                QuickBarrier(barrier[0], m_postProcessOutput.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
            // set back buffer
            auto dsvDescriptor = deviceResources->GetDepthStencilView();
            auto rtvDescriptor = m_resourceDescriptorRTV->GetCpuHandle(RenderTargets::PostProcessOutput);
            commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);
            commandList->SetGraphicsRoot32BitConstant(TerrainRootSignature::CB1, showSparseLighting2x2Tiles, 0);
            commandList->SetPipelineState(m_visualizeCountMapPSO.Get());
            commandList->DrawInstanced(3, 1, 0, 0);
            {
                D3D12_RESOURCE_BARRIER barrier[1];
                QuickBarrier(barrier[0], m_postProcessOutput.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }            
        }
        if (sparseLightingEnabled && (debugVisualisation == Visualise::NoHoleFilling))
        {
            // we need to render holes black as a separate pass after shading rate calculation. Otherwise the shading rate calculation would promote everything to full rate with black holes
            {
                D3D12_RESOURCE_BARRIER barrier[1];
                QuickBarrier(barrier[0], m_postProcessOutput.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
            SwitchToCompute(deviceResources, cbPerFrame);

            commandList->SetPipelineState(m_sparseLightingShowPixelCopiesPSO.Get());

            // using wave32 with 8x4 wave size
            UINT dispatchX = AlignUp(UINT(deviceResources->GetScissorRect().right), 8);
            UINT dispatchY = AlignUp(UINT(deviceResources->GetScissorRect().bottom), 4);

            // arguments are deliberately the wrong way round, see shader for details of why
            commandList->Dispatch(dispatchY / 4, dispatchX / 8, 1);

            SwitchToGraphics(deviceResources, cbPerFrame);
            {
                D3D12_RESOURCE_BARRIER barrier[1];
                QuickBarrier(barrier[0], m_postProcessOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
        }
        // Copy with back buffer with optional zoom
        {
            {
                D3D12_RESOURCE_BARRIER barrier[1];
                QuickBarrier(barrier[0], deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
            SwitchToCompute(deviceResources, cbPerFrame);

            // The ideal solution is a UAV descriptor array for the back buffer, unfortunately the validation layer doesn't know which array element is actually being accessed, so it complains that the front buffer is in the present state, not knowing the shader will index the back buffer, not the front buffer
            // The validation error I get from using a single shader and a descriptor array are erroneous, but I didn't want to ship a sample with validation warnings disabled
            // So the workaround is two shaders, and no descriptor array, so the validation layer knows this shader is accessing the back buffer, with no possibility of accessing the front buffer
            if(deviceResources->GetCurrentFrameIndex())
                commandList->SetPipelineState(m_zoomCopy1PSO.Get());
            else
                commandList->SetPipelineState(m_zoomCopy0PSO.Get());

            // using wave32 with 8x4 wave size
            UINT dispatchX = AlignUp(UINT(deviceResources->GetScissorRect().right), 8);
            UINT dispatchY = AlignUp(UINT(deviceResources->GetScissorRect().bottom), 4);

            commandList->SetComputeRoot32BitConstant(TerrainRootSignature::CB1, UINT(zoomLevel), 0);
            commandList->Dispatch(dispatchX / 8, dispatchY / 4, 1);

            SwitchToGraphics(deviceResources, cbPerFrame);
            {
                D3D12_RESOURCE_BARRIER barrier[1];
                QuickBarrier(barrier[0], deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
        }
        PIXEndEvent(commandList);
    }
    ++m_frameCount;
    m_firstFrame = false;
}
