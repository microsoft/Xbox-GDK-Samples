//--------------------------------------------------------------------------------------
// HiZDecodeCS.hlsl
//
// DirectCompute shader which decodes htile information
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define HiZDecodeRS \
    "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(UAV(u0, numDescriptors=2), visibility=SHADER_VISIBILITY_ALL)"

#include "DepthDecompressUtility.hlsli"

#ifndef g_bStencil
#define g_bStencil (0 || (_XBOX_ONE == 0)) // One Scarlett always enabled because of fixed HTile encodings
#endif

#ifndef g_bDepthOrderStandard
#define g_bDepthOrderStandard 0
#endif

cbuffer HTileParams : register(b0)
{
    uint HTileInfo;
};

//-------------------------------------------------------------------------------------------------------------
// Name: DecodeDelta()
// Desc: When stencil is mapped, hi z consists of a 14-bit base and a 6-bit delta, rather than a min and a max.
//  The delta has a custom format, described below.
//-------------------------------------------------------------------------------------------------------------
uint DecodeDelta( uint zDelta )
{
    // If 6-bit zDelta is of the form [Delta], then 14-bit decode is of the form [Decode].
    // The 'D's are arbitrary binary digits.
    //
    // [Delta] [Decode]
    // ------  --------------
    // 000DDD  00000000000DDD
    // 001DDD  00000000001DDD
    // 010DDD  0000000001DDD1
    // 011DDD  000000001DDD11
    // 1000DD  00000001DD1111
    // 1001DD  0000001DD11111
    // 1010DD  000001DD111111
    // 1011DD  00001DD1111111
    // 1100DD  0001DD11111111
    // 1101DD  001DD111111111
    // 1110DD  01DD1111111111
    // 1111DD  1DD11111111111

    uint iTotalBits = 6;    // hardware value

    // Highest order bit of Delta determines whether there are 3 'D's or 2 'D's
    bool bTwoBitDelta = __XB_UBFE( 1, 5, zDelta );

    // The 'D's
    uint iDeltaBits = bTwoBitDelta ? 2 : 3;
    uint iDelta = __XB_UBFE( iDeltaBits, 0, zDelta );

    // The rest of the Delta bits
    uint iCodeBits = 3;
    uint iCode = __XB_UBFE( iCodeBits, iDeltaBits, zDelta );

    // Highest order Decode bit with a '1' in it
    uint iLeadingOnePlace = iCode + ( bTwoBitDelta ? 6 : 2 );
    uint iOnes = ( 1U << ( iLeadingOnePlace + 1 ) ) - 1;

    // Lowest order Decode bit with a 'D' in it
    uint iDeltaStart = ( iLeadingOnePlace >= iDeltaBits ) ? ( iLeadingOnePlace - iDeltaBits ) : 0;

    // Mask where the 'D's go
    uint iMask = __XB_BFM( iDeltaBits, iDeltaStart );
    return __XB_BFI( iMask, iDelta << iDeltaStart, iOnes );
}

//-------------------------------------------------------------------------------------------------------------
// Name: ExtractHiZ()
// Desc: Extract the portion of htile which represents hi z, and decode to 14-bit integer min and max
//-------------------------------------------------------------------------------------------------------------
uint2 ExtractHiZ( uint iHtile )
{
    if( g_bStencil )
    {
        // These are a 14-bit fixed-point value and a 6-bit delta code
        uint zBase = __XB_UBFE( 14, 18, iHtile );
        uint zDelta = __XB_UBFE( 6, 12, iHtile );

        // The base is the closest to the near plane, and the delta is towards the far plane
        uint zMin = g_bDepthOrderStandard ? ( zBase - DecodeDelta( zDelta ) ) : zBase;
        uint zMax = g_bDepthOrderStandard ? zBase : ( zBase + DecodeDelta( zDelta ) );

        return uint2( zMin, zMax );
    }
    else
    {
        // These are 14-bit fixed-point values
        uint zMin = __XB_UBFE( 14,  4, iHtile );
        uint zMax = __XB_UBFE( 14, 18, iHtile );

        return uint2( zMin, zMax );
    }
}

//-------------------------------------------------------------------------------------------------------------
// Name: ExtractHiStencil()
// Desc: Extract the portion of htile which represents hi stencil, and decode to 2 2-bit values
//-------------------------------------------------------------------------------------------------------------
uint2 ExtractHiStencil( uint iHtile )
{
    if( g_bStencil )
    {
        uint hiS0 = __XB_UBFE(  2,  4, iHtile );
        uint hiS1 = __XB_UBFE(  2,  6, iHtile );

#if _XBOX_ONE // On Scarlett there's no second pretest value
        return uint2( hiS0, hiS1 );
#else
        return uint2( hiS0, 0 );
#endif
    }
    else
    {
        return uint2( 0, 0 );
    }
}

//-------------------------------------------------------------------------------------------------------------
// Name: CSHtileDecode()
// Desc: Compute shader which decodes hi z from htile (with or without stencil).
//
//  Performance of this shader is not too critical, as the htile surface is small. Total time is around 10 us
//  for a 1920x1080 depth buffer.
//
//  For larger surfaces (e.g. shadow map arrays), a few instructions could be shaved off.
//
//  The shader outputs hi z and hi s in their natural units:
//
//      Hi z in [ 0, 2 << 14 )
//      Hi s in [ 1, 3 ]
//
//  (In the case of delta compression, hi z can slightly exceed these limits on both ends.)
//
//  To interpret these values in terms of z and stencil, see InterpretHiZ and InterpretHiS.
//-------------------------------------------------------------------------------------------------------------
ByteAddressBuffer g_bufHtile    : register(t0);
RWTexture2D<uint2> g_texHiz     : register(u0);
RWTexture2D<uint2> g_texHis     : register(u1);

[RootSignature(HiZDecodeRS)]
[numthreads(8,8,1)]
void CSHtileDecode( uint3 id : SV_DispatchThreadID )
{
    // Easier to make the input a buffer, since 2D texture tiling won't match htile anyhow
    uint iHtile = g_bufHtile.Load(GetHTileAddress(id.xy, HTileInfo));

    g_texHiz[ id.xy ] = ExtractHiZ( iHtile );

    if( g_bStencil )
    {
        g_texHis[ id.xy ] = ExtractHiStencil( iHtile );
    }
}
