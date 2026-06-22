//-----------------------------------------------------------------------------
// ps_texture.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "common.hlsli"

// Pixel shader: texture only
float4 main(PSInputPixelLightingTx pin) : SV_Target0
{
    return Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
}