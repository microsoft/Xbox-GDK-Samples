//--------------------------------------------------------------------------------------
// RasterVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "common.hlsli"

struct Constants
{
    float4x4 mvpMatrix;
    uint textureIndex;
};

struct Interpolants
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

struct Vertex
{
    float3 position : SV_Position0;
    float2 uv       : TEXCOORD0;
};

ConstantBuffer<Constants> constantInfo : register(b0);

[RootSignature(MainRSRaster)]
Interpolants main(Vertex In)
{
    Interpolants Out;

    Out.position = mul(constantInfo.mvpMatrix, float4(In.position, 1.0f));
    Out.uv = In.uv;
    
    return Out;
}
