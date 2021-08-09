//--------------------------------------------------------------------------------------
// Render4VS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// quad expansion on VS without instancing
[RootSignature(CommonRS)]
VSOut main(uint vertexIdx : SV_VertexID)
{
    const uint sourceIndex = vertexIdx / 6;
    const uint vi = vertexIdx % 6;

    VSIn vertex = ReadVertex(sourceIndex);

    return VSRender4Helper(vertex, vi);
}
