/**********************************************************************
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
RTShadowDenoiser

prepare_shadow_mask - reads RT output and creates a packed buffer
of the results for use in the denoiser.

********************************************************************/

cbuffer PassData : register(b0)
{
    int2 BufferDimensions;
}

Texture2D<float> RaytracerResult : register(t0);
RWStructuredBuffer<uint> ShadowMask : register(u0);

int2 FFX_DNSR_Shadows_GetBufferDimensions()
{
    return BufferDimensions;
}

bool FFX_DNSR_Shadows_HitsLight(uint2 did)
{
    return RaytracerResult[did] > 0;
}

void FFX_DNSR_Shadows_WriteMask(uint offset, uint value)
{
    ShadowMask[offset] = value;
} 

#include "ffx_rtshadowdenoiser/ffx_denoiser_shadows_prepare.h"

#define ComputeRS \
        "CBV(b0),"\
        "DescriptorTable(SRV(t0)),"\
        "DescriptorTable(UAV(u0))"

[RootSignature(ComputeRS)]
[numthreads(8, 4, 1)]
void main(uint2 gtid : SV_GroupThreadID, uint2 gid : SV_GroupID)
{
    FFX_DNSR_Shadows_PrepareShadowMask(gtid, gid);
}
