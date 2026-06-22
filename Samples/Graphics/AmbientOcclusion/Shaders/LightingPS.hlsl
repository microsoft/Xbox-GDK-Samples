//------------------------------------------------------------------------------------
// LightingPS.hlsl
//
// Simple shader to render lit geometry
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Lighting.hlsli"

static const float3 ambientColor = float3(0.2f, 0.2f, 0.2f);
static const float3 lightDir = float3(0, 1, 1);

[RootSignature(MainRS)]
Pixel main(Interpolants In)
{
    Pixel Out;
    float4 diffuse = texDiffuse.Sample(samLinear, In.texcoord);
    uint2 pixelPos = uint2(In.position.xy);
    float ao = texAO[pixelPos];
    float3 ambient = lerp(1.0f, ao, constants.aoMultiplier) * diffuse.rgb * ambientColor;
    float nDotL = max(0.0f, dot(normalize(In.normal), normalize(lightDir)));
    Out.color = float4(nDotL * diffuse.rgb + ambient, 1.0f);
    return Out;
}
