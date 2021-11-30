//------------------------------------------------------------------------------------
// VisibilityVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "common.hlsli"

// Enable SV_PrimitiveID, must be enabled in vertex shader to be passed to pixel shader on Xbox.
#define __XBOX_ENABLE_PSPRIMID

struct Interpolants
{
    float4 position     : SV_Position;
};

struct Vertex
{
    float3 position : SV_Position0;
    float2 uv       : TEXCOORD0;
};

ConstantBuffer<ConstantsVis> constantInfo : register(b0);

[RootSignature(MainRSVis)]
Interpolants main(Vertex In )
{
    Interpolants Out;

    Out.position = mul(constantInfo.mvpMatrix, float4(In.position, 1.0f));
    
    return Out;
}
