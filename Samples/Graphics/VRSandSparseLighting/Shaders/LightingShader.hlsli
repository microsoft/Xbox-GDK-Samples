//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Global.hlsli"
#include "DeferredCommon.hlsli"
#include "SparseLightingCommon.hlsli"
#include "AtmosphericScattering.hlsli"
#include "TerrainRaycast.hlsli"


#ifndef __XBOX_ENABLE_WAVE32         // make sure we are calling this with correct wave size
# error "Wave32 required"
#endif



#define MAX_DIST_ATMOSPHERE                 400.0

#define HEIGHT_FOG_HEIGHT                   300.0
#define HEIGHT_FOG_VERTICAL_EXTINCTION      16.0
#define HEIGHT_FOG_STRENGTH                 10.0

#define SUN_LIGHT_STRENGTH                  4.0
#define SUN_LIGHT_SPEC_POWER                50.0



// although common resolutions (2160p, 1440p) do divide into exactly 16 (lighting tile size)
// better to use a minimal group size for DRS
[RootSignature(GlobalRS)]
[numthreads(8, 4, 1)]
void DoLighting(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID)
{
    // extra registers introduced by sparse lighting for duration of the shader
    bool sparseLightingEnabled;     // scalar
    uint sparseData;                // vector (though we only need 3x copy bits, so if you're tight on VGPRs, maybe this register could store extra data also)

    float depth01;
    uint2 coord;

    // call prefix for 8x4 wave size
    if (!SparseLightingPrefix84(Gid, GTid, coord, depth01, sparseLightingEnabled, sparseData, true))
        return;

    // Ok, that's all the sparse stuff done until the very end, the rest of this is the regular deferred lighting shader
    float4 gbufferRoughnessAndNormal = GBufferRoughnessNormalSRV[coord];
    float3 albedo = GbufferAlbedoSRV[coord];

    // could be done as (float2(coord) + 0.5) * invRenderTargetDim, but doing it this way allows use of a vector madd instruction, and extra scalar loads are essentially free
    float2 screenSpaceUv = float2(coord) * invRenderTargetDim + renderTargetHalfPixelOffset;
    float3 viewDir;
    float3 worldPos = ComputeWorldPositionAndViewDirection(screenSpaceUv, depth01, viewDir);
    float dist = depth01 * farZ;

    // decode g-buffers
    float3 normal = DecodeNormal(gbufferRoughnessAndNormal);
    float roughness = gbufferRoughnessAndNormal.x;

    // light
    float NL = dot(normal, lightDir);
    float shadow = 0.0;
    float diffuse = 0.0;
    float3 raytraceDebug = float3(0.0, 0.0, 0.0);

    if (NL > 0.0)
    {
        shadow = RayTraceTerrain(worldPos, rayMarchLightDir, raytraceDebug);
        diffuse = NL;
    }
    float specular = pow(saturate(dot(normal, normalize(lightDir - viewDir))), SUN_LIGHT_SPEC_POWER);  // blinn-phong, but not normalised (the shame!)
    float3 colour = shadow * albedo * (diffuse + roughness * specular) * SUN_LIGHT_STRENGTH;

    // atmospheric scattering
    float cosSunAngle = saturate(dot(viewDir, lightDir));
    float invZenithAngle = CalcInvZenithAngle(viewDir.y);

    float3 extinctionSky, extinctionGround;
    float3 inScatterSky = AtmosphericScattering(invZenithAngle, 1.0, cosSunAngle, lightDir.y, extinctionSky);

    // reduce the Mie disc on close up ground, not a great solution.
    // TODO some sort of % visibility of the sun disc to modulate the visibility of the sun per pixel
    float atmosphereDist = saturate(dist / MAX_DIST_ATMOSPHERE);
    float attenuation = sqrt(atmosphereDist);
    float t1 = attenuation * pow(1.0 - pow(1.0 - attenuation, 10.0), 50.0);
    float t2 = pow(attenuation, 4.0);
    float3 inScatterGround = AtmosphericScattering(t1, t2, pow(cosSunAngle, 3.0), lightDir.y, extinctionGround);
    float3 skyColour = inScatterSky * (1.0 - extinctionSky);
    colour = lerp(inScatterGround * skyColour, colour, extinctionGround);

    // hermispherical lighting, calculate the light as seen not from the camera view direction but from the surface normal
    invZenithAngle = CalcInvZenithAngle(normal.y, 0.0);
    inScatterSky = AtmosphericScattering(invZenithAngle, 1.0, saturate(dot(normal, lightDir)), lightDir.y, extinctionSky);
    float3 ambient = 0.25 * saturate(inScatterSky * (1.0 - extinctionSky));

    // add some extra ambient in the disc of the sun at low angles
    ambient += atmosphereDist * 0.125 * ((1.0 - lightDir.y) * skyColour);

    // add in the ambient
    colour += 0.1 + saturate(ambient) * albedo;
    
    // height fog
    float invFogHeight = 1.0 - saturate(worldPos.y / HEIGHT_FOG_HEIGHT);
    float heightFogStrength = min(0.75, pow(invFogHeight, HEIGHT_FOG_VERTICAL_EXTINCTION));     // height
    heightFogStrength *= pow(cosSunAngle, 8.0) * (saturate(0.5 - lightDir.y) * 2.0);            // angle with the sun
    heightFogStrength *= atmosphereDist;                                                        // distance
    float3 heightFog = saturate(sqrt(skyColour));
    heightFog *= heightFog;
    heightFog *= heightFogStrength;

    colour += heightFog;

    // Sparse stuff again, do hole filling
    SparseLightingPostfix(LitOutputUAV, sparseLightingEnabled, sparseData, coord, colour);
}
