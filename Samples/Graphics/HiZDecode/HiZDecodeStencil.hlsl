//--------------------------------------------------------------------------------------
// HiZDecodeStencil.hlsl
//
// Overlay shader for rendering interpreted htile stencil
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

cbuffer cbThumbnail : register(b0)
{
    float2 g_vDims;         // These are frame dimensions divided by tile size,
                            // which may differ fractionally from htile dimensions.
};

//-------------------------------------------------------------------------------------------------------------
// Name: InterpretHiS()
// Desc: Hi stencil state 0 is in the red channel, and hi stencil state 1 is in the green channel.
//
//  Each hi stencil state has possible values 1, 2, 3 (rendered as intensity 64, 128, 192 for clarity):
//      - 1 = No pixels satisfy hi stencil test
//      - 2 = All pixels satisfy hi stencil test
//      - 3 = Some pixels satisfy hi stencil test and some pixels don't (or alternately, value is not up to date)
//
//  Choice of hi stencil states is (currently) controlled by the driver. You can read the hi stencil states
//  in PIX GPU State view, from the registers: 
// 
//      Context - DB - DB_SRESULTS_COMPARE_STATE<0|1>
//-------------------------------------------------------------------------------------------------------------
float2 InterpretHiS( int2 hiS01 )
{
    return hiS01 / float( 1U << 2U );
}

//-------------------------------------------------------------------------------------------------------------
// Name: PSThumbnailHiS()
// Desc: Pixel shader which displays hi stencil as color
//-------------------------------------------------------------------------------------------------------------
Texture2D<uint2> g_texThumbHiS : register(t0);

[RootSignature(FullScreenQuadRS)]
float4 PSThumbnailHiS(Interpolators In) : SV_TARGET0
{
    return float4(InterpretHiS(g_texThumbHiS.Load(int3(In.TexCoord * g_vDims, 0)).xy) , 0.0f, 1.0f);
}
