//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Renders a lit mesh
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct Pixel
{
    float4 color            : SV_TARGET0;
};

// The following is for drawing generic lit, textured meshes
struct InterpolantsMesh
{
    float4 position     : SV_POSITION0;
    float3 normal       : NORMAL0;
    float2 texcoord     : TEXCOORD0;
    float3 tangent      : TANGENT0;
    float3 binormal     : BINORMAL0;
    float3 posworld        : TEXCOORD1;
};

SamplerState samMesh : register(s0);

Texture2D texDiffuse : register(t0);
Texture2D texNormal : register(t1);

static const uint g_iNumLights = 2;
cbuffer Light : register(b0)
{
    float4          ambientColor;
    float4          eye;
    bool            useNormalMap;
    struct LightData
    {
        float4        lightWorldDir;
        float4        lightColor;
        float       specularPower;
    } g_LightData[g_iNumLights];
};

//--------------------------------------------------------------------------------------
// Sample normal map, convert to signed, apply tangent-to-world space transform
//--------------------------------------------------------------------------------------
float3 CalcPerPixelNormal(in float2 texcoord, in float3 vertexNormal, in float3 vertexTangent)
{
    // Compute tangent frame
    vertexNormal = normalize(vertexNormal);
    vertexTangent = normalize(vertexTangent);
    float3 vertexBinormal = normalize(cross(vertexTangent, vertexNormal));
    float3x3 tangentSpaceToWorldSpace = float3x3(vertexTangent, vertexBinormal, vertexNormal);

    // Compute per-pixel normal
    float3 pixelNormal = texNormal.Sample(samMesh, texcoord).xyz;
    pixelNormal = 2.0f * pixelNormal - 1.0f;

    return mul(pixelNormal, tangentSpaceToWorldSpace);
}

ROOT_SIGNATURE_GRAPHICS
Pixel main(InterpolantsMesh In)
{
    Pixel Out;
    Out.color = ambientColor;

    float3 normal;
    if (useNormalMap)
    {
        normal = CalcPerPixelNormal(In.texcoord, In.normal, In.tangent);
    }
    else
    {
        normal = normalize(In.normal);
    }

    for (uint light = 0; light < g_iNumLights; ++light)
    {
        // diffuse
        float diffuseIntensity = saturate(-dot(g_LightData[light].lightWorldDir.xyz, normal));

        // specular
        float3 reflected = reflect(g_LightData[light].lightWorldDir.xyz, normal);
        float3 eyeToSurface = normalize(In.posworld - eye.xyz);
        const float epsilon = 1e-10;    // pow( 0.0f, 0.0f ) can explode
        float specularIntensity = pow(saturate(-dot(reflected, eyeToSurface)) + epsilon, g_LightData[light].specularPower);

        // total
        Out.color += (diffuseIntensity + specularIntensity) * g_LightData[light].lightColor;
    }

    Out.color *= texDiffuse.Sample(samMesh, In.texcoord);

    return Out;
}
