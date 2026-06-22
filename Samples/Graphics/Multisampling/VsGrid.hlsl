//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Vertex shader for rendering horizontal and vertical grid lines
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

struct InterpolantsColor
{
    float4 position     : SV_POSITION0;
    float4 color        : COLOR0;
};

cbuffer cbGrid : register(b2)
{
    float4      g_vGridColor;
    bool        horizontal;
};

// This is a different HLSL type for each MSAA_LEVEL, which means we need a separate instance of the shader.
// The only uses of the texture are for GetDimensions and GetSamplePosition; we never sample from it.
Texture2DMS<float4> texMS : register(t0);

#include "EQAA.hlsli"

//-------------------------------------------------------------------------------------------------------------
// Name: VSGrid()
// Desc: Draw a pixel-aligned grid
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
InterpolantsColor main(uint instance : SV_InstanceID, uint vertex : SV_VertexID)
{
    InterpolantsColor Out;

    uint2 dims;
    uint samples;
    GetDimensions(texMS, dims.x, dims.y, samples);

    float4 position = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (horizontal)
    {
        position.x = (vertex == 0) ? -1.0f : 1.0f;
        position.y = 2.0f * (instance / (float)dims.y) - 1.0f;
    }
    else
    {
        position.x = 2.0f * (instance / (float)dims.x) - 1.0f;
        position.y = (vertex == 0) ? -1.0f : 1.0f;
    }

    Out.position = mul(position, g_mWorldViewProj);
    Out.color = g_vGridColor;

    return Out;
}
