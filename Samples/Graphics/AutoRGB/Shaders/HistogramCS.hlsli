//--------------------------------------------------------------------------------------
// HistogramCS.hlsli
//
// Common shader elements for histogram compute shaders.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "../shared.h"

#define NUMTHREADS (NUM_BINS_PER_HISTOGRAM * NUMBER_OF_HISTOGRAMS)

// Intensity histogram buffer.
RWBuffer<uint> g_Intensities : register(u0);

// Group shared memory allocation for the "local" histogram build
// We are getting 4 histograms (one per component r, g, b plus luma)
groupshared uint LocalHistogram[ NUMBER_OF_HISTOGRAMS * NUM_BINS_PER_HISTOGRAM];

// Source framebuffer for histogram calculation.
Texture2D<float4> g_FrameBuffer : register(t0);

//--------------------------------------------------------------------------------------
// Name: Vertex
// Desc: Constant buffer for histogram calculation. Just stores viewport width and height.
//--------------------------------------------------------------------------------------
cbuffer Histogram
{
    uint2 Viewport;
};

//--------------------------------------------------------------------------------------
// Name: GetBin()
// Desc: Simple calculation to figure out which bin to place a value in.
//--------------------------------------------------------------------------------------
int GetBin(float i)
{
    return int( max( 0, min(NUM_BINS_PER_HISTOGRAM, i * NUM_BINS_PER_HISTOGRAM) ) );
}

// Experiment for color binning
uint3 GetRGBBinned(float3 color)
{
    uint3 clr = uint3(floor(color * 255.0f));
    uint valuesPerBin = 256 / NUM_BINS_PER_HISTOGRAM;
    uint3 binnedColor = clr / valuesPerBin;   
    return binnedColor;
}

// Experiment for color binning
uint3 GetYCCBinned(float3 color)
{
    float3x3 transform = float3x3(
        float3(0.299f, -0.168935f, 0.499813f),
        float3(0.587f, -0.331665f, -0.418531f),
        float3(0.114f, 0.50059f, -0.081282f));

    float3 yccColorFl = mul(floor(color * 255.0f), transform);

    uint YUint = (uint)yccColorFl.r;
    uint CbUint = (int)yccColorFl.g + 128;
    uint CrUint = (int)yccColorFl.b + 128;

    uint valuesPerBin = 256 / NUM_BINS_PER_HISTOGRAM;
    uint3 binnedColor = uint3(YUint, CbUint, CrUint) / valuesPerBin;

    return binnedColor;
}
