//--------------------------------------------------------------------------------------
// DecompressDepthStencil.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define DepthStencilDecompressRS \
    "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(SRV(t0, numDescriptors=3), visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(UAV(u0, numDescriptors=2), visibility=SHADER_VISIBILITY_ALL)"

#include "DepthDecompressUtility.hlsli"

#ifndef ENABLE_STENCIL_DECOMPRESS
#define ENABLE_STENCIL_DECOMPRESS 1
#endif

cbuffer HTileParams : register(b0)
{
    uint HTileInfo;
};

ByteAddressBuffer   HTileBuffer     : register(t0);
Texture2D<float>    ZCompressed     : register(t1);
Texture2D<uint>     SCompressed     : register(t2);

RWTexture2D<float>  ZDecompressed   : register(u0);
RWTexture2D<uint>   SDecompressed   : register(u1);

[RootSignature(DepthStencilDecompressRS)]
[numthreads(8, 8, 1)]
void DepthStencilDecompress(uint2 GroupId : SV_GroupId, uint2 ThreadId : SV_GroupThreadId, uint ThreadIndex : SV_GroupIndex)
{
    const uint2 PixelLocalCoord = SwizzleLinearIndex(ThreadIndex & 63);
    const uint2 ZMaskAndStencilState = GetZMaskAndStencilState(HTileBuffer, HTileInfo, GroupId);

    const uint2 PixelCoord = (GroupId << 3) | PixelLocalCoord;

    ZDecompressed[PixelCoord] = DecompressDepth(ZCompressed, ZMaskAndStencilState.x, PixelCoord, ThreadIndex);

#if ENABLE_STENCIL_DECOMPRESS
    SDecompressed[PixelCoord] = DecompressStencil(SCompressed, ZMaskAndStencilState.y, PixelCoord, ThreadIndex);
#endif
}