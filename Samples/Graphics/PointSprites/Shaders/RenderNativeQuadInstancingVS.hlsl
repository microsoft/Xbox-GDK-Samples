//--------------------------------------------------------------------------------------
// RenderNativeQuadInstancingVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// native quad expansion on VS with instancing
[RootSignature(CommonRS)]
VSOut main(VSIn vertex, uint vertexIdx : SV_VertexID)
{
    return VSRenderNativeQuadHelper(vertex, vertexIdx % 4);
}
