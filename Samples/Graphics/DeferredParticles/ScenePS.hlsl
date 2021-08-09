//--------------------------------------------------------------------------------------
// ScenePS.hlsl
//
// Scene pixel shader.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

[RootSignature(SceneRS)]
float4 main(in SceneInterpolants In) : SV_Target
{
    // Sample diffuse texture, grab specular intensity from alpha channel.
    float4 diffuse = texDiffuse.Sample(sampLinear, In.UV);
    float specularIntensity = diffuse.a;

    // Sample and convert the normal from our normal map.
    float3 normal = DeriveNormalFromUnsignedXY(texNormal.Sample(sampLinear, In.UV).xy);

    // Transform the normal from tangent space to world space.
    float3x3 BTNMatrix = float3x3(In.WorldBinormal, In.WorldTangent, In.WorldNormal);
    normal = normalize(mul(normal, BTNMatrix));

    // Calculate diffuse component of directional lighting.
    float4 lighting = saturate(dot(normal, -g_scene.LightDir.xyz)) * g_scene.Color;

    // Calculate specular component of directional lighting.
    float3 viewDir = normalize(g_scene.CameraPos - In.WorldPosition.xyz);
    float3 halfAngle = normalize(viewDir - g_scene.LightDir.xyz);
    float4 specPower = pow(saturate(dot(halfAngle, normal)), 32) * g_scene.Color;

    // Accumulate contributions from point lights in scene.
    float4 lightColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 GlowLightSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < g_glowLights.NumGlowLights; ++i)
    {
        // Mostly standard point light calculation, with distance falloff...
        float3 delta = g_glowLights.GlowLightPosIntensity[i].xyz - In.WorldPosition.xyz;
        float distSq = dot(delta, delta);
        float dist = sqrt(distSq);
        float3 toLight = delta / dist;
        float3 d = float3(1, dist, distSq);

        float fatten = 1.0 / dot(g_glowLights.MeshLightAttenuation.xyz, d);

        float intensity = fatten * g_glowLights.GlowLightPosIntensity[i].w;
        lightColor += intensity * g_glowLights.GlowLightColor[i] * saturate(dot(toLight, normal));

        // Calculate specular component from glow-light.
        float3 GlowHalfAngle = normalize(viewDir + toLight);
        float4 GlowSpecPower = pow(saturate(dot(GlowHalfAngle, normal)), 32) * g_glowLights.GlowLightColor[i];
        GlowLightSpecular += GlowSpecPower;
    }

    // Return combined lighting contributions.
    return lighting * diffuse + specPower * specularIntensity * diffuse + lightColor + GlowLightSpecular * specularIntensity * diffuse;
}