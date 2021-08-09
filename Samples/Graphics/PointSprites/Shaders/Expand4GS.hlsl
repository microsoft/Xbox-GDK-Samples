//--------------------------------------------------------------------------------------
// Expand4GS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// geometry shader that outputs 4 vertices from a point
// not that the winding order is for a tristrip
[maxvertexcount(4)]
[RootSignature(CommonRS)]
void main(point VSOutGSIn points[1], inout TriangleStream< VSOut > stream)
{
    const float sz = points[0].posSize.z * 0.5f;
    const float2 org = points[0].posSize.xy;
    const float4 clr = points[0].clr;

    // triangle strip for the particle

    VSOut v[4];

    v[0].uv = float2(0, 0);
    v[0].clr = clr;
    v[0].pos = NDC(org + float2(-sz, -sz));

    v[1].uv = float2(1, 0);
    v[1].clr = clr;
    v[1].pos = NDC(org + float2(+sz, -sz));

    v[2].uv = float2(1, 1);
    v[2].clr = clr;
    v[2].pos = NDC(org + float2(+sz, +sz));

    v[3].uv = float2(0, 1);
    v[3].clr = clr;
    v[3].pos = NDC(org + float2(-sz, +sz));

    stream.Append(v[0]);
    stream.Append(v[1]);
    stream.Append(v[3]);
    stream.Append(v[2]);
    stream.RestartStrip();
}
