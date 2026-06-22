//--------------------------------------------------------------------------------------
// RenderAlphaToVRS.hlsl
//
// A custom pixel shader for SpriteBatch that renders alpha values to a smaller render
// target for determining the contents of the VRS Shading Rate Image.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSig.fxh"

Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);

[RootSignature(SpriteStaticRS)]
float main(float4 color    : COLOR0,
    float2 texCoord : TEXCOORD0) : SV_Target0
{
    float alpha = (Texture.Sample(TextureSampler, texCoord)* color).a;
    return alpha;
}
