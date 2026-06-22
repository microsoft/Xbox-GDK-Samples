//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Renders a texture
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct Pixel
{
    float4 color            : SV_TARGET0;
};

struct InterpolantsTexture
{
    float4 position         : SV_POSITION0;
    float2 texcoord         : TEXCOORD0;
};

Texture2D tex : register(t0);

//-------------------------------------------------------------------------------------------------------------
// Name: PSTexture()
// Desc: Draw a textured mesh.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
Pixel main(InterpolantsTexture In)
{
    Pixel Out;

    uint2 dims;
    tex.GetDimensions(dims.x, dims.y);

    Out.color = tex.Load(int3(floor(In.texcoord * dims), 0));

    return Out;
}

