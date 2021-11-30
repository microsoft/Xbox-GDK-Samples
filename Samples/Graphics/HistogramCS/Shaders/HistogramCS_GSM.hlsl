//--------------------------------------------------------------------------------------
// HistogramCS_GSM.hlsl
//
// DirectCompute shader for generating a histogram.
//
// With this method, each thread group takes an entire scanline,  and bins the intensities
// into group shared memory (GSM). At the end of the scanline, we take the result and
// atomically add it to the global histogram.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HistogramCS.hlsli"

[numthreads(NUMTHREADS, 1, 1)]
RS_CS
void main(uint3 TID : SV_DispatchThreadID, uint3 GroupID : SV_GroupID, uint3 Thread : SV_GroupThreadID)
{
    // Zero out our values.
    LocalHistogram[Thread.x] = 0;
    GroupMemoryBarrierWithGroupSync();

    // GroupID.x gives us the row.
    // i gives us the column (pixel)
    for (uint i = 0; i < Viewport.x; i += NUMTHREADS)
    {
        uint2 location = uint2(i + Thread.x, GroupID.x);
        float4 value = g_FrameBuffer.Load(uint3(location, 0));
        float intensity = GetIntensity(value.xyz);

        int Bin = (int)GetBin(intensity);

        InterlockedAdd(LocalHistogram[Bin], 1);
    }

    // Now interlocked add to the SRV.
    GroupMemoryBarrierWithGroupSync();
    InterlockedAdd(g_Intensities[Thread.x], LocalHistogram[Thread.x]);
}
