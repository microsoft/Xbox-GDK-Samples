//--------------------------------------------------------------------------------------
// InstancedLightVolumesPS.hlsl
//
// Pixel Shader for sphere shaped lights (instanced).
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "InstancedLightVolumesCommon.hlsli"
#include "shared.hlsli"

// G-Buffer SRVs
Texture2D<float4> g_texAlbedo : register(t0);
Texture2D<float3> g_texNormal : register(t1);
Texture2D<float3> g_texPos : register(t2);

ConstantBuffer<SceneConstants> sceneConsts : register(b0);

// PSComposeQuadParticle
[RootSignature(ROOT_SIGNATURE_LIGHT_VOLUME)]
float4 main(in InstancedLightVolumeInterpolants input) : SV_Target
{
    float4 albedo = g_texAlbedo.Load(uint3(input.Position.xy, 0));
    float3 normal = g_texNormal.Load(uint3(input.Position.xy, 0));
    float3 bgWorldPos = g_texPos.Load(uint3(input.Position.xy, 0));

    float radius = input.Range;
    
    // Light attenuation calculations
    float3 light = input.LightWorldPos - bgWorldPos;
    float distance = length(light);
    
    clip(radius - distance);
    float3 lightDir = light / distance;
    float Intensity = max(0.0f, (radius - distance) / radius);
    
    // Calculate diffuse component of directional lighting.
    float3 lighting = max(dot(normal, lightDir), 0.0f) * input.Color.xyz * Intensity;
    
    // Calculate specular component of directional lighting.
    float3 viewDir = normalize(sceneConsts.cameraPos - bgWorldPos);
    float3 halfAngle = normalize(viewDir + lightDir.xyz);
    float3 specPower = pow(max(dot(halfAngle, normal), 0.0f), 32) * input.Color.xyz * Intensity;
    
    // Return combined lighting contributions.
    return float4(lighting * albedo.xyz + specPower, 1.0f);
}
