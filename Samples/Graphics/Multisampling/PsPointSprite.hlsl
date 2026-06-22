//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Renders one of two textures, based on the "fragment" interpolant
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct Pixel
{
    float4 color            : SV_TARGET0;
};

struct InterpolantsPointSprite
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
    nointerpolation bool fragment : FRAGMENT;
};

#include "EQAA.hlsli"

Texture2D texFragment   : register(t1);
Texture2D texSample     : register(t2);

//-------------------------------------------------------------------------------------------------------------
// Name: PSPointSprite()
// Desc: Draw a point sprite for the sample dots.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
Pixel main(InterpolantsPointSprite In)
{
    Pixel Out;

    uint2 dims;
    texFragment.GetDimensions(dims.x, dims.y);
    int2 texcoord = floor(In.texcoord * dims);

    Out.color = In.fragment
        ? texFragment.Load(int3(texcoord, 0))
        : texSample.Load(int3(texcoord, 0));

    return Out;
}
