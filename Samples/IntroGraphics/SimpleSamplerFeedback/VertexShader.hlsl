//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer Constants : register(b0)
{
    float4x4 mWorld;
    float4x4 mView;
    float4x4 mProjection;
};

[RootSignature(MainRS)]
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, mWorld);
    output.Pos = mul(output.Pos, mView);
    output.Pos = mul(output.Pos, mProjection);
    output.TexCoord = input.TexCoord;
    return output;
}

