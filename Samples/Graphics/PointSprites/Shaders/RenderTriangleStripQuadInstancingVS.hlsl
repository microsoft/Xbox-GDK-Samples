//--------------------------------------------------------------------------------------
// RenderTriangleStripQuadInstancingVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// triangle strip expansion on VS with instancing
[RootSignature(CommonRS)]
VSOut main(VSIn vertex, uint vertexIdx : SV_VertexID)
{
    const float sz = vertex.posSize.z;
    const float2 org = vertex.posSize.xy;

    const float2 verts[4] =
    {
        float2(0, 1),
        float2(0, 0),
        float2(1, 1),
        float2(1, 0)
    };

    VSOut v;

    v.uv = verts[vertexIdx];
    v.clr = vertex.clr;
    v.pos = NDC(org + (verts[vertexIdx] - 0.5f) * sz);

    return v;
}
