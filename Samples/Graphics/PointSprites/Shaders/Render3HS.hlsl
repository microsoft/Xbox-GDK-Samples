//--------------------------------------------------------------------------------------
// Render3HS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// hull per-patch shader for expanding a point to a triangle
HSPatchData3 HSPatchCFunc3(const InputPatch< VSOutGSIn, 1 > points)
{
    HSPatchData3 d;

    d.vertex = points[0];

    d.edges[0] = d.edges[1] = d.edges[2] = 1;
    d.inside = 1;

    return d;
}

// hull per-control point shader
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[patchconstantfunc("HSPatchCFunc3")]
[outputcontrolpoints(1)]
[RootSignature(CommonRS)]
HSOutEmpty main(const uint id : SV_OutputControlPointID, const InputPatch< VSOutGSIn, 1 > points)
{
    return (HSOutEmpty)0;
}
