//--------------------------------------------------------------------------------------
// Render3DTextureGS.hlsl
//
// Simple geometry shader to draw a 3D texture
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Render3DTexture.hlsli"

[RootSignature(Render3DTextureRS)]
[maxvertexcount(3)]
void main(triangle GS_IN input[3], inout TriangleStream<PS_IN> stream)
{
    [unroll] for (int vertex = 0; vertex < 3; vertex++)
    {
        PS_IN output;
        output.Position = input[vertex].Position;
        output.TexCoord = input[vertex].TexCoord;
        output.LayerIndex = input[vertex].LayerIndex;
        stream.Append(output);
    }
}
