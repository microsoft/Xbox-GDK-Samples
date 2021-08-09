//--------------------------------------------------------------------------------------
// Render3InstancingVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// expand in the VS using instancing
// this requires streams set up so that the source vertex data pointer increments for each 3 vertices
// we still need the vertex index to know which corner it is, but the vertex fetch is done for us
// by geoemtry instancing
[RootSignature(CommonRS)]
VSOut main(VSIn vertex, uint vertexIdx : SV_VertexID)
{
    return VSRender3Helper(vertex, vertexIdx % 3);
}
