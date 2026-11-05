#include "SkinningCommon.hlsli"

[RootSignature(PBRRootSignature)]
float4 main(SkinnedVSInput input) : SV_Position
{
    float4 skinnedPosition;
    float3 skinnedNormal;
    SkinVertex(input, skinnedPosition, skinnedNormal);

    const float4 positionWS = mul(skinnedPosition, World);
    return mul(positionWS, LightViewProjection);
}
