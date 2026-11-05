#include "SkyboxCommon.hlsli"

TextureCube<float4> EnvironmentTexture : register(t0);
SamplerState EnvironmentSampler : register(s0);

float3 ToneMap(float3 color)
{
    const float3 numerator = color * (2.51f * color + 0.03f);
    const float3 denominator = color * (2.43f * color + 0.59f) + 0.14f;
    return pow(saturate(numerator / denominator), 1.0f / 2.2f);
}

float3 RotateEnvironmentDirection(float3 direction)
{
    float sine;
    float cosine;
    sincos(EnvironmentParameters.y, sine, cosine);
    return float3(
        direction.x * cosine - direction.z * sine,
        direction.y,
        direction.x * sine + direction.z * cosine);
}

[RootSignature(SkyboxRootSignature)]
float4 main(SkyboxOutput input) : SV_Target0
{
    const float4 clipPosition = float4(
        input.TexCoord.x * 2.0f - 1.0f,
        1.0f - input.TexCoord.y * 2.0f,
        0.0f,
        1.0f);
    const float4 worldPosition = mul(clipPosition, InverseViewProjection);
    const float3 viewDirection = normalize(
        worldPosition.xyz / worldPosition.w - EyePosition.xyz);
    const float3 color = ToneMap(EnvironmentTexture.SampleLevel(
        EnvironmentSampler,
        RotateEnvironmentDirection(viewDirection),
        0.0f).rgb * EnvironmentParameters.x);
    return float4(color, 1.0f);
}
