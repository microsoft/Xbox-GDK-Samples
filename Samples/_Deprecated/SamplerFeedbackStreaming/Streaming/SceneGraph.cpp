//--------------------------------------------------------------------------------------
// SceneGraph.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "SceneGraph.h"
#include "TextureFiles.h"

SceneObject* g_pCubeObject = nullptr;
SceneObject* g_pPlaneObject = nullptr;

SceneGraph g_SceneGraph;

HRESULT InitializeSceneObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pInitCmdList, CpuGpuHeap* pUploadHeap)
{
    HRESULT hr;

    g_pCubeObject = new SceneObject();
    g_pPlaneObject = new SceneObject();

#define FACEF(x) (x),((x)+1),((x)+2),((x)+2),((x)+1),((x)+3)
#define FACER(x) (x),((x)+2),((x)+1),((x)+2),((x)+3),((x)+1)

    const FLOAT PlaneSize = 15.0f;
    const SceneVertex SceneVertices[] =
    {
        { { -PlaneSize, 0, -PlaneSize },{ 0, 1, 0 },{ 0, 0 },{  0,  1,  0 },{ 0,  1,  0 } },
        { {  PlaneSize, 0, -PlaneSize },{ 0, 1, 0 },{ 8, 0 },{  0,  1,  0 },{ 0,  1,  0 } },
        { { -PlaneSize, 0,  PlaneSize },{ 0, 1, 0 },{ 0, 8 },{  0,  1,  0 },{ 0,  1,  0 } },
        { {  PlaneSize, 0,  PlaneSize },{ 0, 1, 0 },{ 8, 8 },{  0,  1,  0 },{ 0,  1,  0 } },
    };

    const USHORT SceneIndices[] =
    {
        FACER(0)
    };

    hr = CreateDefaultVertexBuffer(pd3dDevice, pUploadHeap, pInitCmdList, sizeof(SceneVertices), sizeof(SceneVertex), SceneVertices, &g_pPlaneObject->pVertexBuffer, &g_pPlaneObject->VBV);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = CreateDefaultIndexBuffer(pd3dDevice, pUploadHeap, pInitCmdList, sizeof(SceneIndices), DXGI_FORMAT_R16_UNORM, SceneIndices, &g_pPlaneObject->pIndexBuffer, &g_pPlaneObject->IBV);
    if (FAILED(hr))
    {
        return hr;
    }
    g_pPlaneObject->IndexCount = ARRAYSIZE(SceneIndices);
    g_pPlaneObject->PrimTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    g_pPlaneObject->AABB.Center = XMFLOAT3(0, 0, 0);
    g_pPlaneObject->AABB.Extents = XMFLOAT3(PlaneSize, 0.001f, PlaneSize);

    const SceneVertex CubeVertices[] =
    {
        // bottom face
        { { -1, -1, -1 },{  0, -1,  0 },{ 0, 0 },{ -1,  0,  0 },{  0,  0, -1 } },
        { {  1, -1, -1 },{  0, -1,  0 },{ 1, 0 },{ -1,  0,  0 },{  0,  0, -1 } },
        { { -1, -1,  1 },{  0, -1,  0 },{ 0, 1 },{ -1,  0,  0 },{  0,  0, -1 } },
        { {  1, -1,  1 },{  0, -1,  0 },{ 1, 1 },{ -1,  0,  0 },{  0,  0, -1 } },
                                                                   
        // top face                                                
        { { -1,  1, -1 },{  0,  1,  0 },{ 0, 0 },{  1,  0,  0 },{  0,  0, -1 } },
        { {  1,  1, -1 },{  0,  1,  0 },{ 1, 0 },{  1,  0,  0 },{  0,  0, -1 } },
        { { -1,  1,  1 },{  0,  1,  0 },{ 0, 1 },{  1,  0,  0 },{  0,  0, -1 } },
        { {  1,  1,  1 },{  0,  1,  0 },{ 1, 1 },{  1,  0,  0 },{  0,  0, -1 } },
                                                                   
        // left face                                               
        { { -1, -1, -1 },{ -1,  0,  0 },{ 0, 0 },{  0,  0, -1 },{  0, -1,  0 } },
        { { -1, -1,  1 },{ -1,  0,  0 },{ 0, 1 },{  0,  0, -1 },{  0, -1,  0 } },
        { { -1,  1, -1 },{ -1,  0,  0 },{ 1, 0 },{  0,  0, -1 },{  0, -1,  0 } },
        { { -1,  1,  1 },{ -1,  0,  0 },{ 1, 1 },{  0,  0, -1 },{  0, -1,  0 } },
                                                                   
        // right face                                              
        { {  1, -1, -1 },{  1,  0,  0 },{ 0, 0 },{  0,  0,  1 },{  0, -1,  0 } },
        { {  1, -1,  1 },{  1,  0,  0 },{ 0, 1 },{  0,  0,  1 },{  0, -1,  0 } },
        { {  1,  1, -1 },{  1,  0,  0 },{ 1, 0 },{  0,  0,  1 },{  0, -1,  0 } },
        { {  1,  1,  1 },{  1,  0,  0 },{ 1, 1 },{  0,  0,  1 },{  0, -1,  0 } },
                                                                   
        // front face                                              
        { { -1, -1, -1 },{  0,  0, -1 },{ 0, 0 },{  1,  0,  0 },{  0, -1,  0 } },
        { {  1, -1, -1 },{  0,  0, -1 },{ 1, 0 },{  1,  0,  0 },{  0, -1,  0 } },
        { { -1,  1, -1 },{  0,  0, -1 },{ 0, 1 },{  1,  0,  0 },{  0, -1,  0 } },
        { {  1,  1, -1 },{  0,  0, -1 },{ 1, 1 },{  1,  0,  0 },{  0, -1,  0 } },
                                                                   
        // back face                                               
        { { -1, -1,  1 },{  0,  0,  1 },{ 0, 0 },{ -1,  0,  0 },{  0, -1,  0 } },
        { {  1, -1,  1 },{  0,  0,  1 },{ 1, 0 },{ -1,  0,  0 },{  0, -1,  0 } },
        { { -1,  1,  1 },{  0,  0,  1 },{ 0, 1 },{ -1,  0,  0 },{  0, -1,  0 } },
        { {  1,  1,  1 },{  0,  0,  1 },{ 1, 1 },{ -1,  0,  0 },{  0, -1,  0 } },
    };

    const USHORT CubeIndices[] =
    {
        FACEF(0),
        FACER(4),
        FACEF(8),
        FACER(12),
        FACER(16),
        FACEF(20),
    };
    hr = CreateDefaultVertexBuffer(pd3dDevice, pUploadHeap, pInitCmdList, sizeof(CubeVertices), sizeof(SceneVertex), CubeVertices, &g_pCubeObject->pVertexBuffer, &g_pCubeObject->VBV);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = CreateDefaultIndexBuffer(pd3dDevice, pUploadHeap, pInitCmdList, sizeof(CubeIndices), DXGI_FORMAT_R16_UINT, CubeIndices, &g_pCubeObject->pIndexBuffer, &g_pCubeObject->IBV);
    if (FAILED(hr))
    {
        return hr;
    }
    g_pCubeObject->IndexCount = ARRAYSIZE(CubeIndices);
    g_pCubeObject->PrimTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    g_pCubeObject->AABB.Center = XMFLOAT3(0, 0, 0);
    g_pCubeObject->AABB.Extents = XMFLOAT3(1, 1, 1);

    return S_OK;
}

void SceneGraph::Initialize(ID3D12Device* pd3dDevice, ID3D12Fence* pGpuFence, UINT64* pCpuFence, UINT32 MaxInstanceCount)
{
    m_pd3dDevice = pd3dDevice;
    m_CBUploadHeap.Initialize(pd3dDevice, pGpuFence, pCpuFence, 256 * 1024);

    // Create descriptor heaps - one heap per streaming texture component view (texture, MinLOD, feedback).
    // The layout of the descriptor heaps will consist of blocks where each block has one view per surface texture type (diffuse, normal, specular, etc).
    // Each instance will consume one block (same block index for all heaps).
    m_InstanceDescriptors.Initialize(pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true, STCT_Count, ST_Count, MaxInstanceCount);
    m_InstanceDescriptors.GetDescriptorHeap()->SetName(L"Instance Descriptors");
    m_MaxInstanceCount = MaxInstanceCount;

    m_FilterSlopes = XMFLOAT4(2, 2, 0, 0);

    m_SamplerHeap.Initialize(pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16, true);

    D3D12_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.MipLODBias = 0;
    SamplerDesc.MaxAnisotropy = 16;
    SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    SamplerDesc.BorderColor[0] = 0.0f;
    SamplerDesc.BorderColor[1] = 0.0f;
    SamplerDesc.BorderColor[2] = 0.0f;
    SamplerDesc.BorderColor[3] = 0.0f;
    SamplerDesc.MinLOD = 0.0f;
    SamplerDesc.MaxLOD = 9999.0f;
    pd3dDevice->CreateSampler(&SamplerDesc, m_SamplerHeap.hCPU(0));

    SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    pd3dDevice->CreateSampler(&SamplerDesc, m_SamplerHeap.hCPU(14));

    SamplerDesc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
    pd3dDevice->CreateSampler(&SamplerDesc, m_SamplerHeap.hCPU(15));
}

void SceneGraph::SetFilterSlopeEnums(UINT32 SlopeUIndex, UINT32 SlopeVIndex, UINT32 OffsetUIndex, UINT32 OffsetVIndex)
{
    m_FilterSlopes.x = g_FilterSlopeValues[SlopeUIndex];
    m_FilterSlopes.y = g_FilterSlopeValues[SlopeVIndex];
    m_FilterSlopes.z = g_FilterOffsetValues[OffsetUIndex];
    m_FilterSlopes.w = g_FilterOffsetValues[OffsetVIndex];

#if XBOX_SAMPLER_FEEDBACK
    D3D12XBOX_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.FilterMag = D3D12XBOX_TEXTURE_XY_FILTER_BILINEAR;
    SamplerDesc.FilterMin = D3D12XBOX_TEXTURE_XY_FILTER_BILINEAR;
    SamplerDesc.FilterMip = D3D12XBOX_TEXTURE_MIP_FILTER_LINEAR;
    SamplerDesc.FilterZ = D3D12XBOX_TEXTURE_Z_FILTER_LINEAR;
    SamplerDesc.FilterMode = D3D12XBOX_TEXTURE_FILTER_MODE_LERP;
    SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.MipLODBias = 0;
    SamplerDesc.MaxAnisotropy = 16;
    SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    SamplerDesc.BorderColor[0] = 0.0f;
    SamplerDesc.BorderColor[1] = 0.0f;
    SamplerDesc.BorderColor[2] = 0.0f;
    SamplerDesc.BorderColor[3] = 0.0f;
    SamplerDesc.MinLOD = 0.0f;
    SamplerDesc.MaxLOD = 9999.0f;

    SamplerDesc.MinMipSlopeU = (D3D12XBOX_MINMIP_FILTER_SLOPE)SlopeUIndex;
    SamplerDesc.MinMipSlopeVOrW = (D3D12XBOX_MINMIP_FILTER_SLOPE)SlopeVIndex;
    SamplerDesc.MinMipOffsetU = (D3D12XBOX_MINMIP_FILTER_OFFSET)OffsetUIndex;
    SamplerDesc.MinMipOffsetVOrW = (D3D12XBOX_MINMIP_FILTER_OFFSET)OffsetVIndex;
    SamplerDesc.Flags |= D3D12XBOX_SAMPLER_FLAG_ENABLE_MINMIP_FILTER;

    m_pd3dDevice->CreateSamplerX(&SamplerDesc, m_SamplerHeap.hCPU(13));
#endif  
}

bool SceneGraph::AddInstance(const SceneObject* pObject, XMVECTOR Position, XMVECTOR Orientation, XMVECTOR Scale, UINT32 TextureIndex, UINT32 TextureIndex2, UINT32 TextureIndex3, const WCHAR* strName)
{
    if ((UINT32)m_Instances.size() >= m_MaxInstanceCount)
    {
        return false;
    }

    const USHORT DescriptorBlockIndex = m_InstanceDescriptors.AllocateBlock();
    if (DescriptorBlockIndex == BlockAllocator::NULL_BLOCK_INDEX)
    {
        return false;
    }

    SceneObjectInstance* pInstance = new SceneObjectInstance();
    pInstance->InstanceIndex = DescriptorBlockIndex;
    pInstance->pSceneObject = pObject;
    if (strName != nullptr)
    {
        wcscpy_s(pInstance->strName, strName);
    }
    else
    {
        pInstance->strName[0] = L'\0';
    }

    XMMATRIX matTransform = XMMatrixRotationQuaternion(Orientation) * XMMatrixScalingFromVector(Scale);
    matTransform.r[3] = XMVectorSelect(g_XMOne, Position, g_XMSelect1110);
    XMStoreFloat4x4(&pInstance->matWorld, matTransform);

    pObject->AABB.Transform(pInstance->AABB, matTransform);

    WCHAR strTexFileName[MAX_PATH];
    WCHAR strTexDataFileName[MAX_PATH];

    CreateTextureFilename(strTexFileName, ARRAYSIZE(strTexFileName), TextureIndex, TextureSuffix_Diffuse, false);
    CreateTextureFilename(strTexDataFileName, ARRAYSIZE(strTexDataFileName), TextureIndex, TextureSuffix_Diffuse, true);
    pInstance->pSurfaceTextures[ST_Diffuse] = g_StreamingTextureManager.LoadTexture(strTexFileName, strTexDataFileName);

    CreateTextureFilename(strTexFileName, ARRAYSIZE(strTexFileName), TextureIndex, TextureSuffix_Normal, false);
    CreateTextureFilename(strTexDataFileName, ARRAYSIZE(strTexDataFileName), TextureIndex, TextureSuffix_Normal, true);
    pInstance->pSurfaceTextures[ST_Normal] = g_StreamingTextureManager.LoadTexture(strTexFileName, strTexDataFileName);

    CreateTextureFilename(strTexFileName, ARRAYSIZE(strTexFileName), TextureIndex, TextureSuffix_Specular, false);
    CreateTextureFilename(strTexDataFileName, ARRAYSIZE(strTexDataFileName), TextureIndex, TextureSuffix_Specular, true);
    pInstance->pSurfaceTextures[ST_Specular] = g_StreamingTextureManager.LoadTexture(strTexFileName, strTexDataFileName);

    CreateTextureFilename(strTexFileName, ARRAYSIZE(strTexFileName), TextureIndex2, TextureSuffix_Diffuse, false);
    CreateTextureFilename(strTexDataFileName, ARRAYSIZE(strTexDataFileName), TextureIndex2, TextureSuffix_Diffuse, true);
    pInstance->pSurfaceTextures[ST_Diffuse2] = g_StreamingTextureManager.LoadTexture(strTexFileName, strTexDataFileName);

    CreateTextureFilename(strTexFileName, ARRAYSIZE(strTexFileName), TextureIndex2, TextureSuffix_Normal, false);
    CreateTextureFilename(strTexDataFileName, ARRAYSIZE(strTexDataFileName), TextureIndex2, TextureSuffix_Normal, true);
    pInstance->pSurfaceTextures[ST_Normal2] = g_StreamingTextureManager.LoadTexture(strTexFileName, strTexDataFileName);

    CreateTextureFilename(strTexFileName, ARRAYSIZE(strTexFileName), TextureIndex2, TextureSuffix_Specular, false);
    CreateTextureFilename(strTexDataFileName, ARRAYSIZE(strTexDataFileName), TextureIndex2, TextureSuffix_Specular, true);
    pInstance->pSurfaceTextures[ST_Specular2] = g_StreamingTextureManager.LoadTexture(strTexFileName, strTexDataFileName);

    CreateTextureFilename(strTexFileName, ARRAYSIZE(strTexFileName), TextureIndex3, TextureSuffix_Diffuse, false);
    CreateTextureFilename(strTexDataFileName, ARRAYSIZE(strTexDataFileName), TextureIndex3, TextureSuffix_Diffuse, true);
    pInstance->pSurfaceTextures[ST_Diffuse3] = g_StreamingTextureManager.LoadTexture(strTexFileName, strTexDataFileName);

    CreateTextureFilename(strTexFileName, ARRAYSIZE(strTexFileName), TextureIndex3, TextureSuffix_Normal, false);
    CreateTextureFilename(strTexDataFileName, ARRAYSIZE(strTexDataFileName), TextureIndex3, TextureSuffix_Normal, true);
    pInstance->pSurfaceTextures[ST_Normal3] = g_StreamingTextureManager.LoadTexture(strTexFileName, strTexDataFileName);

    CreateTextureFilename(strTexFileName, ARRAYSIZE(strTexFileName), TextureIndex3, TextureSuffix_Specular, false);
    CreateTextureFilename(strTexDataFileName, ARRAYSIZE(strTexDataFileName), TextureIndex3, TextureSuffix_Specular, true);
    pInstance->pSurfaceTextures[ST_Specular3] = g_StreamingTextureManager.LoadTexture(strTexFileName, strTexDataFileName);

    for (UINT32 i = 0; i < ST_Count; ++i)
    {
        if (pInstance->pSurfaceTextures[i] != nullptr)
        {
            pInstance->pFeedbackTextures[i] = FindOrCreateFeedbackTexture(pInstance->pSurfaceTextures[i]);
        }
    }

    pInstance->hSRVs = m_InstanceDescriptors.GetGpuDescriptorHandle(DescriptorBlockIndex, STCT_PrimaryTexture);
    pInstance->hMinLODSRVs = m_InstanceDescriptors.GetGpuDescriptorHandle(DescriptorBlockIndex, STCT_MinLODTexture);
    pInstance->hFeedbackUAVs = m_InstanceDescriptors.GetGpuDescriptorHandle(DescriptorBlockIndex, STCT_FeedbackTexture);
    UpdateViewDescriptors(pInstance);

    m_Instances.push_back(pInstance);

    return true;
}

FeedbackTexture* SceneGraph::FindOrCreateFeedbackTexture(StreamingTexture* pST)
{
    auto iter = m_FeedbackTextures.find(pST);
    if (iter != m_FeedbackTextures.end())
    {
        return iter->second;
    }

    FeedbackTexture* pFT = g_StreamingTextureManager.CreateFeedbackTexture(pST);
    m_FeedbackTextures[pST] = pFT;

    return pFT;
}

//-------------------------------------------------------------------------------------------------
// UpdateViewDescriptors gathers SRV and UAV descriptors from the streaming textures' component views
// and arranges them into contiguous ranges of descriptors in the scene graph descriptor heaps.
// This is basically copying from array-of-structs (streaming textures with their component views) into
// struct-of-arrays (instances with their surface textures).
void SceneGraph::UpdateViewDescriptors(SceneObjectInstance* pInstance)
{
    const UINT64 HandleIncrementSize = m_InstanceDescriptors.GetIncrementSize();

    const USHORT DescriptorBlockIndex = pInstance->InstanceIndex;
    for (UINT32 TextureIndex = 0; TextureIndex < ST_Count; ++TextureIndex)
    {
        for (UINT32 ComponentIndex = 0; ComponentIndex < STCT_Count; ++ComponentIndex)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE hDest = m_InstanceDescriptors.GetCpuDescriptorHandle(DescriptorBlockIndex, ComponentIndex);
            hDest.ptr += HandleIncrementSize * TextureIndex;

            D3D12_CPU_DESCRIPTOR_HANDLE hSrc = GetInstanceCpuDescriptor(pInstance, TextureIndex, ComponentIndex);

            m_pd3dDevice->CopyDescriptorsSimple(1, hDest, hSrc, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE SceneGraph::GetInstanceCpuDescriptor(SceneObjectInstance* pInstance, UINT32 TextureIndex, UINT32 ComponentIndex)
{
    if (TextureIndex >= ST_Count || ComponentIndex >= STCT_Count)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE NullDescriptor = {};
        return NullDescriptor;
    }

    switch (ComponentIndex)
    {
    case STCT_PrimaryTexture:
    default:
        return pInstance->pSurfaceTextures[TextureIndex]->hTiledTextureSRV;
    case STCT_MinLODTexture:
        return pInstance->pSurfaceTextures[TextureIndex]->hNativeMinLODTextureSRV;
    case STCT_FeedbackTexture:
        return pInstance->pFeedbackTextures[TextureIndex]->hUAV;
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE SceneGraph::GetInstanceGpuDescriptor(SceneObjectInstance* pInstance, UINT32 TextureIndex, UINT32 ComponentIndex)
{
    if (TextureIndex >= ST_Count || ComponentIndex >= STCT_Count)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE NullDescriptor = {};
        return NullDescriptor;
    }

    const UINT64 HandleIncrementSize = m_InstanceDescriptors.GetIncrementSize();
    D3D12_GPU_DESCRIPTOR_HANDLE hDest = m_InstanceDescriptors.GetGpuDescriptorHandle(pInstance->InstanceIndex, ComponentIndex);
    hDest.ptr += HandleIncrementSize * TextureIndex;

    return hDest;
}

void SceneGraph::Render(XMMATRIX matView, XMMATRIX matProjection, ID3D12GraphicsCommandList* pCmdList, ID3D12CommandQueue* pCmdQueue)
{
    ID3D12DescriptorHeap* pHeaps[] = { m_InstanceDescriptors.GetDescriptorHeap(), m_SamplerHeap };
    pCmdList->SetDescriptorHeaps(ARRAYSIZE(pHeaps), pHeaps);

    XMVECTOR vDet;
    const XMMATRIX matCameraWorld = XMMatrixInverse(&vDet, matView);
    const DirectX::BoundingFrustum LocalFrustum(matProjection);
    DirectX::BoundingFrustum WorldFrustum;
    LocalFrustum.Transform(WorldFrustum, matCameraWorld);

    CBScene* pCBScene = nullptr;
    m_CBUploadHeap.AllocateRootConstantBuffer(&pCBScene, pCmdList, 1);

    XMStoreFloat4x4(&pCBScene->matViewProjection, matView * matProjection);
    XMStoreFloat4(&pCBScene->Zero, g_XMZero);
    XMStoreFloat4(&pCBScene->CameraPosWorld, matCameraWorld.r[3]);
    pCBScene->FilterSlopes = m_FilterSlopes;
    pCBScene->NonResidentColor = m_NonResidentColor;
    pCBScene->StochasticConstants = m_StochasticConstants;

    pCmdList->SetGraphicsRootDescriptorTable(5, m_SamplerHeap.hGPU(0));

    for (SceneObjectInstance* pInstance : m_Instances)
    {
        bool IsVisible = WorldFrustum.Intersects(pInstance->AABB);
        if (!IsVisible)
        {
            continue;
        }

        const SceneObject* pObject = pInstance->pSceneObject;

        CBObject* pCBObject = nullptr;
        m_CBUploadHeap.AllocateRootConstantBuffer(&pCBObject, pCmdList, 0);

        pCBObject->matWorld = pInstance->matWorld;
        if (pInstance->hSRVs.ptr != 0)
        {
            for (UINT32 i = 0; i < ST_Count; ++i)
            {
                const D3D12_SUBRESOURCE_FOOTPRINT& FP = pInstance->pFeedbackTextures[i]->Staging.CopyLocation.PlacedFootprint.Footprint;
                pCBObject->FeedbackDimensions[i].x = FP.Width;
                pCBObject->FeedbackDimensions[i].y = FP.Height;
            }
        }

        if (pInstance->hSRVs.ptr != 0)
        {
            pCmdList->SetGraphicsRootDescriptorTable(2, pInstance->hSRVs);
            pCmdList->SetGraphicsRootDescriptorTable(3, pInstance->hMinLODSRVs);
            pCmdList->SetGraphicsRootDescriptorTable(4, pInstance->hFeedbackUAVs);
        }

        pCmdList->IASetVertexBuffers(0, 1, &pObject->VBV);
        pCmdList->IASetIndexBuffer(&pObject->IBV);
        pCmdList->IASetPrimitiveTopology(pObject->PrimTopology);
        pCmdList->DrawIndexedInstanced(pObject->IndexCount, 1, 0, 0, 0);
    }
}

SceneObjectInstance* SceneGraph::FindSceneObjectInstance(XMVECTOR RayOrigin, XMVECTOR RayDirection) const
{
    FLOAT MinDistance = FLT_MAX;
    SceneObjectInstance* pClosestInstance = nullptr;
    for (SceneObjectInstance* pInstance : m_Instances)
    {
        FLOAT Distance = FLT_MAX;
        if (pInstance->AABB.Intersects(RayOrigin, RayDirection, Distance))
        {
            if (Distance < MinDistance && Distance >= 0.0f)
            {
                pClosestInstance = pInstance;
                MinDistance = Distance;
            }
        }
    }
    return pClosestInstance;
}