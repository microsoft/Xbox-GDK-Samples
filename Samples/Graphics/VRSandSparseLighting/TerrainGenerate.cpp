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




void Terrain::GeneateTerrain(std::unique_ptr<DX::DeviceResources> &deviceResources)
{
    auto commandList = deviceResources->GetCommandList();
    {
        D3D12_RESOURCE_BARRIER barrier[10];
        QuickBarrier(barrier[0], m_displayHeightMap.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[1], m_normalMap.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[2], m_AABBTileMinMaxBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[3], m_drawIndexedInstancedArgsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[4], m_sourceHeightMap[0].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[5], m_sourceHeightMap[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[6], m_instanceBuffer[LODs::VeryLow].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[7], m_instanceBuffer[LODs::Low].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[8], m_instanceBuffer[LODs::Medium].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        QuickBarrier(barrier[9], m_instanceBuffer[LODs::High].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }    
    // initialize AABB limits
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"m_terrainInitAABBsPSO");
        commandList->SetPipelineState(m_terrainInitAABBsPSO.Get());
        commandList->Dispatch(m_tileCount / (8 * 8), 1, 1);
        PIXEndEvent(commandList);
    }
    // create height map
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"m_terrainGenerateHeightsPSO");
        commandList->SetPipelineState(m_terrainGenerateHeightsPSO.Get());
        commandList->Dispatch(m_sourceHeightMapSize / 8, m_sourceHeightMapSize / 8, 1);
        PIXEndEvent(commandList);
    }
    // smooth terrain
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"m_terrainSmoothPSO");
        commandList->SetPipelineState(m_terrainSmoothPSO.Get());

        for (int i = 0; i < 8; ++i)
        {
            commandList->SetComputeRoot32BitConstant(TerrainRootSignature::CB1, 0, 0);
            commandList->Dispatch(m_sourceHeightMapSize / 8, m_sourceHeightMapSize / 8, 1);
            {
                // uav barriers
                D3D12_RESOURCE_BARRIER barrier[2];
                QuickBarrier(barrier[0], m_sourceHeightMap[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                QuickBarrier(barrier[1], m_sourceHeightMap[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
            commandList->SetComputeRoot32BitConstant(TerrainRootSignature::CB1, 1, 0);
            commandList->Dispatch(m_sourceHeightMapSize / 8, m_sourceHeightMapSize / 8, 1);
            {
                // uav barriers
                D3D12_RESOURCE_BARRIER barrier[2];
                QuickBarrier(barrier[0], m_sourceHeightMap[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                QuickBarrier(barrier[1], m_sourceHeightMap[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
            }
        }
        PIXEndEvent(commandList);
    }
    // create normals, AABB tile minY/maxY, AABB maxY per 8x8
    {
        D3D12_RESOURCE_BARRIER barrier[3];
        QuickBarrier(barrier[0], m_sourceHeightMap[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        QuickBarrier(barrier[1], m_AABBTileMinMaxBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);				// uav barrier
        QuickBarrier(barrier[2], m_lowResMaxHeight.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);	
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"m_terrainComputeAABBandNormalsPSO");
        commandList->SetPipelineState(m_terrainComputeAABBandNormalsPSO.Get());
        commandList->Dispatch(m_sourceHeightMapSize / 8, m_sourceHeightMapSize / 8, 1);
        {
            D3D12_RESOURCE_BARRIER barrier;
            QuickBarrier(barrier, m_displayHeightMap.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);	
            commandList->ResourceBarrier(1, &barrier);
        }
        commandList->SetPipelineState(m_terrainComputeLowResMaxHeight.Get());
        commandList->Dispatch(m_lowResMaxHeightSize, m_lowResMaxHeightSize, 1);
        PIXEndEvent(commandList);
    }
    {
        D3D12_RESOURCE_BARRIER barrier[3];
        QuickBarrier(barrier[0], m_AABBTileMinMaxBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        QuickBarrier(barrier[1], m_normalMap.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        QuickBarrier(barrier[2], m_lowResMaxHeight.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
    }
}
