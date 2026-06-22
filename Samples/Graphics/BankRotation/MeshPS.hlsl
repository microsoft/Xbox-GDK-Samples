//--------------------------------------------------------------------------------------
// MeshPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Pixel shader for drawing mesh. For DrawIndexedX, it uses a texture array. The index 
// is chosen based on data passed in the DrawData struct.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

struct PSInput
{
	float4 position : SV_Position;
    float3 normal   : NORMAL0;
	float2 texCoord : TEXCOORD0;
};

struct PSOutput
{
    float4 diffuse : SV_TARGET0;
    float4 output2 : SV_TARGET1;
    float4 normal  : SV_TARGET2;
    float4 output4 : SV_TARGET3;

    //float4 diffuse2 : SV_TARGET4;
    //float4 normal2  : SV_TARGET5;
    //float4 tangent2  : SV_TARGET6;
    //float4 binormal2  : SV_TARGET7;
};

Texture2D<float4> diffuseTex : register(t0);
sampler simpleSampler : register(s0);

[ROOT_SIGNATURE_MESH]
PSOutput main(PSInput psIn)
{
    PSOutput psOut;
	uint materialID = 0;

    psOut.diffuse = diffuseTex.Sample(simpleSampler, psIn.texCoord);
    psOut.normal  = float4(psIn.normal, 0.0f);
    psOut.output2 = diffuseTex.Sample(simpleSampler, psIn.texCoord) * float4(0.3f, 0.4f, 0.3f, 1.f);
    psOut.output4 = diffuseTex.Sample(simpleSampler, psIn.texCoord) * float4(0.f, 0.5f, 0.5f, 1.f);


    //psOut.diffuse2 = diffuseTex.Sample(simpleSampler, psIn.texCoord);
    //psOut.normal2 = float4(1.0, 0.0f, 0.0f, 0.0f);
    //psOut.tangent2 = float4(0.0f, 1.0f, 0.0f, 0.0f);
    //psOut.binormal2 = float4(0.0f, 0.0f, 1.0f, 0.0f);

	return psOut;
}
