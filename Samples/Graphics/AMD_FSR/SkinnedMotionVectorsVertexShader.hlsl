#include "SkinningCommon.hlsli"

[RootSignature(PBRRootSignature)]
MotionVSOutput main(SkinnedVSInput input)
{
    float4 currentSkinnedPosition;
    float3 unusedSkinnedNormal;
    SkinVertex(input, currentSkinnedPosition, unusedSkinnedNormal);
    const float4 previousSkinnedPosition = SkinPreviousPosition(input);

    const float4 currentPositionWS = mul(currentSkinnedPosition, World);
    const float4 previousPositionWS = mul(previousSkinnedPosition, PreviousWorld);

    MotionVSOutput output;
    output.Position = mul(currentPositionWS, ViewProjection);
    output.CurrentClipPosition = output.Position;
    output.PreviousClipPosition = mul(previousPositionWS, PreviousViewProjection);
    return output;
}
