//--------------------------------------------------------------------------------------
// SceneLighting.hlsli
//
// HLSL functions for performing simple Blinn (N dot H) scene lighting with one, two, 
// or three layers of diffuse, normal and specular maps.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

float4 SceneLighting1Layer(PS_IN In)
{
    float Ambient = 0.1f;
    float3 LightDirection = normalize(float3(1, 3, 2));

    float4 Diffuse = SampleTexture(g_BilinearSampler, g_ObjectDiffuseLayer1, In.TexCoord0);
    float3 NormalSample = SampleTexture(g_BilinearSampler, g_ObjectNormalLayer1, In.TexCoord0).xyz;
    float3 NormalVector = normalize(NormalSample * 2 - 1);
    float SpecularSample = SampleTexture(g_BilinearSampler, g_ObjectSpecularLayer1, In.TexCoord0).x;

    float3x3 TangentSpaceMatrix = float3x3(In.Binormal, In.Tangent, In.Normal);
    float3 WorldNormal = normalize(mul(NormalVector, TangentSpaceMatrix));

    float3 ViewDir = normalize(In.ViewDirection);
    float3 HalfDir = normalize(LightDirection - ViewDir);
    float NDotH = max(0, dot(HalfDir, WorldNormal));
    float SpecularAmount = SpecularSample * pow(NDotH, 16);
    float3 SpecularColor = SpecularAmount.xxx;

    float Brightness = saturate(dot(WorldNormal, LightDirection)) + Ambient;
    return float4(Diffuse.rgb * Brightness + SpecularColor, 1);
}

float4 SceneLighting2Layers(PS_IN In)
{
    float Ambient = 0.1f;
    float3 LightDirection = normalize(float3(1, 3, 2));

    float4 DiffuseA = SampleTexture(g_BilinearSampler, g_ObjectDiffuseLayer1, In.TexCoord0);
    float3 NormalSampleA = SampleTexture(g_BilinearSampler, g_ObjectNormalLayer1, In.TexCoord0).xyz;
    float SpecularSampleA = SampleTexture(g_BilinearSampler, g_ObjectSpecularLayer1, In.TexCoord0).x;

    float4 DiffuseB = SampleTexture(g_BilinearSampler, g_ObjectDiffuseLayer2, In.TexCoord0);
    float3 NormalSampleB = SampleTexture(g_BilinearSampler, g_ObjectNormalLayer2, In.TexCoord0).xyz;
    float SpecularSampleB = SampleTexture(g_BilinearSampler, g_ObjectSpecularLayer2, In.TexCoord0).x;

    float4 Diffuse = (DiffuseA + DiffuseB) * 0.5;
    float3 NormalSample = (NormalSampleA + NormalSampleB) * 0.5;
    float SpecularSample = (SpecularSampleA + SpecularSampleB) * 0.5;

    float3 NormalVector = normalize(NormalSample * 2 - 1);

    float3x3 TangentSpaceMatrix = float3x3(In.Binormal, In.Tangent, In.Normal);
    float3 WorldNormal = normalize(mul(NormalVector, TangentSpaceMatrix));

    float3 ViewDir = normalize(In.ViewDirection);
    float3 HalfDir = normalize(LightDirection - ViewDir);
    float NDotH = max(0, dot(HalfDir, WorldNormal));
    float SpecularAmount = SpecularSample * pow(NDotH, 16);
    float3 SpecularColor = SpecularAmount.xxx;

    float Brightness = saturate(dot(WorldNormal, LightDirection)) + Ambient;
    return float4(Diffuse.rgb * Brightness + SpecularColor, 1);
}

float4 SceneLighting3Layers(PS_IN In)
{
    float Ambient = 0.1f;
    float3 LightDirection = normalize(float3(1, 3, 2));

    float4 DiffuseA = SampleTexture(g_BilinearSampler, g_ObjectDiffuseLayer1, In.TexCoord0);
    float3 NormalSampleA = SampleTexture(g_BilinearSampler, g_ObjectNormalLayer1, In.TexCoord0).xyz;
    float SpecularSampleA = SampleTexture(g_BilinearSampler, g_ObjectSpecularLayer1, In.TexCoord0).x;

    float4 DiffuseB = SampleTexture(g_BilinearSampler, g_ObjectDiffuseLayer2, In.TexCoord0);
    float3 NormalSampleB = SampleTexture(g_BilinearSampler, g_ObjectNormalLayer2, In.TexCoord0).xyz;
    float SpecularSampleB = SampleTexture(g_BilinearSampler, g_ObjectSpecularLayer2, In.TexCoord0).x;

    float4 DiffuseC = SampleTexture(g_BilinearSampler, g_ObjectDiffuseLayer3, In.TexCoord0);
    float3 NormalSampleC = SampleTexture(g_BilinearSampler, g_ObjectNormalLayer3, In.TexCoord0).xyz;
    float SpecularSampleC = SampleTexture(g_BilinearSampler, g_ObjectSpecularLayer3, In.TexCoord0).x;

    float4 Diffuse = (DiffuseA + DiffuseB + DiffuseC) * 0.3333;
    float3 NormalSample = (NormalSampleA + NormalSampleB + NormalSampleC) * 0.3333;
    float SpecularSample = (SpecularSampleA + SpecularSampleB + SpecularSampleC) * 0.3333;

    float3 NormalVector = normalize(NormalSample * 2 - 1);

    float3x3 TangentSpaceMatrix = float3x3(In.Binormal, In.Tangent, In.Normal);
    float3 WorldNormal = normalize(mul(NormalVector, TangentSpaceMatrix));

    float3 ViewDir = normalize(In.ViewDirection);
    float3 HalfDir = normalize(LightDirection - ViewDir);
    float NDotH = max(0, dot(HalfDir, WorldNormal));
    float SpecularAmount = SpecularSample * pow(NDotH, 16);
    float3 SpecularColor = SpecularAmount.xxx;

    float Brightness = saturate(dot(WorldNormal, LightDirection)) + Ambient;
    return float4(Diffuse.rgb * Brightness + SpecularColor, 1);
}
