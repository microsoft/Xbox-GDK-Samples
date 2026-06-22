//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Common code for rendering the scene
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Constants.h"

struct Interpolants
{
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 worldPos : COLOR0;
    float2 texcoord : TEXCOORD0;
};

struct Pixel
{
    float4 color : SV_Target;
};

StructuredBuffer<MeshInfo> g_meshInfo : register(t0);
ConstantBuffer<SceneConstants> g_sceneCB : register(b0);
ConstantBuffer<MeshConstants> g_meshCB : register(b1);

#define MainRS \
    "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED" \
    "          | SAMPLER_HEAP_DIRECTLY_INDEXED" \
    "          | DENY_AMPLIFICATION_SHADER_ROOT_ACCESS" \
    "          | DENY_MESH_SHADER_ROOT_ACCESS" \
    "          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "CBV(b0),"\
    "RootConstants(num32BitConstants=20, b1)," \
    "DescriptorTable(SRV(t0)),"
