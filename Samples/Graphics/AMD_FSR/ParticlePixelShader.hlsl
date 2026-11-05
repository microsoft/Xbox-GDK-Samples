#include "ParticleCommon.hlsli"

[RootSignature(ParticleRootSignature)]
float4 main(ParticleOutput input) : SV_Target0
{
    const float2 offset = input.TexCoord * 2.0f - 1.0f;
    const float radial = saturate(1.0f - dot(offset, offset));
    const float alpha = radial * radial * input.Color.a;
    clip(alpha - 0.01f);
    return float4(input.Color.rgb, alpha);
}
