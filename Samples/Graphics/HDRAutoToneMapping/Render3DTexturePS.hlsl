//--------------------------------------------------------------------------------------
// Render3DTexturePS.hlsl
//
// A simple pixel shader to render 3D texture
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Render3DTexture.hlsli"

// A simple example/placeholder shader rendering something into a 3D texture with size 32x32x32
static const float Size = 32;

[RootSignature(Render3DTextureRS)]
float4 main(PS_IN In) : SV_Target0
{
    float3 color = float3(In.TexCoord, In.LayerIndex / (Size - 1));
    return float4(color, 1.0f);
}
