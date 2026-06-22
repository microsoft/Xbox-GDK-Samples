//------------------------------------------------------------------------------------
// AoTexPS.hlsl
//
// Simple shader to render the AO texture
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "AoTex.hlsli"

[RootSignature(MainRS)]
Pixel main(Interpolants In)
{
    Pixel Out;
    uint2 pixelPos = uint2(In.position.xy);
    float ao = texAO[pixelPos];
    Out.color = float4(ao.rrr, 1.0f);
    return Out;
}
