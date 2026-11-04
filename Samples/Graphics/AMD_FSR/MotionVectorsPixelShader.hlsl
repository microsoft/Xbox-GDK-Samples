#include "PBRCommon.hlsli"

[RootSignature(PBRRootSignature)]
float2 main(MotionVSOutput input) : SV_Target0
{
    const float2 currentNdc = input.CurrentClipPosition.xy / input.CurrentClipPosition.w;
    const float2 previousNdc = input.PreviousClipPosition.xy / input.PreviousClipPosition.w;

    // Convert clip-space NDC (-1..1, Y-up) to UV space (0..1, Y-down) and store the
    // backward-looking displacement (current -> previous), the convention FSR/FFX-style
    // upscalers expect for motion vectors: currentUV + motionVector = previousUV.
    const float2 currentUV = currentNdc * float2(0.5f, -0.5f) + 0.5f;
    const float2 previousUV = previousNdc * float2(0.5f, -0.5f) + 0.5f;

    return previousUV - currentUV;
}
