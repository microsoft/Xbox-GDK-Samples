//------------------------------------------------------------------------------------
// SampleCubeMapPixelShader.hlsl
//
// Shader to sample an environment map
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SampleCubeMapHeader.hlsli"

struct Pixel
{
    float4 color    : SV_Target;
};

static const float4 s_materialDiff = { 1.0, 1.0, 1.0, 1.0 };
static const float4 s_materialSpec = { 0.8, 0.8, 0.8, 1.0 };


// Cubic environment mapping
// Greene, "Environment Mapping and Other Applications of World Projections", IEEE Computer Graphics and Applications. 1986.
[RootSignature(MainRS)]
Pixel main(VS_OUTPUT_SCENEENV vin)
{
    Pixel Out;
    Out.color = s_materialDiff;

    float3 eyeVector = normalize(constants.vEye - vin.wPos.xyz);
    float3 worldNormal = normalize(vin.wN);

    //ColorPair lightResult = ComputeLights(eyeVector, worldNormal, 3);

    //Out.color.rgb *= lightResult.Diffuse;

    float3 envcoord = reflect(-eyeVector, worldNormal);

    float4 envmap = txEnvMap.Sample(samCube, envcoord) * Out.color.a;


    Out.color.rgb = lerp(Out.color.rgb, envmap.rgb, s_materialSpec.rgb);

    return Out;
}
