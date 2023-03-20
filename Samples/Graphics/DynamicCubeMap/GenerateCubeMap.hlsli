//--------------------------------------------------------------------------------------
// GenerateCubeMap.hlsli
//
// Common code for generating a cube map
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ConstantBuffer.h"

struct GS_CUBEMAP_IN
{
    float4 Pos		: SV_POSITION;    // World position
    float2 Tex		: TEXCOORD0;      // Texture coord
};

struct PS_CUBEMAP_IN
{
    float4 Pos      : SV_POSITION;     // Projection coord
    float2 Tex      : TEXCOORD0;       // Texture coord
    uint   RTIndex  : SV_RenderTargetArrayIndex;
};

struct MS_CUBEMAP_VERT_OUT
{
    float4 Pos      : SV_POSITION;     // Projection coord
    float2 Tex      : TEXCOORD0;       // Texture coord
};

struct MS_CUBEMAP_PRIM_OUT
{
    uint   RTIndex  : SV_RenderTargetArrayIndex;
};

struct MeshInfo
{
    uint IndexBytes;
    uint MeshletCount;
    uint LastMeshletSize;
};

struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};

struct Payload
{
    uint meshletIndex;
    uint faceIndex[6];
};

struct CullData
{
    float4 BoundingSphere;
    uint   NormalCone;
    float  ApexOffset;
};

ConstantBuffer<CBMultiPerFrame> constants : register(b0);
ConstantBuffer<CBPerCubeRender> cubeMapConstants : register(b1);
ConstantBuffer<MeshInfo> meshInfo : register(b2);
ConstantBuffer<SubMeshlet> subMeshletInfo : register(b3);
ConstantBuffer<VBLayout> vertexInfo : register(b3);
Texture2D txDiffuse : register(t0);
ByteAddressBuffer Vertices : register(t0);
StructuredBuffer<Meshlet> Meshlets : register(t1);
ByteAddressBuffer UniqueVertexIndices : register(t2);
StructuredBuffer<uint> PrimitiveIndices : register(t3);
StructuredBuffer<CullData> MeshletCullData : register(t4);
SamplerState samLinear : register(s0);

#ifdef __XBOX_SCARLETT
#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS" \
    "          | XBOX_FORCE_MEMORY_BASED_ABI)," \
    "CBV(b0),"\
    "CBV(b1),"\
    "CBV(b2),"\
    "DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
    "SRV(t0, visibility=SHADER_VISIBILITY_MESH),"\
    "SRV(t0, visibility=SHADER_VISIBILITY_VERTEX),"\
    "SRV(t1, visibility=SHADER_VISIBILITY_MESH),"\
    "SRV(t2, visibility=SHADER_VISIBILITY_MESH),"\
    "SRV(t3, visibility=SHADER_VISIBILITY_MESH),"\
    "SRV(t4, visibility=SHADER_VISIBILITY_AMPLIFICATION),"\
    "RootConstants(num32BitConstants=3, b3),"\
    "StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL)"
#else
#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "CBV(b0),"\
    "CBV(b1),"\
    "CBV(b2),"\
    "DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
    "SRV(t0, visibility=SHADER_VISIBILITY_MESH),"\
    "SRV(t0, visibility=SHADER_VISIBILITY_VERTEX),"\
    "SRV(t1, visibility=SHADER_VISIBILITY_MESH),"\
    "SRV(t2, visibility=SHADER_VISIBILITY_MESH),"\
    "SRV(t3, visibility=SHADER_VISIBILITY_MESH),"\
    "SRV(t4, visibility=SHADER_VISIBILITY_AMPLIFICATION),"\
    "RootConstants(num32BitConstants=3, b3)," \
    "StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL)"
#endif
