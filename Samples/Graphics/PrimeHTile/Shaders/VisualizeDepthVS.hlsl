//--------------------------------------------------------------------------------------
// VisualiseDepthVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "VisualizeDepth.hlsl"

cbuffer Parameters : register(b0)
{
    row_major float4x4 MatrixTransform;
};

//-------------------------------------------------------------------------------------------------------------
// Custom vertex shader for sprite batch
//-------------------------------------------------------------------------------------------------------------
[RootSignature(VisualizeDepthRS)]
void SpriteVertexShader(inout float4 color    : COLOR0,
    inout float2 texCoord : TEXCOORD0,
    inout float4 position : SV_Position)
{
    position = mul(position, MatrixTransform);
}
