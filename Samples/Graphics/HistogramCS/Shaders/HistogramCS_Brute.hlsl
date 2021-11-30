//--------------------------------------------------------------------------------------
// HistogramCS_Brute.hlsl
//
// DirectCompute shader for generating a histogram.
//
// With this method, each thread iterates over a scanline, and atomically adds
// histogram bucket results to the global histogram store.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HistogramCS.hlsli"

[numthreads(NUMTHREADS, 1, 1)]
RS_CS
void main(uint3 TID : SV_DispatchThreadID, uint3 GroupID : SV_GroupID, uint3 Thread : SV_GroupThreadID)
{
    int Previous = 0;
    for (uint i = 0; i < Viewport.x; i += NUMTHREADS)
    {
        uint2 location = uint2(i + Thread.x, GroupID.x);
        float4 value = g_FrameBuffer.Load(uint3(location, 0));
        float intensity = GetIntensity(value.xyz);
        int Bin = GetBin(intensity);
        InterlockedAdd(g_Intensities[Bin], 1);
    }
}
