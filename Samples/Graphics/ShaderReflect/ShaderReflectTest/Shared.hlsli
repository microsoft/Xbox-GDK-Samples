//--------------------------------------------------------------------------------------
// Shared.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.h"

#define ParticleRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), \
    CBV(b0),\
    CBV(b1),\
    DescriptorTable(SRV(t0, numDescriptors = 2), visibility = SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s0, \
                  filter = FILTER_MIN_MAG_MIP_LINEAR,  \
                  addressU = TEXTURE_ADDRESS_MIRROR,  \
                  addressV = TEXTURE_ADDRESS_CLAMP,  \
                  addressW = TEXTURE_ADDRESS_CLAMP,  \
                  borderColor = STATIC_BORDER_COLOR_OPAQUE_BLACK, \
                  comparisonFunc = COMPARISON_NEVER,  \
                  maxLOD = 3.402823466e+38f, \
                  visibility = SHADER_VISIBILITY_PIXEL)"

#define SceneRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), \
    CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), \
    CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), \
    CBV(b1, visibility = SHADER_VISIBILITY_PIXEL), \
    DescriptorTable(SRV(t0, numDescriptors = 2), visibility = SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s0, \
                  filter = FILTER_MIN_MAG_MIP_LINEAR,  \
                  addressU = TEXTURE_ADDRESS_MIRROR,  \
                  addressV = TEXTURE_ADDRESS_CLAMP,  \
                  addressW = TEXTURE_ADDRESS_CLAMP,  \
                  comparisonFunc = COMPARISON_NEVER,  \
                  borderColor = STATIC_BORDER_COLOR_OPAQUE_BLACK, \
                  visibility = SHADER_VISIBILITY_PIXEL)"

// Input Vertex Definitions
struct Vertex
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL0;
    float2 UV       : TEXCOORD0;
    float3 Tangent  : TANGENT0;
};

struct ParticleVertex
{
    float3 Position   : POSITION;
    float2 TextureUV  : TEXCOORD0;
    float  Life       : LIFE;
    float  Rot        : THETA;
    float4 Color	  : COLOR0;
};

// Shader Interpolant Definitions
struct SceneInterpolants
{
    float4 Position      : SV_Position;
    float2 UV            : TEXCOORD0;
    float3 WorldNormal   : TEXCOORD1;
    float3 WorldTangent  : TEXCOORD2;
    float3 WorldBinormal : TEXCOORD3;
    float3 WorldPosition : TEXCOORD4;
};

struct ScreenInterpolants
{
    float4 Position : SV_POSITION;
};

struct ParticleInterpolants
{
    float4 Position        : SV_Position;
    float3 TextureUVI      : TEXCOORD0;
    float3 SinCosThetaLife : TEXCOORD1;
    float4 Color	       : COLOR0;
};

// MRT Output Definitions
struct DeferredPixel
{
    float4 NormalAndAlpha : SV_Target0;
    float4 Color          : SV_Target1;
};


// Helper functions
float3 DeriveNormalFromUnsignedXY(float2 vNormalUXY)
{
    // Get XYZ from XY...
    float2 SN = vNormalUXY * 2 - 1;
    float3 vUSN = float3(SN, sqrt(1 - dot(SN, SN)));

    // Now move into signed territory...
    return vUSN;
}


ConstantBuffer<cbModel> g_model;
ConstantBuffer<cbScene> g_scene;
ConstantBuffer<cbParticle> g_particle;
ConstantBuffer<cbGlowLights> g_glowLights;

// Resources
Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texNormal  : register(t1);
sampler sampLinear : register(s0);
