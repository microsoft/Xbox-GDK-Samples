//-----------------------------------------------------------------------------
// vs_static.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "common.hlsli"

// Vertex shader: pixel lighting + texture
VSOutputPixelLightingTx main(VSInputNmTx vin)
{
    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(vin.Position, vin.Normal);
    VSOutputPixelLightingTx vout = CreateVSOutput(cout.Pos_ps, cout.Pos_ws, cout.Normal_ws, cout.FogFactor, DiffuseColor.a, vin.TexCoord);

    return vout;
}
