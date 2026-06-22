//--------------------------------------------------------------------------------------
// FMask.hlsli
//
// Routines for interpreting fmask
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FMaskConstants.hlsli"

//-------------------------------------------------------------------------------------------------------------
// Name: ManualLoad()
// Desc: Load the nth sample color, by looking up the index in the FMask, and then looking up that fragment
// in the color buffer.  
//
// Set unknown if the sample has the index which codes "unknown".  In this case, return some valid color 
// for the pixel.
//
// Note this is not the most efficient way to iterate through all samples, as you may end up fetching the
// same fragment color more than once.
//-------------------------------------------------------------------------------------------------------------
float4 ManualLoad(uniform Texture2DMS<float4> texColorMS, uniform Texture2D<uint2> texFMask, uint2 texcoord, uniform uint sample, out bool unknown)
{
    uint2 fmaskXY = texFMask.Load(uint3(texcoord, 0));
    uint shift = sample * INDEX_BITS;
    uint frag = (shift < 32)
        ? ((fmaskXY.x >> shift) & INDEX_MASK)
        : ((fmaskXY.y >> (shift - 32)) & INDEX_MASK);

    unknown = (frag & INVALID_INDEX_MASK);
    return texColorMS.Load(texcoord, frag & VALID_INDEX_MASK);
}

// fragmentWeight records the number of samples which hit each fragment (if any).
#ifdef UBERSHADER
groupshared uint fragmentWeight[MAX_NUM_FRAGMENTS][64];
#else
groupshared uint fragmentWeight[NUM_FRAGMENTS][64];
#endif

//-------------------------------------------------------------------------------------------------------------
// Name: ManualResolve()
// Desc: Returns the average of the fragment colors, weighted by multiplicity.  Ignore unknown samples.
// Take care not to fetch the same sample repeatedly.
//-------------------------------------------------------------------------------------------------------------
float4 ManualResolve(uniform Texture2DMS<float4> texColorMS, uniform Texture2D<uint2> texFMask, uint2 texcoord)
{
    uint laneID = WaveGetLaneIndex();

#ifndef UBERSHADER
	[unroll]
#endif
	for (uint i = 0; i < NUM_FRAGMENTS; ++i)
    {
        fragmentWeight[i][laneID] = 0;
    }

    // Read the fmask, and decode the indices for each sample.
    uint2 fmaskXY = texFMask.Load(uint3(texcoord, 0));
    uint fmask = fmaskXY.x;
#ifndef UBERSHADER
    [unroll] // this breaks the ubershader
#endif
    for (uint sample = 0; sample < NUM_SAMPLES; ++sample)
    {
        uint frag = fmask & INDEX_MASK;
        fmask >>= INDEX_BITS;

        [flatten]
        if ((sample + 1) * INDEX_BITS == 32)    // used 32 bits ... move to the next component
        {
            fmask = fmaskXY.y;
        }

        // If "unknown" code, skip the sample.  An alternative would be to use a nearby 
        // known sample.  GPU guarantees that sample & ( NUM_FRAGMENTS - 1 ) is such a sample.
        [flatten]
        if (frag < NUM_FRAGMENTS)
        {
            InterlockedAdd(fragmentWeight[frag][laneID], 1);
        }
    }

    // Fetch any fragment colors which have non-zero weight, and compute the weighted average.
    // This method performs the minimum number of fetches, but has bad latency-hiding.
    uint totalWeight = 0;
    float4 resolvedColor = 0;
#ifndef UBERSHADER
	[unroll]
#endif
	for (uint frag = 0; frag < NUM_FRAGMENTS; ++frag)
    {
        uint weight = fragmentWeight[frag][laneID];

        [branch]
        if (weight > 0)
        {
            resolvedColor += weight * texColorMS.Load(texcoord, frag);
            totalWeight += weight;
        }
    }
    resolvedColor /= totalWeight;

    return resolvedColor;
}
