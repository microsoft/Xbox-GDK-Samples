//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FullScreenTriangleVS.hlsl"
#include "AtmosphericScattering.hlsli"


[RootSignature(GlobalRS)]
float4 SkyRenderPS(VS_OUTPUT input) : SV_Target
{
    float3 viewDirection = ComputeDirection(input.uv);
    float cosSunAngle = saturate(dot(viewDirection, lightDir));  
    float invZenithAngle = CalcInvZenithAngle(viewDirection.y);

    float3 extinctionSky;
    float3 inScatterSky = AtmosphericScattering(invZenithAngle, 1.0, cosSunAngle, lightDir.y, extinctionSky);
    float3 skyColour = inScatterSky * (1.0 - extinctionSky);

    skyColour += skyColour * skyColour * cosSunAngle;       // fake HDR/bloom
    skyColour  = ToneMap(skyColour);

    return float4(skyColour, 1.0);
}

