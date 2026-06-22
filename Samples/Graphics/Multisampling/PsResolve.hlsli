//--------------------------------------------------------------------------------------
// PsResolve.hlsli
//
// Common code for resolve using a pixel shader
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

Texture2DMS<float4> texMS : register(t0);
Texture2D<uint2> texFMask : register(t1);

#include "EQAA.hlsli"
#include "FMask.hlsli"

struct Pixel
{
    float4 color            : SV_TARGET0;
};

struct InterpolantsTexture
{
    float4 position         : SV_POSITION0;
    float2 texcoord         : TEXCOORD0;
};

//-------------------------------------------------------------------------------------------------------------
// Name: PSManualResolve()
// Desc: Manually resolve the MS texture by reading FMask and Color and writing to render target.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
Pixel main(InterpolantsTexture In)
{
    Pixel Out;

#if ENABLE_EQAA
    uint2 dims;
    uint samples;
    GetDimensions(texMS, dims.x, dims.y, samples);

    Out.color = ManualResolve(texMS, texFMask, floor(In.texcoord * dims));
#else
    Out.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif

    return Out;
}
