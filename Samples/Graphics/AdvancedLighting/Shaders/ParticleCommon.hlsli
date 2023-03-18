//--------------------------------------------------------------------------------------
// ParticleCommon.hlsli
//
// Shaders for rendering particle effects.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "shared.hlsli"

struct ParticleSceneConstCB
{
    float4x4 matViewProj;
};

struct ParticleVertex
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct ParticleInterpolants
{
    float4 Position : SV_Position; // vertex position 
    float2 TextureUV : TEXCOORD0; // vertex texture coords
    float4 Color : COLOR0; // Color...
};
