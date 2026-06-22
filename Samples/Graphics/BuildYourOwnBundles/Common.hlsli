#include "Common.h"

#define ROOT_SIGNATURE_MESH \
    RootSignature\
    (\
       "RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | \
                    DENY_DOMAIN_SHADER_ROOT_ACCESS | \
                    DENY_GEOMETRY_SHADER_ROOT_ACCESS | \
                    DENY_HULL_SHADER_ROOT_ACCESS ), \
        DescriptorTable ( SRV(t0), \
                          SRV(t1), \
                          UAV(u0, NumDescriptors=2)), \
        CBV(b0), \
        CBV(b1), \
        CBV(b2), \
        StaticSampler(s0)" \
    )



Texture2D<float4> diffuseTex : register(t0);
sampler simpleSampler : register(s0);


cbuffer Parameters : register(b0)
{
	float4 DiffuseColor                      : packoffset(c0);
	float3 EmissiveColor                     : packoffset(c1);
	float3 AmbientColor                      : packoffset(c2);
	float3 SpecularColor                     : packoffset(c3);
	float  SpecularPower                     : packoffset(c3.w);

	float3 LightDirection[MAX_LIGHTS]        : packoffset(c4);
	float3 LightDiffuseColor[MAX_LIGHTS]     : packoffset(c7);
	float3 LightSpecularColor[MAX_LIGHTS]    : packoffset(c10);

	float3 EyePosition                       : packoffset(c13);
									         
	float4x4 World                           : packoffset(c14);
	float3x3 WorldInverseTranspose           : packoffset(c18);
	float4x4 WorldViewProj                   : packoffset(c21);
};

