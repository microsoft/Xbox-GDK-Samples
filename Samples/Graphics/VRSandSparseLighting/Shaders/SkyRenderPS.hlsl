//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define __XBOX_ENABLE_WAVE32

#include "FullScreenTriangleVS.hlsl"
#include "AtmosphericScattering.hlsli"
#include "DeferredCommon.hlsli"


[RootSignature(GlobalRS)]
float3 SkyRenderPS(VS_OUTPUT input) : SV_Target
{
    float3 viewDirection = ComputeViewDirection(input.uv);
    float cosSunAngle = saturate(dot(viewDirection, lightDir));  
    float invZenithAngle = CalcInvZenithAngle(viewDirection.y);

    float3 extinctionSky;
    float3 inScatterSky = AtmosphericScattering(invZenithAngle, 1.0, cosSunAngle, lightDir.y, extinctionSky);
    float3 skyColour = inScatterSky * (1.0 - extinctionSky);

    return skyColour;
}

