#include "SmokeParticleCommon.hlsli"

Texture2D<float4> SmokeTexture : register(t0);
SamplerState SmokeSampler : register(s0);

// Read-only copy of the scene's depth buffer (bound after the opaque pass, once nothing else
// writes depth for the rest of the frame). Sampled with Load() using exact pixel coordinates, so
// no separate sampler state is needed.
Texture2D<float> SceneDepthTexture : register(t2);

// Converts a reverse-Z depth-buffer value (0 at the far plane, 1 at the near plane) into a
// linear view-space distance from the camera, given the camera's physical near/far distances.
float LinearizeReverseZDepth(float depth, float nearDistance, float farDistance)
{
    return (nearDistance * farDistance) / (nearDistance + depth * (farDistance - nearDistance));
}

[RootSignature(SmokeParticleRootSignature)]
float4 main(SmokeParticleOutput input) : SV_Target0
{
    // Cross-fade between the current and next sprite-sheet frame (instead of hard-cutting at
    // each frame boundary) so the animation reads as continuous, soft motion rather than a
    // slideshow of stills, even at a low playback rate.
    const float4 texCurrent = SmokeTexture.Sample(SmokeSampler, input.TexCoord);
    const float4 texNext = SmokeTexture.Sample(SmokeSampler, input.TexCoordNextAndBlend.xy);
    const float4 tex = lerp(texCurrent, texNext, input.TexCoordNextAndBlend.z);
    float alpha = tex.a * input.Color.a;

    // Soft particles: fade the billboard out as it nears intersecting scene geometry (e.g. the
    // floor), instead of being clipped by a hard depth test edge.
    const float sceneDepthValue = SceneDepthTexture.Load(int3(int2(input.Position.xy), 0));
    const float sceneViewDistance = LinearizeReverseZDepth(sceneDepthValue, CameraForwardAndNear.w, EyePositionAndFar.w);
    const float particleViewDistance = dot(input.WorldPosition - EyePositionAndFar.xyz, CameraForwardAndNear.xyz);
    const float depthFadeDistance = max(SettleHeightRange.z, 0.0001f);
    const float depthFade = saturate((sceneViewDistance - particleViewDistance) / depthFadeDistance);
    alpha *= depthFade;

    clip(alpha - 0.01f);
    return float4(tex.rgb * input.Color.rgb, alpha);
}
