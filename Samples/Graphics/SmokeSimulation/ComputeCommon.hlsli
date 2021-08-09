//--------------------------------------------------------------------------------------
// ComputeCommon.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FluidShared.h"

RWTexture3D<float> result1 : register(u0);
RWTexture3D<float4> result4 : register(u0);
RWTexture3D<float> result_mip : register(u1);

Texture3D tex0 : register(t0);
Texture3D tex1 : register(t1);

SamplerState samLinear : register(s0);

// Helper function to "advect" sampling position according to current velocity and time step
float3 GetAdvectedPosTexCoords(uint3 coord)
{
    float3 pos = coord;
    pos -= DeltaTime * Forward * tex0[coord].xyz;
    return float3((pos.x + 0.5) / (VolumeSize), (pos.y + 0.5) / (VolumeSize), (pos.z + 0.5) / (VolumeSize));
}

// Helper function which returns 6 neighbour value for a given point, clamp to edge if necessary, float4 version
void ReadLRBTDU(uint3 GTid, uint3 DTid, out float4 L, out float4 R, out float4 B, out float4 T, out float4 D, out float4 U)
{
    L = DTid.x == 0 ? tex0[DTid] : tex0[DTid + uint3(-1, 0, 0)];
    R = DTid.x == (VolumeSize - 1) ? tex0[DTid] : tex0[DTid + uint3(1, 0, 0)];
    B = DTid.y == 0 ? tex0[DTid] : tex0[DTid + uint3(0, -1, 0)];
    T = DTid.y == (VolumeSize - 1) ? tex0[DTid] : tex0[DTid + uint3(0, 1, 0)];
    D = DTid.z == 0 ? tex0[DTid] : tex0[DTid + uint3(0, 0, -1)];
    U = DTid.z == (VolumeSize - 1) ? tex0[DTid] : tex0[DTid + uint3(0, 0, 1)];
}

// Helper function which returns 6 neighbour value for a given point, clamp to edge if necessary, float version
void ReadLRBTDU1(uint3 GTid, uint3 DTid, out float L, out float R, out float B, out float T, out float D, out float U)
{
    L = DTid.x == 0 ? tex0[DTid].x : tex0[DTid + uint3(-1, 0, 0)].x;
    R = DTid.x == (VolumeSize - 1) ? tex0[DTid].x : tex0[DTid + uint3(1, 0, 0)].x;
    B = DTid.y == 0 ? tex0[DTid].x : tex0[DTid + uint3(0, -1, 0)].x;
    T = DTid.y == (VolumeSize - 1) ? tex0[DTid].x : tex0[DTid + uint3(0, 1, 0)].x;
    D = DTid.z == 0 ? tex0[DTid].x : tex0[DTid + uint3(0, 0, -1)].x;
    U = DTid.z == (VolumeSize - 1) ? tex0[DTid].x : tex0[DTid + uint3(0, 0, 1)].x;
}

#define ComputeRS \
"CBV(b0, visibility=SHADER_VISIBILITY_ALL),\
        DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
        DescriptorTable(SRV(t1, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
        DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
        DescriptorTable(UAV(u1, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
        StaticSampler(s0,\
                      addressU = TEXTURE_ADDRESS_BORDER,\
                      addressV = TEXTURE_ADDRESS_BORDER,\
                      addressW = TEXTURE_ADDRESS_BORDER,\
                      comparisonFunc = COMPARISON_NEVER,\
                      borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,\
                      filter = FILTER_MIN_MAG_MIP_LINEAR,\
                      visibility=SHADER_VISIBILITY_ALL)"
