//--------------------------------------------------------------------------------------
// FullscreenColorPassPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define HLSL_INCLUDE
#include "../SharedDataTypes.h"

#include "RootSignatures.hlsli"
#include "ReprojectHelper.hlsli"

struct Interpolators
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORDS;
};

static const float3 lightPosition = float3(10.0f, 10.0f, 0.0f);
static const float3 lightDiffuseColor = float3(0.75f, 0.75f, 0.75f);
static const float3 ambientLightColor = float3(0.3f, 0.3f, 0.3f);

ConstantBuffer<FullscreenPassCB> constants : register(b0);

// GBuffer shader resources
Texture2D<float4> texAlbedo     : register(t0);
Texture2D<float4> texNormals    : register(t1);
Texture2D<float> texDepth       : register(t2);
RWTexture2D<float2> texVelocity : register(u0);
sampler diffuseSampler          : register(s0);

// Forward declarations.
float2 Reproject(in uint2 DTid, in float2 motionVector);

// Diffuse lighting calculation.
inline float CalculateDiffuseCoefficient(in float3 hitPosition, in float3 incidentLightRay, in float3 normal)
{
    float fNDotL = saturate(dot(-incidentLightRay, normal));
    return fNDotL;
}

// Phong lighting specular component
inline float3 CalculateSpecularCoefficient(in float3 hitPosition, in float3 worldRayDirection, in float3 incidentLightRay,
    in float3 normal, in float specularPower)
{
    float3 reflectedLightRay = normalize(reflect(incidentLightRay, normal));
    return pow(saturate(dot(reflectedLightRay, normalize(-worldRayDirection))), specularPower);
}

// Calculate phong shading
inline float3 CalculatePhongLighting(in float3 albedo, in float3 worldPosition, in float3 cameraRayDir, in float3 normal,
    in float diffuseCoef = 1.0f, in float specularCoef = 1.0f, in float specularPower = 50.0f)
{
    float3 incidentLightDir = normalize(worldPosition - lightPosition);

    // Diffuse component.
    float Kd = CalculateDiffuseCoefficient(worldPosition, incidentLightDir, normal);
    float3 diffuseColor = diffuseCoef * Kd * lightDiffuseColor * albedo;

    // Specular component.
    float3 lightSpecularColor = 1.0f;
    float3 Ks = CalculateSpecularCoefficient(worldPosition, cameraRayDir, incidentLightDir, normal, specularPower);
    float3 specularColor = specularCoef * Ks * lightSpecularColor;

    // Ambient component.
    // Fake AO: Darken faces with normal facing downwards/away from the sky a little bit.
    float3 ambientColorMin = ambientLightColor - 0.1f;
    float3 ambientColorMax = ambientLightColor;
    float a = 1.0f - saturate(dot(normal, float3(0.0f, -1.0f, 0.0f)));
    float3 ambientColor = albedo * lerp(ambientColorMin, ambientColorMax, a);

    return ambientColor + diffuseColor + specularColor;
}

[RootSignature(FullscreenPassRS)]
float4 main(Interpolators IN) : SV_TARGET
{
    uint3 pixelFloor = uint3(IN.Position.xy, 0);

    float3 albedo = texAlbedo.Load(pixelFloor).xyz;
    float3 normal = texNormals.Load(pixelFloor).xyz;

    float2 backgroundVelocity = Reproject(pixelFloor.xy, 0.0f);
    float2 objectVelocity = texVelocity[pixelFloor.xy];
    texVelocity[pixelFloor.xy] = objectVelocity + backgroundVelocity;

    // Load depth. It will be between 0 (near) and 1 (far)
    float ndcZ = texDepth.Load(int3(pixelFloor.xy, 0));
    float4 worldPosition = getWorldSpaceFromPixelCoordAndDepth(IN.Position.xy, constants.invView, constants.invProj, constants.resolution.zw, ndcZ);

    float3 cameraRayDir = (worldPosition.xyz - constants.cameraPosition);

    float3 phongColor = CalculatePhongLighting(albedo, worldPosition.xyz, cameraRayDir, normal);
    return float4(phongColor, 1.0f);
}

float2 Reproject(in uint2 DTid, in float2 motionVector)
{
    // Correct current position by motion vector
    float2 pixelCenter = DTid + 0.5f;
    float2 pixelCorrectedPosition = pixelCenter + motionVector;

    // Load depth. It will be between 0 (near) and 1 (far)
    float ndcZ = texDepth.Load(int3(pixelCorrectedPosition, 0));

    // Get world space for the current pixel 
    float4 worldSpaceCoord = getWorldSpaceFromPixelCoordAndDepth(pixelCorrectedPosition, constants.invView, constants.invProj, constants.resolution.zw, ndcZ);

    // Reproject using previous frame viewProj
    float4 prevFrameViewSpace = mul(worldSpaceCoord, constants.prevFrameView);
    float4 prevFrameClipSpace = mul(prevFrameViewSpace, constants.prevFrameProj);

    // Discard threads that would be out of bounds
    if (abs(prevFrameClipSpace.x) > abs(prevFrameClipSpace.w) ||
        abs(prevFrameClipSpace.y) > abs(prevFrameClipSpace.w))
    {
        return 0.0f;
    }

    float2 prevFrameNDC = prevFrameClipSpace.xy / prevFrameClipSpace.w;
    prevFrameNDC.y *= -1.0f;

    float2 halfRes = 0.5f * constants.resolution.xy;

    float2 prevFrameScreen = (prevFrameNDC * halfRes) + halfRes;

    float2 backgroundVelocity = prevFrameScreen - pixelCenter;
    return backgroundVelocity;
}
