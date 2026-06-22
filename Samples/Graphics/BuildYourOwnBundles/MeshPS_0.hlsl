#include "MeshPS.hlsli"

[ROOT_SIGNATURE_MESH]
float4 main(PSInput psIn) : SV_TARGET0
{
	float4 color = MainMeshPS(psIn);
	return color * float4(150.f / 256.f, 125.0f / 256.f, 50.f / 256.f, 1.0f);
}