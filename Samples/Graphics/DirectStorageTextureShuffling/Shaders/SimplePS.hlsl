#include "Shared.hlsli"

Texture2D g_texture : register(t0);
sampler g_sampler : register(s0);

[RootSignature(RootSig)]
float4 main(Interpolants In) : SV_TARGET0
{
    return g_texture.Sample(g_sampler, In.uv);
}
