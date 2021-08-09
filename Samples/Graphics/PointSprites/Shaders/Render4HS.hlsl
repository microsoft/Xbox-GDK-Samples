//--------------------------------------------------------------------------------------
// Render4HS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// hull per-patch shader for quad tesselation
HSPatchData4 HSPatchCFunc4(const InputPatch< VSOutGSIn, 1 > points)
{
    HSPatchData4 d;

    d.vertex = points[0];

    d.edges[0] = d.edges[1] = d.edges[2] = d.edges[3] = 1;
    d.inside[0] = d.inside[1] = 1;

    return d;
}

// quad per-control point hull shader
[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[patchconstantfunc("HSPatchCFunc4")]
[outputcontrolpoints(1)]
[RootSignature(CommonRS)]
HSOutEmpty main(const uint id : SV_OutputControlPointID, const InputPatch< VSOutGSIn, 1 > points)
{
    return (HSOutEmpty)0;
}
