//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Renders interpolated vertex color
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct Pixel
{
    float4 color            : SV_TARGET0;
};

struct InterpolantsColor
{
    float4 position         : SV_POSITION0;
    float4 color            : COLOR0;
};

//-------------------------------------------------------------------------------------------------------------
// Name: PSColor()
// Desc: Draw using vertex color.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
Pixel main(InterpolantsColor In)
{
    Pixel Out;
    Out.color = In.color;
    return Out;
}
