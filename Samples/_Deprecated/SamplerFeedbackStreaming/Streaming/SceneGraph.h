//--------------------------------------------------------------------------------------
// SceneGraph.h
//
// Data structures and code for simple prefabricated objects and a scenegraph.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "d3d12app.hpp"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "TextureStreaming.h"
#include "d3d12util.h"

struct SceneVertex
{
    FLOAT Position[3];
    FLOAT Normal[3];
    FLOAT TexCoord[2];
    FLOAT Tangent[3];
    FLOAT Binormal[3];
};

static const D3D12_INPUT_ELEMENT_DESC SceneVertexElements[] =
{
    { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  0,      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,    0 },
    { "NORMAL",     0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  12,     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,    0 },
    { "TEXCOORD",   0,  DXGI_FORMAT_R32G32_FLOAT,           0,  24,     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,    0 },
    { "TANGENT",    0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  32,     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,    0 },
    { "BINORMAL",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  44,     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,    0 },
};

struct SceneObject
{
    ID3D12Resource* pVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW VBV;

    ID3D12Resource* pIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW IBV;

    D3D12_PRIMITIVE_TOPOLOGY PrimTopology;
    UINT32 IndexCount;

    DirectX::BoundingBox AABB;
};
extern SceneObject* g_pCubeObject;
extern SceneObject* g_pPlaneObject;

HRESULT InitializeSceneObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pInitCmdList, CpuGpuHeap* pUploadHeap);

enum SurfaceTextureType
{
    ST_Diffuse = 0,
    ST_Normal = 1,
    ST_Specular = 2,
    ST_Diffuse2 = 3,
    ST_Normal2 = 4,
    ST_Specular2 = 5,
    ST_Diffuse3 = 6,
    ST_Normal3 = 7,
    ST_Specular3 = 8,
    ST_Count
};

struct SceneObjectInstance
{
    XMFLOAT4X4 matWorld;
    DirectX::BoundingBox AABB;

    const SceneObject* pSceneObject;

    StreamingTexture* pSurfaceTextures[ST_Count];
    FeedbackTexture* pFeedbackTextures[ST_Count];

    USHORT InstanceIndex;
    D3D12_GPU_DESCRIPTOR_HANDLE hSRVs;
    D3D12_GPU_DESCRIPTOR_HANDLE hMinLODSRVs;
    D3D12_GPU_DESCRIPTOR_HANDLE hFeedbackUAVs;

    WCHAR strName[64];
};

struct CBScene
{
    XMFLOAT4X4 matViewProjection;
    XMFLOAT4 Zero;
    XMFLOAT4 CameraPosWorld;
    XMFLOAT4 FilterSlopes;
    XMFLOAT4 NonResidentColor;
    XMFLOAT4 StochasticConstants;
};

struct CBObject
{
    XMFLOAT4X4 matWorld;
    XMUINT4 FeedbackDimensions[ST_Count];
};

class SceneGraph
{
private:
    ID3D12Device* m_pd3dDevice;
    typedef std::vector<SceneObjectInstance*> SceneObjectInstanceCollection;
    SceneObjectInstanceCollection m_Instances;
    UINT32 m_MaxInstanceCount;
    BlockAllocatedDescriptorHeap m_InstanceDescriptors;

    CpuGpuHeap m_CBUploadHeap;

    XMFLOAT4 m_FilterSlopes;
    XMFLOAT4 m_NonResidentColor;
    XMFLOAT4 m_StochasticConstants;

    typedef std::unordered_map<StreamingTexture*, FeedbackTexture*> FeedbackTextureMap;
    FeedbackTextureMap m_FeedbackTextures;

    DescriptorHeapWrapper m_SamplerHeap;

public:
    void Initialize(ID3D12Device* pd3dDevice, ID3D12Fence* pGpuFence, UINT64* pCpuFence, UINT32 MaxInstanceCount = 5000);

    bool AddInstance(const SceneObject* pObject, XMVECTOR Position, XMVECTOR Orientation, XMVECTOR Scale, UINT32 TextureIndex, UINT32 TextureIndex2, UINT32 TextureIndex3, const WCHAR* strName);
    UINT32 GetInstanceCount() const { return (UINT32)m_Instances.size(); }

    void SetFilterSlopeEnums(UINT32 SlopeUIndex, UINT32 SlopeVIndex, UINT32 OffsetUIndex, UINT32 OffsetVIndex);

    void SetNonResidentColor(XMVECTOR v)
    {
        XMStoreFloat4(&m_NonResidentColor, v);
    }
    void SetStochasticConstants(FLOAT Seed, FLOAT Fraction)
    {
        m_StochasticConstants = XMFLOAT4(Fraction, Seed, 0, 0);
    }

    void Render(XMMATRIX matView, XMMATRIX matProjection, ID3D12GraphicsCommandList* pCmdList, ID3D12CommandQueue* pCmdQueue);

    SceneObjectInstance* FindSceneObjectInstance(XMVECTOR RayOrigin, XMVECTOR RayDirection) const;

    ID3D12DescriptorHeap* GetDescriptorHeap() { return m_InstanceDescriptors.GetDescriptorHeap(); }

    D3D12_CPU_DESCRIPTOR_HANDLE GetInstanceCpuDescriptor(SceneObjectInstance* pInstance, UINT32 TextureIndex, UINT32 ComponentIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstanceGpuDescriptor(SceneObjectInstance* pInstance, UINT32 TextureIndex, UINT32 ComponentIndex);

private:
    FeedbackTexture* FindOrCreateFeedbackTexture(StreamingTexture* pST);
    void UpdateViewDescriptors(SceneObjectInstance* pInstance);
};

extern SceneGraph g_SceneGraph;
