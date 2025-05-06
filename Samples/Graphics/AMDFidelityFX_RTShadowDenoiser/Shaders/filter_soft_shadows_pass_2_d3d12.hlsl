/**********************************************************************
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved. 
RTShadowDenoiser - Filter Pass 2
********************************************************************/

#include "filter_soft_shadow_common_io.hlsli"

Texture2D<float16_t2>  InputBuffer  : register(t2);
StructuredBuffer<uint>  TileMetaData : register(t3);

RWTexture2D<float>   OutputBuffer    : register(u0);

float16_t2 FFX_DNSR_Shadows_ReadInput(int2 p)
{
    return (float16_t2)InputBuffer.Load(int3(p, 0)).xy;
}

uint FFX_DNSR_Shadows_ReadTileMetaData(uint p)
{
    return TileMetaData[p];
}
void FFX_DNSR_Shadows_StoreOutput(int2 p, float mean, float variance)
{
    // Recover some of the contrast lost during denoising
    const float shadow_remap = max(1.2f - mean * mean, 1.0f);
    mean = pow(abs(mean), shadow_remap);

    OutputBuffer[p] = mean;
}
 
#include "ffx_rtshadowdenoiser/ffx_denoiser_shadows_filter.h"

#define ComputeRS \
        "CBV(b0),"\
        "DescriptorTable (SRV(t0) ),"\
        "DescriptorTable (SRV(t1) ),"\
        "DescriptorTable (SRV(t2) ),"\
        "DescriptorTable (SRV(t3) ),"\
        "DescriptorTable (UAV(u0) ),"\
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
    const uint PASS_INDEX = 2;
    const uint STEP_SIZE = 4;
    FFX_DNSR_Shadows_FilterSoftShadowsPass(gid, gtid, did, PASS_INDEX, STEP_SIZE);
}
