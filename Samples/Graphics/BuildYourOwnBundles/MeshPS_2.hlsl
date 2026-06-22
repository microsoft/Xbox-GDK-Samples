#include "MeshPS.hlsli"

[ROOT_SIGNATURE_MESH]
float4 main(PSInput psIn) : SV_TARGET0
{
	float4 color = MainMeshPS(psIn);
	return color * float4(100.f / 256.f, 75.0f / 256.f, 226.f / 256.f, 1.0f);
}
