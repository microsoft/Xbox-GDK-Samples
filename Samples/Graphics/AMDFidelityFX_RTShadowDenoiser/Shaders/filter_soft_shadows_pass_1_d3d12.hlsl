/**********************************************************************
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
RTShadowDenoiser - Filter Pass 1
********************************************************************/

#include "filter_soft_shadow_common_io.hlsli"

Texture2D<float16_t>   HistoryBuffer : register(t2);
Texture2D<float16_t>   IntermediateFilterResult  : register(t3);
StructuredBuffer<uint>  TileMetaData : register(t4);

RWTexture2D<float2> OutputBuffer : register(u0);

uint FFX_DNSR_Shadows_ReadTileMetaData(uint p)
{
    return TileMetaData[p];
}

float16_t2 FFX_DNSR_Shadows_ReadInput(int2 p)
{
    return float16_t2(
        HistoryBuffer.Load(int3(p, 0)).x,
        IntermediateFilterResult.Load(int3(p, 0)).x
        );
}

void FFX_DNSR_Shadows_StoreOutput(int2 p, float mean, float variance)
{
    OutputBuffer[p] = float2(mean, variance);
}
 
#include "ffx_rtshadowdenoiser/ffx_denoiser_shadows_filter.h"

#define ComputeRS \
        "CBV(b0),"\
        "DescriptorTable (SRV(t0) ),"\
        "DescriptorTable (SRV(t1) ),"\
        "DescriptorTable (SRV(t2) ),"\
        "DescriptorTable (SRV(t3) ),"\
        "DescriptorTable (SRV(t4) ),"\
        "DescriptorTable (UAV(u0) ),"\
        "DescriptorTable (UAV(u1) ),"\
        "StaticSampler(s0,"\
                "filter = FILTER_MIN_MAG_MIP_LINEAR,"\
                "addressU = TEXTURE_ADDRESS_CLAMP,"\
                "addressV = TEXTURE_ADDRESS_CLAMP,"\
                "addressW = TEXTURE_ADDRESS_CLAMP,"\
                "mipLODBias = 0.f,"\
                "maxAnisotropy = 1,"\
                "comparisonFunc = COMPARISON_NEVER,"\
                "minLOD = 0.f,"\
                "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,"\
                "maxLOD = 3.402823466e+38f,"\
                "space = 0,"\
                "visibility = SHADER_VISIBILITY_ALL )"

[RootSignature(ComputeRS)]
[numthreads(8, 8, 1)]
void main(uint2 gid : SV_GroupID, uint2 gtid : SV_GroupThreadID, uint2 did : SV_DispatchThreadID)
{
    const uint PASS_INDEX = 1;
    const uint STEP_SIZE = 2;
    FFX_DNSR_Shadows_FilterSoftShadowsPass(gid, gtid, did, PASS_INDEX, STEP_SIZE);
}
