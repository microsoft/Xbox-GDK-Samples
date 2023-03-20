//--------------------------------------------------------------------------------------
// AmbientPS.hlsl
//
// Pixel Shader for the Ambient Pass.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "shared.hlsli"

Texture2D<float4> texAlbedo : register(t0);

struct Interpolators
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

[RootSignature(ROOT_SIGNATURE_AMBIENT)]
float4 main(in Interpolators In) : SV_Target
{
    float3 loadpos = float3(In.Position.xy, 0.0f);
    float3 albedo = texAlbedo.Load(loadpos).xyz;
    return float4(albedo * AMBIENT_INTENSITY, 1.0f);
}
