//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Terrain.h"
#include "CommonStates.h"
#include "EffectPipelineStateDescription.h"
#include "ReadData.h"

using namespace DirectX;

#pragma warning(disable : 4061)


void NormalizePlane(float plane[4])
{
    float rMag;
    rMag = 1.0f / sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
    plane[0] = plane[0] * rMag;
    plane[1] = plane[1] * rMag;
    plane[2] = plane[2] * rMag;
    plane[3] = plane[3] * rMag;
}


void ExtractFrustumPlanes(Frustum &frustum, const XMFLOAT4X4 &vp)
{
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
    // Top clipping plane
    frustum.planes[2][0] = vp._14 - vp._12;
    frustum.planes[2][1] = vp._24 - vp._22;
    frustum.planes[2][2] = vp._34 - vp._32;
    frustum.planes[2][3] = vp._44 - vp._42;
    // Bottom clipping plane
    frustum.planes[3][0] = vp._14 + vp._12;
    frustum.planes[3][1] = vp._24 + vp._22;
    frustum.planes[3][2] = vp._34 + vp._32;
    frustum.planes[3][3] = vp._44 + vp._42;
    // Near clipping plane
    frustum.planes[4][0] = vp._13;
    frustum.planes[4][1] = vp._23;
    frustum.planes[4][2] = vp._33;
    frustum.planes[4][3] = vp._43;

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
    commandList->SetComputeRootConstantBufferView(ComputeTerrainRootConstants::CB0, cb.GpuAddress());
    commandList->SetComputeRootDescriptorTable(ComputeTerrainRootConstants::UAVs, m_resourceDescriptorsCBV_SRV_UAV->GetGpuHandle(Descriptors::SourceHeightMapUAV0));
    commandList->SetComputeRootDescriptorTable(ComputeTerrainRootConstants::SRVs, m_resourceDescriptorsCBV_SRV_UAV->GetGpuHandle(Descriptors::SourceHeightMapSRV0));
}



void Terrain::SwitchToGraphics(std::unique_ptr<DX::DeviceResources> &deviceResources, GraphicsResource &cb)
{
    auto commandList = deviceResources->GetCommandList();
    commandList->SetGraphicsRootSignature(m_globalRS.Get());
    commandList->SetGraphicsRootConstantBufferView(ComputeTerrainRootConstants::CB0, cb.GpuAddress());
    commandList->SetGraphicsRootDescriptorTable(ComputeTerrainRootConstants::UAVs, m_resourceDescriptorsCBV_SRV_UAV->GetGpuHandle(Descriptors::SourceHeightMapUAV0));
    commandList->SetGraphicsRootDescriptorTable(ComputeTerrainRootConstants::SRVs, m_resourceDescriptorsCBV_SRV_UAV->GetGpuHandle(Descriptors::SourceHeightMapSRV0));
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}



void Terrain::RenderScene(ID3D12GraphicsCommandList *commandList, UINT32 frameIndex, DX12Timer &gpuTimer, GPUPasses::Enum terrainTimer, GPUPasses::Enum skyTimer, RenderTechnqiue::Enum renderTechnique)
{
    // terrain
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"m_terrainRenderPSO");
        gpuTimer.StartTimer(commandList, terrainTimer);

        switch (renderTechnique)
        {
        default:
            assert(0);
        case RenderTechnqiue::Normal:
            commandList->SetPipelineState(m_terrainRenderPSO.Get());
            break;
        case RenderTechnqiue::AOandShadows:
            commandList->SetPipelineState(m_terrainRenderAOOnlyPSO.Get());
            break;
        }
        commandList->IASetIndexBuffer(&m_indexBufferView[LODs::High]);
        commandList->SetGraphicsRoot32BitConstant(ComputeTerrainRootConstants::CB1, 3, 0);  // select high res instance buffer
        commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_drawIndexedInstancedArgsBuffer.Get(), (6 + frameIndex) * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), nullptr, 0);

        commandList->IASetIndexBuffer(&m_indexBufferView[LODs::Medium]);
        commandList->SetGraphicsRoot32BitConstant(ComputeTerrainRootConstants::CB1, 2, 0);  // select medium res instance buffer
        commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_drawIndexedInstancedArgsBuffer.Get(), (4 + frameIndex) * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), nullptr, 0);

        commandList->IASetIndexBuffer(&m_indexBufferView[LODs::Low]);
        commandList->SetGraphicsRoot32BitConstant(ComputeTerrainRootConstants::CB1, 1, 0);  // select low res instance buffer
        commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_drawIndexedInstancedArgsBuffer.Get(), (2 + frameIndex) * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), nullptr, 0);

        commandList->IASetIndexBuffer(&m_indexBufferView[LODs::VeryLow]);
        commandList->SetGraphicsRoot32BitConstant(ComputeTerrainRootConstants::CB1, 0, 0);  // select very low res instance buffer
        commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_drawIndexedInstancedArgsBuffer.Get(), (0 + frameIndex) * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), nullptr, 0);

        gpuTimer.EndTimer(commandList, terrainTimer);
        PIXEndEvent(commandList);
    }
    // sky
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"m_skyRenderPSO");
        gpuTimer.StartTimer(commandList, skyTimer);
        commandList->SetPipelineState(m_skyRenderPSO.Get());
        commandList->IASetIndexBuffer(nullptr);
        commandList->DrawInstanced(3, 1, 0, 0);
        gpuTimer.EndTimer(commandList, skyTimer);
        PIXEndEvent(commandList);
    }
}



void Terrain::CalculateShadingRate(std::unique_ptr<DX::DeviceResources> &deviceResources, GraphicsResource &cb, ShadingRateCalculation::Enum shadingRateCalc, DX12Timer &gpuTimer)
{
    auto commandList = deviceResources->GetCommandList();
    SwitchToCompute(deviceResources, cb);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"VRS Calc Shading Rate");
    gpuTimer.StartTimer(commandList, GPUPasses::VRSCalcShadingRate);    
    {
        D3D12_RESOURCE_BARRIER barrier[1];
        QuickBarrier(barrier[0], m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }
    // don't process 16 pixel border around edge of the screen (safe area)
    UINT dispatchX = UINT(deviceResources->GetScissorRect().right) - 32;
    UINT dispatchY = UINT(deviceResources->GetScissorRect().bottom) - 32;

    // for wave32 full res, technically we should dispatch half the work in Y, since each wave will loop twice vertically to consume 8 rows of pixels, and we divide by 4 instead of 8 as the group size.
    // however halving both the numerator and the denominator results in the same final value, that is, dispatchY / 8 gives the same result
    // for wave32 half res, we are not looping vertically, so we need to dispatch twice as many groups vertically
    UINT ydenominator = 8;

    if (shadingRateCalc & 1)      // half res
    {
        dispatchX >>= 1;
        dispatchY >>= 1;

        if (shadingRateCalc & 2)   // wave32
        {
            ydenominator = 4;
        }
    }
    commandList->SetPipelineState(m_shadingRatePSOs[shadingRateCalc].Get());
    commandList->Dispatch(dispatchX / 8, dispatchY / ydenominator, 1);
    {
        D3D12_RESOURCE_BARRIER barrier[1];
        QuickBarrier(barrier[0], m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }
    gpuTimer.EndTimer(commandList, GPUPasses::VRSCalcShadingRate);
    PIXEndEvent(commandList);

    SwitchToGraphics(deviceResources, cb);
}



void Terrain::Render(std::unique_ptr<DX::DeviceResources> &deviceResources, UINT rtvIndex, UINT32 frameIndex, DX12Timer &gpuTimer, GPUPasses::Enum terrainPoass, GPUPasses::Enum skyPass, RenderTechnqiue::Enum renderTechnique)
{
    auto commandList = deviceResources->GetCommandList();
    auto dsvDescriptor = deviceResources->GetDepthStencilView();
    auto rtvDescriptor = m_resourceDescriptorRTV->GetCpuHandle(rtvIndex);

    D3D12_RESOURCE_BARRIER barrier[1];
    QuickBarrier(barrier[0], m_renderedImage[rtvIndex].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);

    commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    RenderScene(commandList, frameIndex, gpuTimer, terrainPoass, skyPass, renderTechnique);

    QuickBarrier(barrier[0], m_renderedImage[rtvIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
}



void Terrain::RenderNoVRS(std::unique_ptr<DX::DeviceResources> &deviceResources, UINT32 frameIndex, DX12Timer &gpuTimer, RenderTechnqiue::Enum renderTechnique)
{
    auto commandList = deviceResources->GetCommandList();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"1x1 Rate Render");
    Render(deviceResources, 0, frameIndex, gpuTimer, GPUPasses::NoVRSTerrain, GPUPasses::NoVRSSky, renderTechnique);
    PIXEndEvent(commandList);
}



void Terrain::RenderVRS(std::unique_ptr<DX::DeviceResources> &deviceResources, UINT32 frameIndex, DX12Timer &gpuTimer, RenderTechnqiue::Enum renderTechnique)
{
    auto commandList = deviceResources->GetCommandList();

    // Set Shading Rate Image, this has a cost
    {
        gpuTimer.StartTimer(commandList, GPUPasses::VRSSetShadingRate);
        D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT] =
        {
            D3D12_SHADING_RATE_COMBINER_OVERRIDE,       // pass thorough is the render state, override is the provoking vertex
            D3D12_SHADING_RATE_COMBINER_OVERRIDE        // take the shading rate map, ignore the previous combiner
        };

        // RSSetShadingRate can fail silently under certain conditions, disabling VRS. Luckily a D3D validation error
        // will indicate when such a scenario happens. This is a known issue and is tracked as driver bug#34719677.
        // In this sample, VRS is correctly enabled when it calls OMSetRenderTargets with a DSV that has an HTile.
        // Therefore we can ignore the D3D validation error.
        deviceResources->GetD3DDevice()->SetDebugErrorFilterX(0xC7F895EB, D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_BREAKS | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_FAILURE | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_OUTPUT);

        commandList->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
        commandList->RSSetShadingRateImage(m_shadingRateImage.Get());
        gpuTimer.EndTimer(commandList, GPUPasses::VRSSetShadingRate);
    }
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"VRS Render");
    Render(deviceResources, 1, frameIndex, gpuTimer, GPUPasses::VRSTerrain, GPUPasses::VRSSky, renderTechnique);
    PIXEndEvent(commandList);
    {
        D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT] =
        {
            D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,    // pass thorough is the render state
            D3D12_SHADING_RATE_COMBINER_PASSTHROUGH     // take the result of the previous combiner
        };
        commandList->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
        commandList->RSSetShadingRateImage(nullptr);
    }
}



void Terrain::Render(
    const XMFLOAT4X4 &viewProj,
    const XMFLOAT3 &position,
    std::unique_ptr<DX::DeviceResources> &deviceResources,
    std::unique_ptr<GraphicsMemory> &graphicsMemory,
    const float elapsedTimeSeconds,
    DX12Timer &gpuTimer,
    Visualise::Enum visualise,
    RenderTechnqiue::Enum renderTechnique,
    float sobelTolerance,
    ShadingRateCalculation::Enum shadingRateCalc,
    float sunHeight)
{
    static bool s_firstFrame = true;
    static UINT32 s_frameIndex = 0;

    // Setup per-frame constant buffer
    GraphicsResource cbPerFrame;
    {
        float worldScale = 0.3f;
        float worldScaleY = 25.0f * 3.0f;
        float xzTranslation = (-0.5f * float(m_heightMapSize)) * worldScale;

        Constants cb = {};

        // terrain
        cb.invSourceTextureSize = 1.0f / float(m_sourceHeightMapSize);
        cb.invTextureSize = 1.0f / float(m_heightMapSize);
        cb.terrainTileSizeLog2 = m_tileSizeLog2;
        cb.tileCountPerAxis = m_tileCountPerAxis;
        cb.frameIndex = s_frameIndex;
        cb.worldScaleY = worldScaleY;
        cb.worldScale = worldScale;
        cb.xzTranslation = xzTranslation;
        cb.halfTexelOffsetSourceSize = 1.0 / float(m_sourceHeightMapSize * 2.0);
        cb.sourceToHeightMapSizeMultiplier = 1.0f / float(1 << m_sourceToHeightMapSizeLog2);
        cb.sourceToHeightMapSizeLog2 = m_sourceToHeightMapSizeLog2;
        cb.worldToTextureCoordMul = (1.0f / worldScale) * cb.invTextureSize;

        // camera
        XMMATRIX world;
        world.r[0] = XMVectorSet(worldScale, 0.0f, 0.0f, 0.0f);
        world.r[1] = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        world.r[2] = XMVectorSet(0.0f, 0.0f, worldScale, 0.0f);
        world.r[3] = XMVectorSet(xzTranslation, 0.0f, xzTranslation, 1.0f);
        XMMATRIX viewProjMatrix = XMLoadFloat4x4(&viewProj);
        XMMATRIX wvp = XMMatrixMultiply(world, viewProjMatrix);
        cb.worldViewProjectionMatrix = XMMatrixTranspose(wvp);

        XMVECTOR p1 = XMVectorSet(-1.0, -1.0, 1.0, 1.0);
        XMVECTOR p2 = XMVectorSet(1.0, -1.0, 1.0, 1.0);
        XMVECTOR p3 = XMVectorSet(-1.0, 1.0, 1.0, 1.0);

        XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProjMatrix);
        p1 = XMVector3TransformCoord(p1, invViewProj);
        p2 = XMVector3TransformCoord(p2, invViewProj);
        p3 = XMVector3TransformCoord(p3, invViewProj);

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
        cb.stepSize = lerp(0.5, 10.0, cb.lightDir.y * cb.lightDir.y);    // higher step size the lower the angle of the sun, needed to capture small details
        cb.lowResMaxHeightMapSize = float(m_lowResMaxHeightSize);
 
        // shading rate generation 
        cb.depthCompare = 0.001f;
        cb.discardSampleCountMaxRate = 2;
        cb.discardSampleCountHalfRate = 2;
        cb.depthTolerance = 2.0f;
        cb.sobelTolerance = sobelTolerance;
        cb.invInputDimX = 1.0f / float(deviceResources->GetScissorRect().right);
        cb.invInputDimY = 1.0f / float(deviceResources->GetScissorRect().bottom);
        cb.halfResInputMode = 0;

        cb.shadingRateSurfaceSizeFX = float(m_shadingRateWidth);
        cb.shadingRateSurfaceSizeFY = float(m_shadingRateHeight);
        cb.shadingRateSurfaceSizeUX = m_shadingRateWidth;
        cb.shadingRateSurfaceSizeUY = m_shadingRateHeight;

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
    
    if (s_firstFrame)
    {
        GeneateTerrain(deviceResources);

        // set whole shading rate target to 2x2, including the edges
        UINT32 fillValue = (D3D12_SHADING_RATE_2X2 << 24) | (D3D12_SHADING_RATE_2X2 << 16) | (D3D12_SHADING_RATE_2X2 << 8) | D3D12_SHADING_RATE_2X2;
        commandList->FillMemoryWith32BitValueX(m_shadingRateImage->GetGPUVirtualAddress(), m_shadingRateImageSizeBytes, fillValue, D3D12XBOX_COPY_FLAG_NONE);
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
    // Rendering time, render twice with and without VRS
    SwitchToGraphics(deviceResources, cbPerFrame);

    RenderVRS(deviceResources, s_frameIndex, gpuTimer, renderTechnique);
    RenderNoVRS(deviceResources, s_frameIndex, gpuTimer, renderTechnique);
    CalculateShadingRate(deviceResources, cbPerFrame, shadingRateCalc, gpuTimer);

    // copy to back buffer
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"CopyToBackBuffer");
        commandList->IASetIndexBuffer(nullptr);

        // set back buffer
        if (visualise == Visualise::ShadingRateOverlay)
        {
            {
                D3D12_RESOURCE_BARRIER barrier[2];
                QuickBarrier(barrier[0], m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                QuickBarrier(barrier[1], m_renderedImage[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
            // set back buffer
            auto dsvDescriptor = deviceResources->GetDepthStencilView();
            auto rtvDescriptor = m_resourceDescriptorRTV->GetCpuHandle(1);
            commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);
            commandList->SetPipelineState(m_debugVisualizeShadingRateOverlayPSO.Get());
            commandList->DrawInstanced(3, 1, 0, 0);
            {
                D3D12_RESOURCE_BARRIER barrier[2];
                QuickBarrier(barrier[0], m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
                QuickBarrier(barrier[1], m_renderedImage[1].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
            visualise = Visualise::VRS;     // drop down
        }
        if (visualise == Visualise::ShadingRate)
        {
            {
                D3D12_RESOURCE_BARRIER barrier;
                QuickBarrier(barrier, m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                commandList->ResourceBarrier(1, &barrier);
            }
            // set back buffer
            auto rtvDescriptor = deviceResources->GetRenderTargetView();
            auto dsvDescriptor = deviceResources->GetDepthStencilView();
            commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
            commandList->SetPipelineState(m_debugVisualizeShadingRatePSO.Get());
            commandList->DrawInstanced(3, 1, 0, 0);
            {
                D3D12_RESOURCE_BARRIER barrier;
                QuickBarrier(barrier, m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
                commandList->ResourceBarrier(1, &barrier);
            }
        }
        if (visualise == Visualise::NoVRS || visualise == Visualise::VRS)
        {
            {
                D3D12_RESOURCE_BARRIER barrier[3];
                QuickBarrier(barrier[0], deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
                QuickBarrier(barrier[1], m_renderedImage[0].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
                QuickBarrier(barrier[2], m_renderedImage[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
            switch (visualise)
            {
            default: assert(0);
            case Visualise::NoVRS: commandList->CopyResourceX(deviceResources->GetRenderTarget(), m_renderedImage[0].Get(), D3D12XBOX_COPY_FLAG_NONE); break;
            case Visualise::VRS: commandList->CopyResourceX(deviceResources->GetRenderTarget(), m_renderedImage[1].Get(), D3D12XBOX_COPY_FLAG_NONE); break;
            }
            {
                D3D12_RESOURCE_BARRIER barrier[3];
                QuickBarrier(barrier[0], deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
                QuickBarrier(barrier[1], m_renderedImage[0].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                QuickBarrier(barrier[2], m_renderedImage[1].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
        }
        else if (visualise == Visualise::Diff)
        {
            // set back buffer
            auto rtvDescriptor = deviceResources->GetRenderTargetView();
            auto dsvDescriptor = deviceResources->GetDepthStencilView();
            commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
            commandList->SetPipelineState(m_debugVisualizeAbsDiffPSO.Get());
            commandList->DrawInstanced(3, 1, 0, 0);
        }
        PIXEndEvent(commandList);
    }
    s_frameIndex ^= 1;
    s_firstFrame = false;
}
