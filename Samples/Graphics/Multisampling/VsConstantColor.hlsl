//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Position-only vertex shader
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

struct VertexConstantColor
{
    float4 position     : POSITION0;
};

struct InterpolantsConstantColor
{
    float4 position     : SV_POSITION0;
};

//-------------------------------------------------------------------------------------------------------------
// Name: VSConstantColor()
// Desc: Draw a constant color
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
InterpolantsConstantColor main(VertexConstantColor In)
{
    InterpolantsConstantColor Out;

    Out.position = mul(In.position, g_mWorldViewProj);

    return Out;
}
