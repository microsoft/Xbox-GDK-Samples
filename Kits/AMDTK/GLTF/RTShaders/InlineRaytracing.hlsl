//--------------------------------------------------------------------------------------
// InlineRaytracing.hlsl
//
// Single HLSL file containing inline raytracing simple RT shadow implementation.
//
// Modifications Copyright (C) 2020. Advanced Micro Devices, Inc. All Rights Reserved.
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef __XBOX_SCARLETT
#define __XBOX_ENABLE_WAVE32 1
#include "XDXRStandaloneTraverseShortStack.hlsli"
#endif

#include "GlobalRootSignature.hlsl"
#include "RTShared.hlsli"


[RootSignature(GlobalRootSignature)]
[numthreads(8, 4, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const uint2 did = DTid.xy;
    const float2 uv = (did.xy + 0.5f) * float2(DispatchWidthInverse, DispatchHeightInverse);   // normalized texture coordinates
    const float2 ndc = 2.0f * float2(uv.x, 1.0f - uv.y) - 1.0f;     // to clip space
    const float depth = DepthBuffer.Load(int3(did.xy, 0)).x;   // non-linear depth

    // Do not raytrace the sky pixels
    bool is_valid = all(did.xy < uint2(DispatchWidth, DispatchHeight)) && (depth > 0.0f && depth < 1.0f);

    if (is_valid)
    {
        // Recover the world-space position
        const float4 homogeneous = mul(CameraViewProjInverse, float4(ndc, depth, 1.0f));
        const float3 world_position = homogeneous.xyz / homogeneous.w;  // perspective divide

        const float3 normal = normalize(2.0f * NormalBuffer.Load(int3(did.xy, 0)).xyz - 1.0f);

        RayDesc rayDesc;
        FillRayDesc(did, world_position, normal, rayDesc);

#ifdef __XBOX_SCARLETT
        XboxRayQuery q;
        q.TraceRayInlineNoAHSNoIS(Scene, 0
            | RAY_FLAG_CULL_BACK_FACING_TRIANGLES
            | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
            | RAY_FLAG_FORCE_OPAQUE
            | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER
            | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES, ~0, rayDesc);
#else
        RayQuery<RAY_FLAG_NONE> q;
        q.TraceRayInline(Scene, 0
            | RAY_FLAG_CULL_BACK_FACING_TRIANGLES
            | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
            | RAY_FLAG_FORCE_OPAQUE
            | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER
            | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES, ~0, rayDesc);
#endif
        q.Proceed();

        if (q.CommittedStatus() == COMMITTED_NOTHING)
        {
            renderOutput[did] = 1;
        }
        else
        {
            renderOutput[did] = 0;
        }
    }
    else
    {
        //Denoiser expects no surface == 0
        renderOutput[did] = 0;
    }
}
