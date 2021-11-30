//--------------------------------------------------------------------------------------
// CommonHLSL.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#ifndef HLSL
typedef uint32_t uint;
typedef DirectX::XMFLOAT4X4 float4x4;
typedef DirectX::XMFLOAT3 float3;
#endif

static const uint s_maxObjects = 50;

#if defined(_GAMING_XBOX) || defined(__XBOX_ONE) || defined(__XBOX_SCARLETT)
#define MESHLET_MAX_VERTICES 256
#define MESHLET_MAX_PRIMITIVES 256
#else
#define MESHLET_MAX_VERTICES 128
#define MESHLET_MAX_PRIMITIVES 128
#endif

namespace Descriptors
{
    enum Value
    {
        DragonTexture,
        ConcreteTexture,
        VisibilityBuffer,
        FontSmall,
        FontBig,
        ControllerFont,
        OutputUAV,
        OutputUAV2,
        OutputSRV,
        OutputSRV2,
        ObjectInfoBuffer,
        VertexBuffer,
        IndexBuffer = VertexBuffer + s_maxObjects,
        MeshletBuffer = IndexBuffer + s_maxObjects,
        UniqueIndices = MeshletBuffer + s_maxObjects,
        PrimitiveIndices = UniqueIndices + s_maxObjects,
        Count = PrimitiveIndices + s_maxObjects
    };
}

namespace Samplers
{
    enum Value
    {
        LinearSampler,
        PointSampler,
        Count
    };
}

namespace OverlayModes
{
    enum Value
    {
        None,
        PrimitiveID,
        ObjectID,
        MeshletID,
        Count
    };
}

struct ConstantsVis
{
    float4x4        mvpMatrix;
    uint            objectIndex;
    uint            vertexBufferIndex;
    uint            drawOverlay;
};

struct ConstantElement
{
    float4x4        inverseViewProjection;
    float3          cameraPosition;
    uint            uavIndex;
    uint            drawOverlay;
};

struct ObjectInfo
{
    uint            vertexBufferID;
    uint            indexBufferID;
    uint            normalBufferID;
    uint            materialID;
    float4x4        modelTransform;
    uint            textureIDs[6];
};

struct MeshletDesc
{
    uint startUniqueVertex;
    uint startPrimitiveIndex;
    uint numVertices;
    uint numPrimitives;
};
