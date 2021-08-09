//--------------------------------------------------------------------------------------
// RenderNativeQuadVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// native quad expansion on VS without instancing
[RootSignature(CommonRS)]
VSOut main(uint vertexIdx : SV_VertexID)
{
    const uint sourceIndex = vertexIdx / 4;
    const uint vi = vertexIdx % 4;

    VSIn vertex = ReadVertex(sourceIndex);

    return VSRenderNativeQuadHelper(vertex, vi);
}
