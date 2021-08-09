//--------------------------------------------------------------------------------------
// Render3VS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// we don't use any input vertex data except a system generated value for the vertex index
// first we manually read the vertex, then we move it to the corner of the particle
[RootSignature(CommonRS)]
VSOut main(uint vertexIdx : SV_VertexID)
{
    const uint sourceIndex = vertexIdx / 3;
    const uint vi = vertexIdx % 3;

    VSIn vertex = ReadVertex(sourceIndex);

    return VSRender3Helper(vertex, vi);
}
