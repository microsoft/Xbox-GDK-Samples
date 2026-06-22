//--------------------------------------------------------------------------------------
// MeshPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Pixel shader for drawing mesh
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

struct PSInput
{
	float4 diffuse  : COLOR0;
	float4 specular : COLOR1;
	float2 texCoord : TEXCOORD0;
};

void AddSpecular(inout float4 color, float3 specular)
{
	color.rgb += 0.f; //specular * color.a;
}

float4 MainMeshPS(PSInput psIn)
{
	float4 color = diffuseTex.Sample(simpleSampler, psIn.texCoord) * psIn.diffuse + float4(AmbientColor, 0.f);

	AddSpecular(color, psIn.specular.rgb);

	return color;
}
