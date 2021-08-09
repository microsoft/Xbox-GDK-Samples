//--------------------------------------------------------------------------------------
// Render4InstancingVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// quad expansion on VS with instancing
[RootSignature(CommonRS)]
VSOut main(VSIn vertex, uint vertexIdx : SV_VertexID)
{
    return VSRender4Helper(vertex, vertexIdx % 6);
}
