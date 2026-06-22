//--------------------------------------------------------------------------------------
// HLSLCommon.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// RS shared by all opaque and translucency passes.
#define MAIN_RS " \
    RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), \
    CBV(b0), \
    CBV(b1), \
    CBV(b2), \
    DescriptorTable( SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL), \
    DescriptorTable( UAV(u0, numDescriptors = 2), visibility = SHADER_VISIBILITY_PIXEL), \
    DescriptorTable( UAV(u2, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT)"

// RS shared by the two blend techniques (PPLL, MLAB) composite passes.
#define COMPOSITE_RS " \
    CBV(b0), \
    DescriptorTable( SRV(t0, numDescriptors = 2)), \
    DescriptorTable( UAV(u0, numDescriptors = 1))"

#define DEPTH_MAX 0xffffff
#define TRANSMISSION_MAX 0xff

struct VertexInputs
{
    float3 position     : POSITION;
    float2 texcoords    : TEXCOORDS0;
};

struct Interpolators
{
    float4 position     : SV_POSITION;
    float2 texcoords    : TEXCOORDS;

    // Passing viewSpace depth to sort the fragments.
    float viewDepth     : VIEWDEPTH;
};

// https://github.com/GameTechDev/AOIT-Update/blob/master/OIT_DX11/AOIT%20Technique/AOIT.hlsl
// ToRGBE - takes a float RGB value and converts it to a float RGB value with a shared exponent
float4 ToRGBE(float4 inColor)
{
    float base = max(inColor.r, max(inColor.g, inColor.b));
    int e;
    float m = frexp(base, e);
    return float4(saturate(inColor.rgb / exp2(e)), e + 127);
}

// https://github.com/GameTechDev/AOIT-Update/blob/master/OIT_DX11/AOIT%20Technique/AOIT.hlsl
// PackRGBA takes a float4 value and packs it into a UINT (8 bits / float)
uint PackRGBA(float4 unpackedInput)
{
    uint4 u = (uint4)(unpackedInput * float4(255, 255, 255, 1));
    uint packedOutput = (u.w << 24UL) | (u.z << 16UL) | (u.y << 8UL) | u.x;
    return packedOutput;
}

// https://github.com/GameTechDev/AOIT-Update/blob/master/OIT_DX11/AOIT%20Technique/AOIT.hlsl
// FromRGBE takes a float RGB value with a shared exponent and converts it to a float RGB value
float4 FromRGBE(float4 inColor)
{
    return float4(inColor.rgb * exp2(inColor.a - 127), inColor.a);
}

// https://github.com/GameTechDev/AOIT-Update/blob/master/OIT_DX11/AOIT%20Technique/AOIT.hlsl
// UnpackRGBA takes a uint value and converts it to a float4
float4 UnpackRGBA(uint packedInput)
{
    float4 unpackedOutput;
    uint4 p = uint4((packedInput & 0xFFUL),
        (packedInput >> 8UL) & 0xFFUL,
        (packedInput >> 16UL) & 0xFFUL,
        (packedInput >> 24UL));

    unpackedOutput = ((float4)p) / float4(255, 255, 255, 1.0f);
    return unpackedOutput;
}
