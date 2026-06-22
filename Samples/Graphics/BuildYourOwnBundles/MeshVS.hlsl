//--------------------------------------------------------------------------------------
// MeshVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Vertex shader for drawing mesh
//--------------------------------------------------------------------------------------

#include "Common.hlsli"


struct VSInput
{
	float4 position : SV_Position;
	float3 normal   : NORMAL;
	float2 texCoord : TEXCOORD;
};


struct VSOutput
{
	float4 diffuse    : COLOR0;
	float4 specular   : COLOR1;
	float2 texCoord   : TEXCOORD;
	float4 positionPS : SV_Position;
};


struct ColorPair
{
	float3 diffuse;
	float3 specular;
};

ColorPair ComputeLights(float3 eyeVector, float3 worldNormal)
{
	float3x3 lightDirections = 0;
	float3x3 lightDiffuse = 0;
	float3x3 lightSpecular = 0;
	float3x3 halfVectors = 0;

	[unroll]
	for (int i = 0; i < NUM_LIGHTS; i++)
	{
		lightDirections[i] = LightDirection[i];
		lightDiffuse[i] = LightDiffuseColor[i];
		lightSpecular[i] = LightSpecularColor[i];

		halfVectors[i] = normalize(eyeVector - lightDirections[i]);
	}

	float3 dotL = mul(-lightDirections, worldNormal);
	float3 dotH = mul(halfVectors, worldNormal);

	float3 zeroL = step(0, dotL);

	float3 diffuse = zeroL * dotL;
	float3 specular = pow(max(dotH, 0) * zeroL, SpecularPower);

	ColorPair result;

	result.diffuse = mul(diffuse, lightDiffuse)  * DiffuseColor.rgb + EmissiveColor;
	result.specular = mul(specular, lightSpecular) * SpecularColor;

	return result;
}

[ROOT_SIGNATURE_MESH]
VSOutput main(VSInput vsIn)
{
	VSOutput vsOut;

	float4 pos_ws = mul(vsIn.position, World);
	float3 eyeVector = normalize(EyePosition - pos_ws.xyz);
	float3 worldNormal = normalize(mul(vsIn.normal, WorldInverseTranspose));
	ColorPair lightResult = ComputeLights(eyeVector, worldNormal);

	vsOut.positionPS = mul(vsIn.position, WorldViewProj);
	vsOut.diffuse = float4(lightResult.diffuse, DiffuseColor.a);
	vsOut.specular = float4(lightResult.specular, 1.0f);
	vsOut.texCoord = vsIn.texCoord;

	return vsOut;
}