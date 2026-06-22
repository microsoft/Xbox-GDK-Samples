//--------------------------------------------------------------------------------------
// HiZDecodeDepth.hlsl
//
// Overlay shader for rendering interpreted htile depth
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

#ifndef REVERSE_Z
#define REVERSE_Z 1
#endif

cbuffer cbThumbnail : register(b0)
{
    float2 g_vDims;         // These are frame dimensions divided by tile size,
                            // which may differ fractionally from htile dimensions.
    float2 g_vPadding;
    float g_fFarNearRatio;  // needed for re-linearizing depth
};

//-------------------------------------------------------------------------------------------------------------
// Name: LinearizeDepth()
// Desc: Convert perspective depth back into normalized linear depth. This equation is not literally valid for 
// hi z, since hi z treats float depth as fixed point. But it's good enough for visualization purposes.
//-------------------------------------------------------------------------------------------------------------
float2 LinearizeDepth( float2 fIn )
{
#if REVERSE_Z
    return 1.0 / (g_fFarNearRatio * fIn + 1.0);
#else
    return fIn / (g_fFarNearRatio - (g_fFarNearRatio - 1.0) * fIn);
#endif
}

//-------------------------------------------------------------------------------------------------------------
// Name: InterpretHiZ()
// Desc: Hi z comes in as zMin and zMax in a natural range of
//
//      Hi z in [ 0, 2 << 14 )
//
//  (In the case of delta compression, hi z can slightly exceed these limits on both ends.)
//
//  To interpret hi z in terms of depth, all z in the tile are between
//
//      z in [ zMin / ( 1 << 14 ), ( zMax + 1 ) / ( 1 << 14 ) )
//
//  This assumes a D32F z format. For D16 and D24, the values should be adjusted by 
//  2^15 / ( 2^15 - 1 ) or 2^24 / ( 2^24 - 1 ) respectively.
//-------------------------------------------------------------------------------------------------------------
float2 InterpretHiZ( int2 zMinMax )
{
#if REVERSE_Z
    return int2(zMinMax.y + 1, zMinMax.x) / float(1U << 14);
#else
    return int2(zMinMax.x, zMinMax.y + 1) / float(1U << 14);
#endif
}


//-------------------------------------------------------------------------------------------------------------
// Name: PSThumbnailHiZ()
// Desc: Pixel shader which displays hi z as color
//-------------------------------------------------------------------------------------------------------------
Texture2D<int2> g_texThumbHiZ : register(t0);

[RootSignature(FullScreenQuadRS)]
float4 PSThumbnailHiZ(Interpolators In) : SV_TARGET0
{
    return LinearizeDepth(InterpretHiZ(g_texThumbHiZ.Load(int3(In.TexCoord * g_vDims, 0)))).xyxy;
}
