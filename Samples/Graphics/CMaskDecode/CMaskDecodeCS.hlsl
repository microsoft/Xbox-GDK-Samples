//--------------------------------------------------------------------------------------
// CMaskDecodeCS.hlsl
//
// DirectCompute shader which decodes fast clear information from cmask.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ColorDecompressUtility.hlsli"

#define CMaskDecodeRS \
    "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)"

//-------------------------------------------------------------------------------------------------------------
cbuffer CmaskParams : register(b0)
{
    uint CMaskInfo;
};

//-------------------------------------------------------------------------------------------------------------
// Name: CSCmaskDecode()
// Desc: Compute shader which decodes fast clear information from cmask.
//
//  Performance of this shader is not too critical, as the cmask surface is small. 
//-------------------------------------------------------------------------------------------------------------
ByteAddressBuffer g_bufCmask    : register(t0);
RWTexture2D<uint> g_texCmask    : register(u0);

[RootSignature(CMaskDecodeRS)]
[numthreads(8,8,1)]
void CSCmaskDecode( uint3 id : SV_DispatchThreadID )
{
    // Every pixel of the output texture corresponds to 8x8 tile of pixels of the texture for which given CMask is created
    // After unpacking, 4-bit CMask value is stored to appropriate pixel corresponding to 8x8 tile of the original texture
    g_texCmask[ id.xy ] = GetCMask(g_bufCmask, id.xy, CMaskInfo);
}
