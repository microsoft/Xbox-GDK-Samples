//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Renders a textured mesh
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

struct VertexTexture
{
    float4 position     : POSITION0;
    float2 texcoord     : TEXCOORD0;
};

struct InterpolantsTexture
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
};

//-------------------------------------------------------------------------------------------------------------
// Name: VSTexture()
// Desc: Draw a textured mesh
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
InterpolantsTexture main(VertexTexture In)
{
    InterpolantsTexture Out;

    Out.position = mul(In.position, g_mWorldViewProj);
    Out.texcoord = In.texcoord;

    return Out;
}

