//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Root signature for DepthPrePass and ColorPass
// Root 0   - CVB (matrices)
// Root 1   - Table for texture SRV (1 texture for this model)
#define ROOT_SIGNATURE   "\
    RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT \
               | DENY_DOMAIN_SHADER_ROOT_ACCESS \
               | DENY_HULL_SHADER_ROOT_ACCESS), \
    CBV(b0, space = 0), \
    DescriptorTable (SRV(t0, numDescriptors  = 1), visibility=SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL)"

// Root signature for Compute shader resolve
// Root 0   - Table for texture SRV (1 for color buffer, other for fmask)
// Root 1   - Table for texture UAV (for writing resolved color)
#define ROOT_SIGNATURE_COMPUTE   "\
    CBV(b0, space = 0), \
    DescriptorTable ( \
        SRV(t0, numDescriptors  = 1)), \
    DescriptorTable ( \
        UAV(u0, numDescriptors  = 1)), \
    StaticSampler(s0, \
       filter =   FILTER_MIN_MAG_MIP_LINEAR, \
       addressU = TEXTURE_ADDRESS_CLAMP, \
       addressV = TEXTURE_ADDRESS_CLAMP, \
       addressW = TEXTURE_ADDRESS_CLAMP, \
       visibility = SHADER_VISIBILITY_ALL)"

// Input and Outputs
struct Interpolants
{
    float4 position : SV_Position;
};

struct VSIn
{
    float3 position : SV_Position0;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
};

// Constant Buffers
struct SceneConstantsCB
{
    float4x4 mvp;
};
