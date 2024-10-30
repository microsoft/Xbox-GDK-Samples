#include "rs_and_vsps_params.hlsl"

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

[RootSignature(ROOT_SIGNATURE)]
float4 main(VSPS_PARAMS input) : SV_Target
{
    float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
    return out_col;
}
