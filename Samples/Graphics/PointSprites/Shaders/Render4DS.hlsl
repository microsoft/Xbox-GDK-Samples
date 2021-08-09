//--------------------------------------------------------------------------------------
// Render4DS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// domain shader to generate actual quad vertices
[domain("quad")]
[RootSignature(CommonRS)]
VSOut main(const float2 uv : SV_DomainLocation, const OutputPatch< HSOutEmpty, 1 > patch, const HSPatchData4 patchData)
{
    VSOut v;

    const float sz = patchData.vertex.posSize.z;
    const float2 org = patchData.vertex.posSize.xy;

    float2 sizeScaler = uv - 0.5f; // uv is [0..1] we need [-0.5..0.5] here

    v.uv = uv;
    v.clr = patchData.vertex.clr;
    v.pos = NDC(org + sz.xx * sizeScaler);

    return v;
}
