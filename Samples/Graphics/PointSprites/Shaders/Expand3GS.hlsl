//--------------------------------------------------------------------------------------
// Expand3GS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// geometry shader that outputs 3 vertices from a point
[maxvertexcount(3)]
[RootSignature(CommonRS)]
void main(point VSOutGSIn points[1], inout TriangleStream< VSOut > stream)
{
    VSOut v;

    const float2 verts[3] =
    {
        float2(-0.5f, -0.5f),
        float2(1.5f, -0.5f),
        float2(-0.5f,  1.5f)
    };

    const float sz = points[0].posSize.z;
    const float2 org = points[0].posSize.xy;
    const float4 clr = points[0].clr;

    // triangle strip for the particle

    v.uv = float2(0, 0);
    v.clr = clr;
    v.pos = NDC(org + verts[0] * sz);
    stream.Append(v);

    v.uv = float2(2, 0);
    v.clr = clr;
    v.pos = NDC(org + verts[1] * sz);
    stream.Append(v);

    v.uv = float2(0, 2);
    v.clr = clr;
    v.pos = NDC(org + verts[2] * sz);
    stream.Append(v);

    stream.RestartStrip();
}
