//--------------------------------------------------------------------------------------
// Render3DTextureVS.hlsl
//
// Simple vertex shader to draw a 3D Texture
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Render3DTexture.hlsli"

[RootSignature(Render3DTextureRS)]
VS_OUT main(uint vI : SV_VertexId, uint layerIndex : SV_InstanceID)
{
    VS_OUT output;

    float2 texcoord = float2(vI & 1, vI >> 1);
    output.TexCoord = texcoord;
    output.Position = float4((texcoord.x - 0.5f) * 2.0f, -(texcoord.y - 0.5f) * 2.0f, 0.0f, 1.0f);
    output.LayerIndex = layerIndex;

    return output;
}
