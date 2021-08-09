//--------------------------------------------------------------------------------------
// ParticleCommon.hlsli
//
// Root signature and common types, functions, and resources for particle rendering.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.h"

struct ParticleInterpolants
{
    float4 Position         : SV_Position; // Position 
    float2 TextureUV        : TEXCOORD0;   // Texture coordinates
    float4 Color	        : COLOR0;      // Color
};

SamplerState g_sampLinear : register(s0); // Bilinear interpolation sampler static sampler (specified below).

// Common root signature for both ParticleVS & ParticlePS shaders
#define ParticleRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),  \
    CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), \
    DescriptorTable(SRV(t0, numDescriptors = 2), visibility = SHADER_VISIBILITY_VERTEX),  \
    DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL),  \
    StaticSampler(s0, \
    filter = FILTER_MIN_MAG_LINEAR_MIP_POINT,  \
    addressU = TEXTURE_ADDRESS_BORDER,  \
    addressV = TEXTURE_ADDRESS_BORDER,  \
    addressW = TEXTURE_ADDRESS_BORDER,  \
    borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, \
    comparisonFunc = COMPARISON_ALWAYS,  \
    maxLOD = 3.402823466e+38f)"
