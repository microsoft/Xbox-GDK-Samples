#include "SkinningCommon.hlsli"

[RootSignature(PBRRootSignature)]
VSOutput main(SkinnedVSInput input)
{
    float4 skinnedPosition;
    float3 skinnedNormal;
    SkinVertex(input, skinnedPosition, skinnedNormal);

    VSOutput output;
    const float4 positionWS = mul(skinnedPosition, World);
    output.Position = mul(positionWS, ViewProjection);
    output.PositionWS = positionWS.xyz;
    output.NormalWS = normalize(mul(float4(skinnedNormal, 0.0f), WorldInverseTranspose).xyz);
    output.TexCoord = input.TexCoord;
    output.ShadowPosition = mul(positionWS, LightViewProjection);
    return output;
}
