//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define __XBOX_ENABLE_WAVE32

#define NEGATIVE_Y_NORMAL_POSSIBLE  0   // not possible on terrains

#include "TerrainRenderVS.hlsl"
#include "DeferredCommon.hlsli"
#include "SparseLightingCommon.hlsli"



float3 CalcAlbedoAndRoughness(out float roughness, float3 posWS, float3 normal)
{
    float3 grey     = float3(0.12, 0.12, 0.12);
    float3 brown    = float3(0.49, 0.2 , 0.1 );
    float3 green1   = float3(0.35, 0.45, 0.1 );
    float3 green2   = float3(0.2 , 0.3 , 0.07);
    float3 green3   = float3(0.3 , 0.35, 0.05);
    float3 snow     = float3(1.25, 1.25, 1.25);

    float noiseSmall = Fbm(posWS.xz * 2.0 + 2.0f * normal.xz, 6);
    float noiseLarge = Fbm(posWS.xz * 0.5 + normal.y, 10);

    float slopeSmooth = 1.0 - normal.y;
    float slopeRough = 1.0 - (normal.y + 0.1 * (noiseSmall * 2.0 - 1.0));

    float3 albedo, meadow, cliff;
    float t;

    roughness = 0.0;

    // construct meadow green
    t = saturate(abs(noiseLarge * 3.0 - 1.5));
    meadow = lerp(green1, green2, t);
    t = saturate(abs(noiseSmall * 2.0 - 1.0));
    meadow = lerp(meadow, green3, t);
    meadow *= min(noiseLarge, noiseSmall) * 0.5 + 0.25;

    // add in brown increasing on slopes and on height
    brown *= noiseSmall * 0.5 + 0.5;
    t = saturate((posWS.y - 7.0) / 10.0 + slopeRough * slopeRough + 0.5 * noiseSmall);
    t = lerp(t, t * 0.5, normal.y);
    albedo = lerp(meadow, brown, t);

    // small amount of spec for brown
    roughness = pow(t, 4.0) * 0.025;

    // texture the green and brown
    albedo *= (abs(noiseSmall * 3.0 - 1.5) * abs(noiseLarge * 2.0 - 1.0)) * 0.35 + 0.5;

    // add in snow at a higher range, but less on slopes
    t  = (posWS.y - (22.0 + noiseSmall * 2.0)) / 10.0;
    t += pow((1.0 - slopeSmooth), 3.0);
    t  = pow(saturate(t), 0.25);

    albedo = lerp(albedo, snow, t);

    // snow has a lot of specular
    roughness += pow(t, 2.0);

    // make any steep slopes cliff coloured, which is a mix of grey and brown, more grey at higher altitudes
    t = noiseSmall + (posWS.y - (15.0 + 10 * noiseLarge)) / 25.0;
    t += (saturate(noiseSmall) - 0.5) * 0.1;
    t = saturate(t);
    cliff = lerp(brown * 1.3, grey, t);
    float greyT = t;

    t = saturate(pow(slopeRough, 3.0) * 10.0);
    albedo = lerp(albedo, cliff, t);

    // remove specularity where we removed snow due to cliff face
    roughness *= 1.0 - t * t;

    // add some extra specular to cliffs, particularly if grey
    roughness += 0.75 * lerp(0.0, pow(greyT, 4.0), t);

    // extreme heights become whiter (not snow as such)
    t  = (posWS.y - (26.0 + 4 * noiseLarge)) / 25.0;
    t *= 1.0 - slopeRough;
    t  = smoothstep(0.0, 1.0, saturate(t));

    albedo = lerp(albedo, snow, t);

    // and more snow at very high ranges
    t = saturate((posWS.y - (26.0 + noiseLarge * 3.0)) / 32.0);

    albedo = lerp(albedo, snow, t);

    // snow has a lot of specular
    roughness += pow(t, 2.0);
    
    // everything has a little spec
    roughness += 0.02;

    return albedo;
    //return float3(slope.xxx);
    //return float3(roughness.xxx);
}


struct MRT
{
    float3 albedo : SV_Target0;
    float4 roughnessAndNormal : SV_Target1;
    float4 debugAndShadingRate : SV_Target2;
};


[RootSignature(GlobalRS)]
MRT TerrainRenderPS(VS_OUTPUT input, uint svCoverage : SV_Coverage, uint svShadingRate : SV_ShadingRate)
{
    float roughness;
    float3 normal, albedo;
    float2 uv = PosToUV(input.worldPos);
    float highResHeight = SourceHeightMapSRV0.Sample(LinearClampSampler, uv);
#if BUILD_LAKES == 1
    if (highResHeight <= TERRAIN_LAKE_HEIGHT)
    {
        normal = float3(0.0, 1.0, 0.0);
        albedo = float3(0.0, 0.0, 1.0);
    }
    else
#endif
    {
        normal = LoadAndDecodeNormal(uv);
        albedo = CalcAlbedoAndRoughness(roughness, input.worldPos, normal);
    }
    MRT output;

    output.albedo = albedo;
    output.roughnessAndNormal = EncodeRoughnessNormal(roughness, normal);
    output.debugAndShadingRate = float4(1.0, 1.0, 1.0, float(GetQuadCoverage(svCoverage, svShadingRate)) / 15.0);

#if DEBUG_LODS == 1
    switch (input.debug)
    {
    case 0: // very low res, keep green
        output.debugAndShadingRate.xz = 0.0;
        break;
    case 1: // low res, keep blue
        output.debugAndShadingRate.xy = 0.0;
        break;
    case 2: // med res, keep yellow
        output.debugAndShadingRate.z = 0.0;
        break;
    case 3: // high res, keep red
        output.debugAndShadingRate.yz = 0.0;
        break;
    }
#endif
    return output;
}

