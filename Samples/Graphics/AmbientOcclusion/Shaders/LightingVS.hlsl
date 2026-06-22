//--------------------------------------------------------------------------------------
// LightingVS.hlsl
//
// Simple vertex shader for rendering geometry
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Lighting.hlsli"

struct VSInput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL0;
    float2 TexCoord : TEXCOORD0;
};

[RootSignature(MainRS)]
Interpolants main(VSInput In)
{
    Interpolants Out = (Interpolants)0.0f;

    // Output position
    Out.position = mul(In.Position, constants.WorldViewProj);
    Out.normal = mul(float4(In.Normal, 0.f), constants.WorldInverseTranspose).xyz;
    Out.texcoord = In.TexCoord;

    return Out;
}
