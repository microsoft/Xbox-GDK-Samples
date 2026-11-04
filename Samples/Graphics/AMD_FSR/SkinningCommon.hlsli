#include "PBRCommon.hlsli"

static const uint MaxJoints = 128;

cbuffer JointConstants : register(b2)
{
    float4x4 JointPalette[MaxJoints];
}

// Only bound for the skinned motion-vectors pass, holding last frame's joint palette so the
// previous-frame skinned position can be reconstructed alongside the current one.
cbuffer PreviousJointConstants : register(b3)
{
    float4x4 PreviousJointPalette[MaxJoints];
}

struct SkinnedVSInput
{
    float3 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    uint4 JointIndices : BLENDINDICES0;
    float4 JointWeights : BLENDWEIGHT0;
};

void SkinVertex(
    SkinnedVSInput input,
    out float4 skinnedPosition,
    out float3 skinnedNormal)
{
    const float weightSum = dot(input.JointWeights, float4(1.0f, 1.0f, 1.0f, 1.0f));
    const float4 weights = weightSum > 0.0f
        ? input.JointWeights / weightSum
        : float4(1.0f, 0.0f, 0.0f, 0.0f);

    skinnedPosition =
        mul(float4(input.Position, 1.0f), JointPalette[input.JointIndices.x]) * weights.x
        + mul(float4(input.Position, 1.0f), JointPalette[input.JointIndices.y]) * weights.y
        + mul(float4(input.Position, 1.0f), JointPalette[input.JointIndices.z]) * weights.z
        + mul(float4(input.Position, 1.0f), JointPalette[input.JointIndices.w]) * weights.w;

    skinnedNormal =
        mul(float4(input.Normal, 0.0f), JointPalette[input.JointIndices.x]).xyz * weights.x
        + mul(float4(input.Normal, 0.0f), JointPalette[input.JointIndices.y]).xyz * weights.y
        + mul(float4(input.Normal, 0.0f), JointPalette[input.JointIndices.z]).xyz * weights.z
        + mul(float4(input.Normal, 0.0f), JointPalette[input.JointIndices.w]).xyz * weights.w;
}

// Same blend as SkinVertex, but against last frame's joint palette and position-only (no normal
// is needed for motion vectors).
float4 SkinPreviousPosition(SkinnedVSInput input)
{
    const float weightSum = dot(input.JointWeights, float4(1.0f, 1.0f, 1.0f, 1.0f));
    const float4 weights = weightSum > 0.0f
        ? input.JointWeights / weightSum
        : float4(1.0f, 0.0f, 0.0f, 0.0f);

    return mul(float4(input.Position, 1.0f), PreviousJointPalette[input.JointIndices.x]) * weights.x
        + mul(float4(input.Position, 1.0f), PreviousJointPalette[input.JointIndices.y]) * weights.y
        + mul(float4(input.Position, 1.0f), PreviousJointPalette[input.JointIndices.z]) * weights.z
        + mul(float4(input.Position, 1.0f), PreviousJointPalette[input.JointIndices.w]) * weights.w;
}
