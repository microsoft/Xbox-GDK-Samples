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
[RootSignature(HISTO_ROOT_SIGNATURE)]
void main(uint3 TID : SV_DispatchThreadID, uint3 GroupID : SV_GroupID, uint3 gThreadid : SV_GroupThreadID)
{
    // Zero out our values.
    if (gThreadid.x < NUM_BINS_PER_HISTOGRAM * NUMBER_OF_HISTOGRAMS)
    {
        LocalHistogram[gThreadid.x] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    // GroupID.x gives us the row, since number of groups is equivalent to the height of the image.
    // i gives us the column (x coord)
    // This for loop is going through a single column appears
    for (uint i = gThreadid.x; i < Viewport.x; i += NUMTHREADS)
    {
        uint2 location = uint2(i, GroupID.x);
        float4 color = g_FrameBuffer.Load(uint3(location, 0));

        uint3 binnedRGBValues = GetRGBBinned(color.xyz);
        uint3 binnedYCCValues = GetYCCBinned(color.xyz);

        // For each channel, update the histogram
        InterlockedAdd(LocalHistogram[HISTO_OFFSET_0 + binnedRGBValues.r], 1);
        InterlockedAdd(LocalHistogram[HISTO_OFFSET_1 + binnedRGBValues.g], 1);
        InterlockedAdd(LocalHistogram[HISTO_OFFSET_2 + binnedRGBValues.b], 1);
        InterlockedAdd(LocalHistogram[HISTO_OFFSET_3 + binnedYCCValues.x], 1);
    }

    // Now interlocked add to the SRV.
    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(g_Intensities[gThreadid.x], LocalHistogram[gThreadid.x]);
}
