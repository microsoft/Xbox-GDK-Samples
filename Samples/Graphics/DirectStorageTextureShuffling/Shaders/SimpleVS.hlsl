#include "Shared.hlsli"

[RootSignature(RootSig)]
Interpolants main(float4 position : POSITION, float2 uv : TEXCOORD)
{
    Interpolants output;

    output.pos = position;
    output.pos.xy = (position.xy * scale) + offset;
    output.uv = uv;

    return output;
}
