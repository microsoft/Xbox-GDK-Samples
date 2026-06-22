//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Passthrough of color
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

cbuffer cbTransform : register(b0)
{
    float4x4    g_mWorldViewProj;
    float4x4    g_mWorld;
};

struct VertexColor
{
    float4 position     : POSITION0;
    float4 color        : COLOR0;
};

struct InterpolantsColor
{
    float4 position     : SV_POSITION0;
    float4 color        : COLOR0;
};

//-------------------------------------------------------------------------------------------------------------
// Name: VSColor()
// Desc: Draw mesh with vertex colors
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
InterpolantsColor main(VertexColor In)
{
    InterpolantsColor Out;

    Out.position = mul(In.position, g_mWorldViewProj);
    Out.color = In.color;

    return Out;
}

