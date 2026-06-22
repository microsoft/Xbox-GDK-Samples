//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Renders a constant color
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

static const float4 chartreuse = float4(0.5f, 1.0f, 0.0f, 1.0f);

struct Pixel
{
    float4 color            : SV_TARGET0;
};

struct InterpolantsConstantColor
{
    float4 position         : SV_POSITION0;
};

//-------------------------------------------------------------------------------------------------------------
// Name: PSConstantColor()
// Desc: Draw a constant color.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
Pixel main(InterpolantsConstantColor In)
{
    Pixel Out;
    Out.color = chartreuse;
    return Out;
}

