//--------------------------------------------------------------------------------------
// ConvertToHDR10VS.hlsl
//
// Simple full-screen triangle vertex shader for render full-screen shaders in PS.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE \
    "DescriptorTable(SRV(t0, numDescriptors=1)), \
     StaticSampler(s0, \
             filter = FILTER_MIN_MAG_MIP_POINT, \
             addressU = TEXTURE_ADDRESS_CLAMP, \
             addressV = TEXTURE_ADDRESS_CLAMP, \
             addressW = TEXTURE_ADDRESS_CLAMP, \
             visibility = SHADER_VISIBILITY_PIXEL)"

// Clockwise triangle
static const float4 Positions[3] =
{
    float4(-1, 1, 0, 1),
    float4(3, 1, 0, 1),
    float4(-1, -3, 0, 1),
};

static const float2 Texcoords[3] =
{
    float2(0, 0),
    float2(2, 0),
    float2(0, 2),
};

struct Interpolators
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

[RootSignature(ROOT_SIGNATURE)]
Interpolators main(uint VertexId : SV_VertexId)
{
    Interpolators Out;
    Out.Position = Positions[VertexId];
    Out.TexCoord = Texcoords[VertexId];
    return Out;
}
