#include "SimpleBezier.hlsli"

//--------------------------------------------------------------------------------------
// The pixel shader works the same as it would in a normal graphics pipeline.
// In this sample, it performs very simple N dot L lighting.
//--------------------------------------------------------------------------------------
[RootSignature(rootSig)]
float4 main(DS_OUTPUT Input) : SV_TARGET
{
    float3 N = normalize(Input.normal);
    float3 L = normalize(Input.worldPos - g_cameraWorldPos);
    return abs(dot(N, L)) * float4(1, 0, 0, 1);
}
