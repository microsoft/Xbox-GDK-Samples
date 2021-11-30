//--------------------------------------------------------------------------------------
// HistogramCS_GSM_U.hlsl
//
// DirectCompute shader for generating a histogram.
//
// Build a histogram in group shared meory (GSM) for a scanline, and then atomically add the results
// to the global object. However, this function does the operation incoherently, which results
// in suboptimal memory access patterns. In real life, we would use the coalesced version,
// which is faster.
//
// This version is only here to illustrate the importance of coalescing memory access in compute.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HistogramCS.hlsli"

[numthreads(NUMTHREADS, 1, 1)]
RS_CS
void main(uint3 TID : SV_DispatchThreadID, uint3 GroupID : SV_GroupID, uint3 Thread : SV_GroupThreadID)
{
    int Count = Viewport.x / NUMTHREADS;
    int Base = Thread.x * Count;

    // Zero out our values.
    LocalHistogram[Thread.x] = 0;
    GroupMemoryBarrierWithGroupSync();

    for (int i = 0; i < Count; ++i)
    {
        uint2 location = uint2(Base + i, GroupID.x);
        float4 value = g_FrameBuffer.Load(uint3(location, 0));
        float intensity = GetIntensity(value.xyz);
        int Bin = GetBin(intensity);
        InterlockedAdd(LocalHistogram[Bin], 1);
    }

    // Now interlocked add to the SRV.
    GroupMemoryBarrierWithGroupSync();
    InterlockedAdd(g_Intensities[Thread.x], LocalHistogram[Thread.x]);
}
