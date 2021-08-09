//--------------------------------------------------------------------------------------
// PixelShaders.hlsli
//
// Pixel Shaders for the Dolphin sample
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
struct VSOUT
{
    float4 vPosition    : SV_POSITION;
    float4 vLightAndFog : COLOR0_center;		// COLOR0.x = light, COLOR0.y = fog
    float4 vTexCoords   : TEXCOORD1;			// TEXCOORD0.xy = basetex, TEXCOORD0.zw = caustictex
};


//--------------------------------------------------------------------------------------
// Pixel shader constants
//--------------------------------------------------------------------------------------
sampler SamplerSkin : register(s0);
sampler SamplerCaustics : register(s1);

cbuffer cb0 : register(b1)
{
    uniform float3 g_vAmbient       : register(c0); // Material ambient color
    uniform float4 g_vFogColor      : register(c1); // Fog color
}

Texture2D TextureSkin : register(t0);
Texture2D TextureCaustics : register(t1);


//--------------------------------------------------------------------------------------
// Root Signature
//--------------------------------------------------------------------------------------
#define rootSig "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
	"| DENY_DOMAIN_SHADER_ROOT_ACCESS " \
	"| DENY_GEOMETRY_SHADER_ROOT_ACCESS " \
	"| DENY_HULL_SHADER_ROOT_ACCESS), " \
    "CBV(b0, space = 0, visibility=SHADER_VISIBILITY_VERTEX)," \
    "CBV(b1, space = 0, visibility=SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t0), visibility=SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t1), visibility=SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL," \
        "addressU=TEXTURE_ADDRESS_MIRROR," \
        "addressV=TEXTURE_ADDRESS_MIRROR," \
        "addressW=TEXTURE_ADDRESS_MIRROR," \
        "filter=FILTER_MIN_MAG_LINEAR_MIP_POINT," \
        "comparisonFunc = COMPARISON_NEVER)," \
    "StaticSampler(s1, visibility=SHADER_VISIBILITY_PIXEL," \
        "addressU=TEXTURE_ADDRESS_WRAP," \
        "addressV=TEXTURE_ADDRESS_WRAP," \
        "addressW=TEXTURE_ADDRESS_WRAP," \
        "filter=FILTER_MIN_MAG_MIP_LINEAR," \
        "comparisonFunc = COMPARISON_NEVER)"


//--------------------------------------------------------------------------------------
// Name: ShadeCausticsPixel()
// Desc: Pixel shader to add underwater caustics to a lit base texture.
//--------------------------------------------------------------------------------------
[RootSignature(rootSig)]
float4 ShadeCausticsPixel(VSOUT Input) : SV_Target
{
    // Decompress input values
    float3 vLightColor = Input.vLightAndFog.xxx;
    float  fFogValue = Input.vLightAndFog.y;
    float2 vBaseTexCoords = Input.vTexCoords.xy;
    float2 vCausticTexCoords = Input.vTexCoords.zw;

    // Combine lighting, base texture and water caustics texture
    float4 vDiffuse = TextureSkin.Sample(SamplerSkin, vBaseTexCoords);
    float4 vCaustics = TextureCaustics.Sample(SamplerCaustics, vCausticTexCoords);

    // Combine lighting, base texture and water caustics texture
    float4 PixelColor0 = vDiffuse  * float4(vLightColor + g_vAmbient, 1);
    float4 PixelColor1 = vCaustics * float4(vLightColor, 1);

    // Return color blended with fog
    return lerp(g_vFogColor, PixelColor0 + PixelColor1, fFogValue);
}


