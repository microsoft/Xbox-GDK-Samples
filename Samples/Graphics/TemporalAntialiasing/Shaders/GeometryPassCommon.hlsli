//--------------------------------------------------------------------------------------
// GeometryPassCommon.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignatures.hlsli"

#define HLSL_INCLUDE
#include "../SharedDataTypes.h"

struct VSInputs
{
    float3 position     : POSITION;
    float3 normals      : NORMALS;
    float2 texcoords    : TEXCOORDS;
};

struct PSInterpolators
{
    float4 position             : Sv_Position;
    float4 normals              : NORMALS;
    float2 texcoords            : TEXCOORDS0;
    float3 previousFrameClip    : TEXCOORDS1;
    float3 currentFrameClip     : TEXCOORDS2;
};

struct PSOutputs
{
    float4 gbufferAlbedo     : SV_TARGET0;
    float4 gbufferNormals    : SV_TARGET1;
    float2 gbufferMotion     : SV_TARGET2;
};
