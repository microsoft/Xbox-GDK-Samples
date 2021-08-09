//--------------------------------------------------------------------------------------
// Render3DS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// domain shader that actually outputs the triangle vertices
[domain("tri")]
[RootSignature(CommonRS)]
VSOut main(const float3 bary : SV_DomainLocation, const OutputPatch< HSOutEmpty, 1 > patchUnused, const HSPatchData3 patchData)
{
    VSOut v;

    const float sz = patchData.vertex.posSize.z;
    const float2 org = patchData.vertex.posSize.xy;

    const float2 verts[3] =
    {
        float2(-0.5f, -0.5f),
        float2(1.5f, -0.5f),
        float2(-0.5f,  1.5f)
    };

    // for the uv we assume (0, 0) - (2, 0) - (0, 2)
    v.uv = bary.zy * 2;
    v.clr = patchData.vertex.clr;
    v.pos = NDC(org + verts[0] * bary.x * sz + verts[1] * bary.y * sz + verts[2] * bary.z * sz);

    return v;
}
