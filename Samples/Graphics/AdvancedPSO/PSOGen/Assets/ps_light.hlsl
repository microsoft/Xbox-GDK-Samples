//-----------------------------------------------------------------------------
// ps_light.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "common.hlsli"

// Pixel shader: pixel lighting only 
float4 main(PSInputPixelLightingTx pin) : SV_Target0
{
    float4 color = pin.Diffuse;

    ColorPair lightResult;
    ApplyLighting(color, lightResult, EyePosition, pin.PositionWS.xyz, pin.NormalWS);
    AddSpecular(color, lightResult.Specular);
    ApplyFog(color, pin.PositionWS.w);

    return color;
}