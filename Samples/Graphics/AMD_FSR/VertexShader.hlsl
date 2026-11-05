#include "PBRCommon.hlsli"

[RootSignature(PBRRootSignature)]
VSOutput main(VSInput input)
{
    VSOutput output;
    const float4 positionWS = mul(float4(input.Position, 1.0f), World);

    output.Position = mul(positionWS, ViewProjection);
    output.PositionWS = positionWS.xyz;
    output.NormalWS = normalize(mul(float4(input.Normal, 0.0f), WorldInverseTranspose).xyz);
    output.TexCoord = input.TexCoord;
    output.ShadowPosition = mul(positionWS, LightViewProjection);
    return output;
}
