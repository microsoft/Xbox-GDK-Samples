//--------------------------------------------------------------------------------------
// HistogramCS.hlsli
//
// Common shader elements for histogram compute shaders.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define NUMBINS 64
#define NUMTHREADS 64

#define RS_CS \
[\
    RootSignature\
    (\
       "CBV(b0, visibility=SHADER_VISIBILITY_ALL),\
        DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
        DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
        StaticSampler(\
            s0,\
            addressU = TEXTURE_ADDRESS_WRAP,\
            addressV = TEXTURE_ADDRESS_WRAP,\
            addressW = TEXTURE_ADDRESS_WRAP,\
            comparisonFunc = COMPARISON_ALWAYS,\
            borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,\
            filter = FILTER_MIN_MAG_MIP_LINEAR,\
            visibility = SHADER_VISIBILITY_ALL)"\
    )\
]

// Intensity histogram buffer.
RWBuffer< uint > g_Intensities : register(u0);
// Group shared memory allocation for the "local" histogram build
groupshared uint LocalHistogram[ NUMBINS ];

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
int GetBin( float i )
{
    return int( max( 0, min( NUMBINS, i * NUMBINS ) ) );
}

//--------------------------------------------------------------------------------------
// Name: GetIntensity()
// Desc: Simple calculation to figure out the intensity from grayscale, using the HDTV 
//       Luma Value (http://en.wikipedia.org/wiki/Grayscale)
//--------------------------------------------------------------------------------------
float GetIntensity( float3 In )
{
    return 0.2126 * In.x + 0.7152 * In.y + 0.0722 * In.z;
}
