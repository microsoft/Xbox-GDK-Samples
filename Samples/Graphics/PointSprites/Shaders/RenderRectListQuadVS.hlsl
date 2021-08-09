//--------------------------------------------------------------------------------------
// RenderRectListQuadVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

[RootSignature(CommonRS)]
VSOut main(uint vertexIdx : SV_VertexID)
{
    const uint sourceIndex = vertexIdx / 3;
    const uint vi = vertexIdx % 3;

    VSIn vertex = ReadVertex(sourceIndex);

    const float size = vertex.posSize.z;
    const float2 position = vertex.posSize.xy;

    const float2 verts[3] =
    {
        float2(0, 0),
        float2(1, 0),
        float2(1, 1)
    };

    VSOut v;

    v.uv = verts[vi];
    v.clr = vertex.clr;
    v.pos = NDC(position + (verts[vi] - 0.5f) * size);

    return v;
}
