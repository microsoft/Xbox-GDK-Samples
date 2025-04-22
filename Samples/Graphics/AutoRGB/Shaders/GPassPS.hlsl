//--------------------------------------------------------------------------------------
// GPassPS.hlsl
//
// Pixel Shader for the GPass (renders into GBuffer for deferred).
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "../shared.h"
#include "GPassCommon.hlsli"

Texture2D<float4> texDiffuse : register(t0);

sampler diffuseSampler : register(s0);

//--------------------------------------------------------------------------------------
// Name: DeriveNormalFromUnsignedXY()
// Desc: Derives a signed 3-component normal vector from an unsigned two-component
//       vector.
//--------------------------------------------------------------------------------------
float3 DeriveNormalFromUnsignedXY(float2 vNormalUXY)
{
    // Get XYZ from XY.
    float2 SN = vNormalUXY * 2.0f - 1.0f;
    float3 vUSN = float3(SN, sqrt(1.0f - dot(SN, SN)));

    // Now move into signed territory.
    return vUSN;
}


[RootSignature(ROOT_SIGNATURE)]
DeferredOut main(in SceneInterpolants In)
{
    DeferredOut Out;

    Out.Albedo = texDiffuse.Sample(diffuseSampler, In.UV);

    // Sample and convert the normal from our normal map.
    float3 normal = In.WorldNormal;

    // Transform the normal from tangent space to world space.
    float3x3 BTNMatrix = float3x3(In.WorldBinormal, In.WorldTangent, In.WorldNormal);
    Out.Normal = float4(normalize(normal), 0.0f);
    Out.Position = float4(In.WorldPosition, 0.0f);

    return Out;
}
