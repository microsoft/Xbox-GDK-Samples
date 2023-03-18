//--------------------------------------------------------------------------------------
// InstancedLightVolumesCommon.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Vertex Input
struct LightVolumeInstancedVertex
{
    float3 Position : POSITION;
};

// Interpolants (VS out, PS in)
struct InstancedLightVolumeInterpolants
{
    float4 Position :       SV_Position;
    float4 Color :          TEXCOORD0;
    float3 LightWorldPos :  TEXCOORD1;
    float Range :           TEXCOORD2;
};
