//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define __XBOX_ENABLE_WAVE32 1

#include "ExperimentalScattering.hlsli"

#define RootSig \
    "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
    "RootConstants(b0, num32bitconstants=5),"                                       \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT)"

cbuffer Input : register(b0)
{
    uint w;
    uint h;
    uint flags;
    float sunDiskX;
    float sunDiskY;
};

RWTexture2D<float3> Output : register(u0);

[RootSignature(RootSig)]
[numthreads(4, 8, 1)]
void main(uint2 GroupId : SV_GroupId, uint2 ThreadId : SV_GroupThreadId)
{
    static const float2 outputResolution = float2(w, h);
    static const float2 kSun = float2(sunDiskX, sunDiskY) * outputResolution;

    ATM_PROJECTION_MODE = ((flags >> 2) & 0x3) + 1u;
    ATM_TONEMAPPING_MODE = flags & 0x1;
    ATM_SHOW_TRANSMITTANCE = (flags >> 1) & 0x1;

#if USE_TWO_PIXELS_PER_THREAD
    uint2 PixelCoord = (GroupId << uint2(3, 3)) + ThreadId;
    #if USE_MANUAL_PIXEL_PACKING
        FpType4 result = callMainImage(FpTypeMax2(PixelCoord.xx + uint2(0, 4), PixelCoord.yy), outputResolution, kSun);
        float3 color0 = transpose(result)[0].rgb;
        float3 color1 = transpose(result)[1].rgb;
    #else
        float3 color0 = callMainImage(PixelCoord, outputResolution, kSun).rgb;
        float3 color1 = callMainImage(PixelCoord + uint2(4, 0), outputResolution, kSun).rgb;
    #endif
    Output[PixelCoord] = color0;
    Output[PixelCoord + uint2(4, 0)] = color1;
#else
    const uint2 PixelCoord = (GroupId << uint2(2, 3)) + ThreadId;
    Output[PixelCoord] = callMainImage(PixelCoord, outputResolution, kSun).rgb;
#endif
}
