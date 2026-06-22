//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainRenderVS.hlsl"
#include "AtmosphericScattering.hlsli"
#include "TerrainRaycast.hlsli"

#define MAX_DIST_ATMOSPHERE                 400.0


#define HEIGHT_FOG_HEIGHT                   75.0
#define HEIGHT_FOG_VERTICAL_EXTINCTION      8.0
#define HEIGHT_FOG_STRENGTH                 100.0

#define SPECULAR_STRENGTH                   0.5

[RootSignature(GlobalRS)]
float4 TerrainRenderPS(VS_OUTPUT input) : SV_Target
{
    float3 normal = LoadAndDecodeNormal(input.worldPos);

    // light
    float NL = dot(normal, lightDir);
    float shadow = 0.0;
    float ambient = 0.1 + 0.1 * ((NL * 0.5) + 0.5);
    float specular = 0.0;
    float diffuse = 0.0;

    float3 raytraceDebug = float3(0.0, 0.0, 0.0);
    if (NL > 0.0)
    {
        shadow = RayTraceTerrain(float3(input.worldPos), rayMarchLightDir, raytraceDebug);
        diffuse = NL;
    }
    float albedo = lerp(0.125, 1.5, smoothstep(0.0, 1.0, normal.y - 0.5));
    albedo = pow(albedo, 1.5) * 4.0;
    float roughness = pow(saturate(albedo), 4.0);

    float3 viewDir = input.worldPos - cameraPos;
    float dist = length(viewDir);
    viewDir /= dist;

    specular = pow(saturate(dot(normal, normalize(lightDir - viewDir))), 50.0) * SPECULAR_STRENGTH;  // blinn-phong, but not normalised (the shame!)

    float sunLightIntensity = shadow * 1.5 * (0.5 + (1.0 - lightDir.y));
    float3 colour = ambient + sunLightIntensity * ((albedo * diffuse) + (roughness * specular));

    // atmospheric scattering
    float cosSunAngle = saturate(dot(viewDir, lightDir));
    float invZenithAngle = CalcInvZenithAngle(viewDir.y);

    float3 extinctionSky, extinctionGround;
    float3 inScatterSky = AtmosphericScattering(invZenithAngle, 1.0, cosSunAngle, lightDir.y, extinctionSky);

    // reduce the Mie disc on close up ground, not a great solution.
    // TODO some sort of % visibility of the sun disc to modulate the visibility of the sun per pixel, tricky for a forward renderer!
    float atmosphereDist = saturate(dist / MAX_DIST_ATMOSPHERE);
    float attenuation = sqrt(atmosphereDist);
    float t1 = attenuation * pow(1.0 - pow(1.0 - attenuation, 10.0), 50.0);
    float t2 = pow(attenuation, 4.0);
    float3 inScatterGround = AtmosphericScattering(t1, t2, cosSunAngle * cosSunAngle, lightDir.y, extinctionGround);
    float3 skyColour = inScatterSky * (1.0 - extinctionSky);
    colour = lerp(inScatterGround * skyColour, colour, extinctionGround);

    // AO
    float3 aoDebug;
    float ao = CalculateAO(input.worldPos, normal, aoDebug);

    // height fog
    float invFogHeight = 1.0 - saturate(input.worldPos.y / HEIGHT_FOG_HEIGHT);
    float heightColour = min(0.75, pow(invFogHeight, HEIGHT_FOG_VERTICAL_EXTINCTION));     // height
    heightColour *= pow(cosSunAngle, 16.0) * saturate(1.0 - lightDir.y);                    // angle with the sun
    heightColour *= min(0.15, saturate(2.5 * pow(atmosphereDist, 4.0)));                    // distance
    colour += ao * heightColour * HEIGHT_FOG_STRENGTH * DISC_COLOUR_L;

    // hack so AO doesn't show through the most intense scattering, ambient is a big hack all up
    colour = lerp(colour * ao, colour, pow(cosSunAngle, 4.0));    

    // done
    colour = ToneMap(colour);
//    colour = normal;
//    colour = raytraceDebug;

#if DEBUG_LODS == 1
    switch (input.debug)
    {
    case 0: // very low res, keep green
        colour.xz = 0.0;
        break;
    case 1: // low res, keep blue
        colour.xy = 0.0;
        break;
    case 2: // med res, keep yellow
        colour.z = 0.0;
        break;
    case 3: // high res, keep red
        colour.yz = 0.0;
        break;
    }
#endif
    return float4(colour, 1.0);
}

