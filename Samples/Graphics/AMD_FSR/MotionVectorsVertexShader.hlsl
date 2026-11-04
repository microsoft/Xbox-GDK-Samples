#include "PBRCommon.hlsli"

[RootSignature(PBRRootSignature)]
MotionVSOutput main(VSInput input)
{
    MotionVSOutput output;
    const float4 positionOS = float4(input.Position, 1.0f);
    const float4 currentPositionWS = mul(positionOS, World);
    const float4 previousPositionWS = mul(positionOS, PreviousWorld);

    output.Position = mul(currentPositionWS, ViewProjection);
    output.CurrentClipPosition = output.Position;
    output.PreviousClipPosition = mul(previousPositionWS, PreviousViewProjection);
    return output;
}
