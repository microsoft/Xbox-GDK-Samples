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
	float3 position : SV_Position;
	float3 normal   : NORMAL0;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
	float4 positionPS : SV_Position;
    float3 normal     : NORMAL0;
	float2 texCoord   : TEXCOORD0;
};

cbuffer Parameters : register(b0)
{
	float4x4 WorldViewProj;
};

[ROOT_SIGNATURE_MESH]
VSOutput main(VSInput vsIn, uint vertexID : SV_VERTEXID)
{
	VSOutput vsOut;
	vsOut.positionPS = mul(float4(vsIn.position.xyz, 1.0f), WorldViewProj);
    vsOut.normal = vsIn.normal;
	vsOut.texCoord = vsIn.texCoord;

	return vsOut;
}