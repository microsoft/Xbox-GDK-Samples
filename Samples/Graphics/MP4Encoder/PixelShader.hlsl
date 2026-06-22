//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Simple shader to render a triangle
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE "RootConstants(b0, num32bitconstants=1)"

struct Interpolants
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
};

struct Pixel
{
    float4 Color : SV_TARGET0;
};

[RootSignature(ROOT_SIGNATURE)]
Pixel main(Interpolants In)
{
    Pixel Out;
    Out.Color = In.Color;
    return Out;
}
