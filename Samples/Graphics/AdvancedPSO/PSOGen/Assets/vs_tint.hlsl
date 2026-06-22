//-----------------------------------------------------------------------------
// vs_tint.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "common.hlsli"

// Vertex shader: pixel lighting + texture + animated color
VSOutputPixelLightingTx main(VSInputNmTx vin)
{
    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, vin.Normal);
	VSOutputPixelLightingTx vout = CreateVSOutput(cout.Pos_ps, cout.Pos_ws, cout.Normal_ws, cout.FogFactor, DiffuseColor.a, vin.TexCoord);
 
    // distort colors
    vout.Diffuse *= sin((GlobalTime + vin.Position.x) * 10.0) / 3.f + 0.66f;

    return vout;
}