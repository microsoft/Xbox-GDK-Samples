//-----------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

// Common structures and code for shaders

// Constants & Parameters
Texture2D<float4> Texture : register(t0);
sampler Sampler : register(s0);

cbuffer Parameters : register(b0)
{
    float4 DiffuseColor             : packoffset(c0);
    float3 EmissiveColor            : packoffset(c1);
    float3 SpecularColor            : packoffset(c2);
    float  SpecularPower            : packoffset(c2.w);

    float3 LightDirection[3]        : packoffset(c3);
    float3 LightDiffuseColor[3]     : packoffset(c6);
    float3 LightSpecularColor[3]    : packoffset(c9);

    float3 EyePosition              : packoffset(c12);

    float3 FogColor                 : packoffset(c13);
    float4 FogVector                : packoffset(c14);

    float4x4 World                  : packoffset(c15);
    float3x3 WorldInverseTranspose  : packoffset(c19);
    float4x4 WorldViewProj          : packoffset(c22);

    float  GlobalTime               : packoffset(c26.x);
};

// Root signatures must match definition in each effect, or shaders will be recompiled on Xbox when PSO loads
#define __XBOX_DX12_ROOT_SIGNATURE "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|DENY_DOMAIN_SHADER_ROOT_ACCESS|DENY_GEOMETRY_SHADER_ROOT_ACCESS|DENY_HULL_SHADER_ROOT_ACCESS), DescriptorTable(SRV(t0)), CBV(b0), StaticSampler(s0, filter = FILTER_ANISOTROPIC)"

// Vertex input
struct VSInputNmTx
{
	float4 Position : SV_POSITION;
	float3 Normal   : NORMAL;
	float2 TexCoord : TEXCOORD0;
};

// Interpolants
struct VSOutputPixelLightingTx
{
	float2 TexCoord   : TEXCOORD0;
	float4 PositionWS : TEXCOORD1;
	float3 NormalWS   : TEXCOORD2;
	float4 Diffuse    : COLOR0;
	float4 PositionPS : SV_Position;
};

struct PSInputPixelLightingTx
{
    float2 TexCoord   : TEXCOORD0;
    float4 PositionWS : TEXCOORD1;
    float3 NormalWS   : TEXCOORD2;
    float4 Diffuse    : COLOR0;
};


// Lighting
struct CommonVSOutputPixelLighting
{
    float4 Pos_ps;
    float3 Pos_ws;
    float3 Normal_ws;
    float  FogFactor;
};

float ComputeFogFactor(float4 position)
{
    return saturate(dot(position, FogVector));
}

void ApplyFog(inout float4 color, float fogFactor)
{
    color.rgb = lerp(color.rgb, FogColor * color.a, fogFactor);
}


void AddSpecular(inout float4 color, float3 specular)
{
    color.rgb += specular * color.a;
}

CommonVSOutputPixelLighting ComputeCommonVSOutputPixelLighting(float4 position, float3 normal)
{
    CommonVSOutputPixelLighting vout;
    
    vout.Pos_ps = mul(position, WorldViewProj);
    vout.Pos_ws = mul(position, World).xyz;
    vout.Normal_ws = normalize(mul(normal, WorldInverseTranspose));
    vout.FogFactor = ComputeFogFactor(position);
    
    return vout;
}

struct ColorPair
{
    float3 Diffuse;
    float3 Specular;
};

ColorPair ComputeLights(float3 eyeVector, float3 worldNormal, uniform int numLights)
{
    float3x3 lightDirections = 0;
    float3x3 lightDiffuse = 0;
    float3x3 lightSpecular = 0;
    float3x3 halfVectors = 0;
    
    [unroll]
    for (int i = 0; i < numLights; i++)
    {
        lightDirections[i] = LightDirection[i];
        lightDiffuse[i]    = LightDiffuseColor[i];
        lightSpecular[i]   = LightSpecularColor[i];
        
        halfVectors[i] = normalize(eyeVector - lightDirections[i]);
    }

    float3 dotL = mul(-lightDirections, worldNormal);
    float3 dotH = mul(halfVectors, worldNormal);
    
    float3 zeroL = step(0, dotL);

    float3 diffuse  = zeroL * dotL;
    float3 specular = pow(max(dotH, 0) * zeroL, SpecularPower);

    ColorPair result;
    
    result.Diffuse  = mul(diffuse,  lightDiffuse)  * DiffuseColor.rgb + EmissiveColor;
    result.Specular = mul(specular, lightSpecular) * SpecularColor;

    return result;
}

void ApplyLighting(inout float4 color, out ColorPair lightResult, float3 eyePosition, float3 positionWS, float3 normalWS)
{
    float3 eyeVector = normalize(EyePosition - positionWS);
    float3 worldNormal = normalize(normalWS);

    lightResult = ComputeLights(eyeVector, worldNormal, 3);

    color.rgb *= lightResult.Diffuse;
}

VSOutputPixelLightingTx CreateVSOutput(float4 positionPS, float3 positionWS, float3 normalWS, float fogFactor, float diffuseAlpha, float2 texCoord)
{
    VSOutputPixelLightingTx vout;
    
    vout.PositionPS = positionPS;
    vout.PositionWS = float4(positionWS, fogFactor);
    vout.NormalWS = normalWS;
    vout.Diffuse = float4(1, 1, 1, diffuseAlpha);
    vout.TexCoord = texCoord;

    return vout;
}
