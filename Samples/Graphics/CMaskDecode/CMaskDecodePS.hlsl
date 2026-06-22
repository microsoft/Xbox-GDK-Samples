//--------------------------------------------------------------------------------------
// CMaskDecodePS.hlsl
//
// Overlay shader for rendering interpreted cmask.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef g_bSubTile
#define g_bSubTile 0
#endif

#include "FullScreenQuad.hlsli"

//-------------------------------------------------------------------------------------------------------------
// Name: PSOverlay()
// Desc: Pixel shader which displays cmask as color
//-------------------------------------------------------------------------------------------------------------
cbuffer OverlayParams : register(b0)
{
    uint SubTileLayoutMode;
};

Texture2D<int> g_texThumb : register(t0);

[RootSignature(FullScreenQuadRS)]
float4 PSOverlay(Interpolators In) : SV_TARGET0
{
    const float4 vRed = float4( 1.0f, 0.0f, 0.0f, 0.5f );
    const float4 vGreen = float4( 0.0f, 1.0f, 0.0f, 0.5f );

    uint2 ScreenPosition = uint2(In.Position.xy);

    // Cmask tile size is always 8x8;
    uint2 CmaskTileCoord = ScreenPosition >> 3;

    // Fetch Cmask value that is has 4 bits
    uint CmaskNibble = g_texThumb[CmaskTileCoord];

#if g_bSubTile

    // Compute coordinates of 2x2 quad within Cmask 8x8 tile
    uint2 QuadLocalCoord = uint2(
        __XB_UBFE(2u, 1u, ScreenPosition.x),
        __XB_UBFE(2u, 1u, ScreenPosition.y)
    );

    // Determine bit position indicating whether current sub-tile was touched or not based on sub-tile layout mode within Cmask tile
    uint SubTileBitPos = 0;

    if (SubTileLayoutMode == 0)
    {
        /** 2x2 mode. 4x4 pixel tiles correspond to the following bits:
         *      +---+---+
         *      | 0 | 1 |
         *      +---+---+
         *      | 2 | 3 |
         *      +---+---+
         */
        SubTileBitPos = (QuadLocalCoord.x >> 1u) | (QuadLocalCoord.y & 0x2);
    }
    else if (SubTileLayoutMode == 1)
    {
        /** 2x2 rotated mode (Xbox Series X|S only). 4x4 tiles correspond to the following bits:
         *      +---+---+
         *      | 0 | 2 |
         *      +---+---+
         *      | 1 | 3 |
         *      +---+---+
         */
        SubTileBitPos = (QuadLocalCoord.y >> 1u) | (QuadLocalCoord.x & 0x2);
    }
    else if (SubTileLayoutMode == 2)
    {
        /** 4x1 mode (Xboe One only). 2x8 tiles correspond to the following bits:
         *      +-+-+-+-+
         *      | | | | |
         *      |0|1|2|3|
         *      | | | | |
         *      +-+-+-+-+
         */
        SubTileBitPos = QuadLocalCoord.x;
    }
    else if (SubTileLayoutMode == 3)
    {
        /** 1x4 mode. 8x2 tiles correspond to the following bits:
         *      +---0---+
         *      +---1---+
         *      +---2---+
         *      +---3---+
         */
        SubTileBitPos = QuadLocalCoord.y;
    }
    else if (SubTileLayoutMode == 4)
    {
        /** 2x1 mode (Xbox One only, MSAA only). 4x8 tiles correspond to the following bits:
         *      +---+---+
         *      |   |   |
         *      | 0 | 1 |
         *      |   |   |
         *      +---+---+
         */
        SubTileBitPos = 2u + (QuadLocalCoord.x >> 1u);
    }
    else if (SubTileLayoutMode == 5)
    {
        /** 1x2 mode (MSAA only). 8x4 tiles correspond to the following bits:
         *      +-------+
         *      +   0   +
         *      +-------+
         *      +   1   +
         *      +-------+
         */
        SubTileBitPos = 2u + (QuadLocalCoord.y >> 1u);
    }
    return __XB_UBFE(1u, SubTileBitPos, CmaskNibble) != 0 ? vRed : vGreen;
#else
    /** layout modes 4 and 5 are only set when Msaa is enabled. That means only upper 2 bits out of 4 are valid */
    if (SubTileLayoutMode >= 4)
        return (CmaskNibble & 0xc) != 0 ? vRed : vGreen;
    else
        return (CmaskNibble & 0xf) != 0 ? vRed : vGreen;
#endif
}
