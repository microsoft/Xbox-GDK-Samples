#include "SimpleLighting.hlsli"

[RootSignature(rootSig)]
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, mWorld);
    output.Pos = mul(output.Pos, mView);
    output.Pos = mul(output.Pos, mProjection);
    output.Normal = mul(input.Normal, ((float3x3) mWorld));

    return output;
}