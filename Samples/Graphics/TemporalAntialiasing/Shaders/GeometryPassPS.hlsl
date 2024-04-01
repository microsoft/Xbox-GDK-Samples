//--------------------------------------------------------------------------------------
// GeometryPassPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "GeometryPassCommon.hlsli"

Texture2D<float4> texDiffuse    : register(t0);

ConstantBuffer<SceneCB> sceneCts : register(b0);
ConstantBuffer<PerObjectCB> perObjCts : register(b1);

sampler diffuseSampler          : register(s0);

[RootSignature(GeometryPassRS)]
PSOutputs main(PSInterpolators IN)
{
    PSOutputs psOut;

    psOut.gbufferAlbedo = texDiffuse.SampleBias(diffuseSampler, IN.texcoords, sceneCts.samplingBias) * float4(perObjCts.diffuseColor, 1.0f);
    psOut.gbufferNormals = IN.normals;

    // Transform from clip space to UV [0.0f, 1.0f].
    float2 previousFrameNDC = IN.previousFrameClip.xy / IN.previousFrameClip.z;
    float2 currentFrameNDC = IN.currentFrameClip.xy / IN.currentFrameClip.z;

    // Remove Jitter (jitter is passed in NDC space).
    currentFrameNDC -= sceneCts.jitterNDC.xy;
    previousFrameNDC -= sceneCts.jitterNDC.xy;

    // Flipping Y (since origin is at top left in directx)
    float2 previousFrameUV = previousFrameNDC * float2(0.5f, -0.5f) + 0.5f;
    float2 currentFrameUV = currentFrameNDC * float2(0.5f, -0.5f) + 0.5f;

    float2 velocityUV = previousFrameUV - currentFrameUV;

    // Transform UV to Screen Space.
    psOut.gbufferMotion = velocityUV * (2.0f * sceneCts.halfRes);

    return psOut;
}
