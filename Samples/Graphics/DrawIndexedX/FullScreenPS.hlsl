
#include "Common.hlsli"

struct PSInput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

Texture2D<float4> diffuseTex[] : register(t0, space1);
sampler simpleSampler : register(s0);

float LinearizeDepth(float depthValue)
{
	float near = 2.0f;
	float far = 1500.f;
	float linearZFactorA = (near - far) / near;
	float linearZFactorB = far / near;
	float dist = 1.0 / (linearZFactorA * depthValue + linearZFactorB);
	return dist;
}

[ROOT_SIGNATURE_MESH]
float4 main(PSInput psIn) : SV_TARGET0
{
	return float4(LinearizeDepth(diffuseTex[0].Sample(simpleSampler, psIn.texCoord).x), 0.f, 0.f, 1.0f);
}