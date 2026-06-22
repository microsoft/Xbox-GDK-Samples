//-----------------------------------------------------------------------------
// vs_morph.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "common.hlsli"

// Vertex shader: pixel lighting + texture + animated verts
VSOutputPixelLightingTx main(VSInputNmTx vin)
{
    // distort vertices for a wobble effect (normals unaffected)
    float4 v = vin.Position;
    v.y += sin((GlobalTime + v.x) * 5.0f) * 1.2;

    CommonVSOutputPixelLighting cout = ComputeCommonVSOutputPixelLighting(v, vin.Normal);
    VSOutputPixelLightingTx vout = CreateVSOutput(cout.Pos_ps, cout.Pos_ws, cout.Normal_ws, cout.FogFactor, DiffuseColor.a, vin.TexCoord);

    return vout;
}