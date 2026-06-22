//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define VRS_SUPPORT_PER_FRAME_CBR       0       // render alternate frames with 1x2 and 2x1, also in a checkerboard pattern per 8x8 tile, needs TAA to clean this up
#define D3D12_SHADING_RATE_CBR          0xff  
#define CBR_ADDITIONAL_TOLERANCE        1.4

#define SOBEL_KERNEL                    0       // actually ignores the row/column we're most interested in, good for wider features but can be troublesome for per pixel highlights
#define MINIMUM_KERNEL                  0       // shader is not VALU bound so not really worth the VALU savings
#define RECOMMENDED_KERNEL              1       // most expensive VALU but better at finding the details we're interested in as it includes a weight for all pixels, not just the surrounding pixels


#include "Global.hlsli"


#ifndef HALF_RES
# error
#endif


void WriteShadingRate(
    uint tileShadingRate,
    uint2 coord
    )
{
#if VRS_SUPPORT_PER_FRAME_CBR == 1
    if (D3D12_SHADING_RATE_CBR == tileShadingRate)
    {
        if (((coord.x ^ coord.y) & 1) ^ (frameIndex & 1))
            tileShadingRate = D3D12_SHADING_RATE_1X2;
        else
            tileShadingRate = D3D12_SHADING_RATE_2X1;
    }
#endif
    ShadingRateMapUAV[coord.xy] = tileShadingRate;
}


groupshared float g_Luma10x10[10][10];


#if MINIMUM_KERNEL == 1
# define MAX_OFFSETS	5
# define MAX_OPPOSITES	4

// arranged such that the index ^ 1 gives the opposite coordinate in the 3x3 grid
static const int2 offsets[MAX_OFFSETS] =
{
    int2(0, 1),
    int2(2, 1),
    int2(1, 0),
    int2(1, 2),
    int2(1, 1),		// not included in opposite
};
uint OppositeOffset(uint index) { return index ^ 1; }     // used to overwrite luma with straight line gradient when discounting sample due to depth, i.e. if we have samples A,B,C and C is to be discounted, set C luma to straight line extrapolation of A and B. OppositeOffset gets us A if we know C.
float CalcShadingRateX(float luma[MAX_OFFSETS]) { return abs(8.0 * luma[4] - 4.0 * (luma[0] + luma[1])); }
float CalcShadingRateY(float luma[MAX_OFFSETS]) { return abs(8.0 * luma[4] - 4.0 * (luma[2] + luma[3])); }
#elif SOBEL_KERNEL == 1
# define MAX_OFFSETS	8
# define MAX_OPPOSITES	8

// arranged such that the 7 - index gives the opposite coordinate in the 3x3 grid, i.e. index 1 == coord[1][0], opposite index is = 7 - 1 == 6 == coord[1][2]
static const int2 offsets[MAX_OFFSETS] =
{
    int2(0, 0),
    int2(1, 0),
    int2(2, 0),
    int2(0, 1),
    int2(2, 1),
    int2(0, 2),
    int2(1, 2),
    int2(2, 2),
};
uint OppositeOffset(uint index) { return 7 - index; }   // used to overwrite luma with straight line gradient when discounting sample due to depth, i.e. if we have samples A,B,C and C is to be discounted, set C luma to straight line extrapolation of A and B. OppositeOffset gets us A if we know C.
float CalcShadingRateX(float luma[MAX_OFFSETS])
{
    /*
         1  0 -1
         2  0 -2
         1  0 -1
    */
    return abs((luma[0] + (2.0 * luma[3]) + luma[5]) - (luma[2] + (2.0 * luma[4]) + luma[7]));
}
float CalcShadingRateY(float luma[MAX_OFFSETS])
{
    /*
         1  2  1
         0  0  0
        -1 -2 -1
    */
    return abs((luma[0] + (2.0 * luma[1]) + luma[2]) - (luma[5] + (2.0 * luma[6]) + luma[7]));
}
#elif RECOMMENDED_KERNEL == 1
# define MAX_OFFSETS	9
# define MAX_OPPOSITES	8

// arranged such that the 7 - index gives the opposite coordinate in the 3x3 grid, i.e. index 1 == coord[1][0], opposite index is = 7 - 1 == 6 == coord[1][2]
static const int2 offsets[MAX_OFFSETS] =
{
    int2(0, 0),
    int2(1, 0),
    int2(2, 0),
    int2(0, 1),
    int2(2, 1),
    int2(0, 2),
    int2(1, 2),
    int2(2, 2),
    int2(1, 1),		// not included in opposite
};
uint OppositeOffset(uint index) { return 7 - index; }   // used to overwrite luma with straight line gradient when discounting sample due to depth, i.e. if we have samples A,B,C and C is to be discounted, set C luma to straight line extrapolation of A and B. OppositeOffset gets us A if we know C.

float CalcShadingRateX(float luma[MAX_OFFSETS])
{
    /*
        -1  2 -1
        -4  8 -4
        -1  2 -1
    */
    return abs(2.0 * (luma[1] + luma[6] + 4.0 * luma[8]) - (luma[0] + luma[2] + luma[5] + luma[7] + 4.0 * (luma[3] + luma[4])));
}

float CalcShadingRateY(float luma[MAX_OFFSETS])
{
    /*
        -1 -4 -1
         2  8  2
        -1 -4 -1
    */
    return abs(2.0 * (luma[3] + luma[4] + 4.0 * luma[8]) - (luma[0] + luma[2] + luma[5] + luma[7] + 4.0 * (luma[1] + luma[6])));
}
#else
# error "undefined kernel"
#endif


#if HALF_RES == 1
float2 IndexToFullResUV(int2 index) { return (float2(index * 2) + 0.5) * invInputDim; }      // *2 because we have only launched half the thread groups in half res mode, +0.5 gets us the half way point
#endif


float BuildLuma(float3 colour)
{
    return dot(float3(0.299, 0.587, 0.114), colour);
}



uint CheckForCBR(float xRate, float yRate)
{
#if VRS_SUPPORT_PER_FRAME_CBR == 1
    float CBRsobelTolerance = SobelTolerance * CBR_ADDITIONAL_TOLERANCE;
    bool highRateX = xRate > CBRsobelTolerance;
    bool highRateY = yRate > CBRsobelTolerance;
    uint countMax = WaveActiveCountBits(highRateX && highRateY);

    return (countMax == 0) ? D3D12_SHADING_RATE_CBR : D3D12_SHADING_RATE_1X1;
#else
    return D3D12_SHADING_RATE_1X1;
#endif
}



uint CalcTileShadingRate(float xRate, float yRate)
{
    bool highRateX = xRate > SobelTolerance;
    bool highRateY = yRate > SobelTolerance;
    uint countMax = WaveActiveCountBits(highRateX && highRateY);

    if (countMax > 0)// discardSampleCountMaxRate)
    {
        return CheckForCBR(xRate, yRate);
    }
    uint count1x2 = WaveActiveCountBits(highRateX);
    uint count2x1 = WaveActiveCountBits(highRateY);

    if ((count1x2 | count2x1) > 0)// discardSampleCountHalfRate)
    {
        return (count1x2 > count2x1) ? D3D12_SHADING_RATE_1X2 : D3D12_SHADING_RATE_2X1;
    }
    return D3D12_SHADING_RATE_2X2;
}



uint CalcInitialTileShadingRate(float xRate, float yRate, inout uint count1x2, inout uint count2x1)
{
    bool highRateX = xRate > SobelTolerance;
    bool highRateY = yRate > SobelTolerance;
    uint countMax = WaveActiveCountBits(highRateX && highRateY);

    if (countMax > 0)// discardSampleCountMaxRate)
    {
        return CheckForCBR(xRate, yRate);
    }
    count1x2 = WaveActiveCountBits(highRateX);
    count2x1 = WaveActiveCountBits(highRateY);

    return D3D12_SHADING_RATE_2X2;
}



uint CalcFinalTileShadingRate(float xRate, float yRate, inout uint count1x2, inout uint count2x1)
{
    bool highRateX = xRate > SobelTolerance;
    bool highRateY = yRate > SobelTolerance;
    uint countMax = WaveActiveCountBits(highRateX && highRateY);

    if (countMax > 0)// discardSampleCountMaxRate)
    {
        return CheckForCBR(xRate, yRate);
    }
    count1x2 += WaveActiveCountBits(highRateX);
    count2x1 += WaveActiveCountBits(highRateY);

    if ((count1x2 | count2x1) > 0)// discardSampleCountHalfRate)
    {
        return (count1x2 > count2x1) ? D3D12_SHADING_RATE_1X2 : D3D12_SHADING_RATE_2X1;
    }
    return D3D12_SHADING_RATE_2X2;
}



uint GetTileShadingRate(float luma[MAX_OFFSETS])
{
    return CalcTileShadingRate(CalcShadingRateX(luma), CalcShadingRateY(luma));
}



uint GetInitialTileShadingRate(float luma[MAX_OFFSETS], inout uint count1x2, inout uint count2x1)
{
    return CalcInitialTileShadingRate(CalcShadingRateX(luma), CalcShadingRateY(luma), count1x2, count2x1);
}



uint GetFinalTileShadingRate(float luma[MAX_OFFSETS], inout uint count1x2, inout uint count2x1)
{
    return CalcFinalTileShadingRate(CalcShadingRateX(luma), CalcShadingRateY(luma), count1x2, count2x1);
}



#if __XBOX_ENABLE_WAVE32 == 1
#if HALF_RES == 1

// half res wave32, group size 8x4
// use tex unit to average each 2x2
// deriving shading rate for each 4x4 threads representing a 8x8 tile each, write 2x results
void BuildShadingRate(uint2 tileId, uint2 threadId)     
{
    uint threadIdLinear = threadId.x + threadId.y * 8;

    // load first 10x6
    if (threadIdLinear < 30)    // first 30 threads makes 2x loads to populate 10x6 luma
    {
        uint x = threadIdLinear / 3;            // compiler does well with this even on platforms with no integer divide
        uint y = threadIdLinear - (x * 3);

        uint2 ldsId = uint2(x, y * 2);
        uint2 loadId = uint2(tileId.x * 8, tileId.y * 4) + ldsId;

        // loads would go out of bounds except we are not calculating the shading rate for the safe area
        // use texture unit to average each 2x2        
        g_Luma10x10[ldsId.x][ldsId.y    ] = BuildLuma(RenderedImageVRSSRV.SampleLevel(LinearClampSampler, IndexToFullResUV(int2(loadId.x - 1, loadId.y - 1)), 0).xyz);
        g_Luma10x10[ldsId.x][ldsId.y + 1] = BuildLuma(RenderedImageVRSSRV.SampleLevel(LinearClampSampler, IndexToFullResUV(int2(loadId.x - 1, loadId.y    )), 0).xyz);
    }
    // calculate shading rate for first 8x4
    float luma[MAX_OFFSETS];
    int i;

    [unroll]
    for (i = 0; i < MAX_OFFSETS; ++i)
    {
        uint2 coord = threadId + offsets[i];
        luma[i] = g_Luma10x10[coord.x][coord.y];
    }
    uint tileShadingRate = 0;
    for (i = 0; i < 2; ++i)
    {
        uint2 threadMin = int2((i & 1) * 4, 0);     // {0,0}, {4,0}
        uint2 threadMax = threadMin + 4;            // {4,04, {8,4}

        if (all(threadId >= threadMin) && all(threadId < threadMax))
        {
            tileShadingRate = GetTileShadingRate(luma);
        }
    }
    if (all(threadId & 3) == 0)   // one thread per 4x4
    {
        // 2 threads writing to a 2x1
        WriteShadingRate(tileShadingRate, uint2((tileId.x << 1) + (threadId.x >> 2), tileId.y));
    }
}

#else

// full res wave32, group size 8x4
// pull in 10x6 to LDS, calculating shading rate for first half tile, early out if full rate
// pull in 10x4 into LDS, calculating shading rate for second half tile, write one result
void BuildShadingRate(uint2 tileId, uint2 threadId)   
{
    uint threadIdLinear = threadId.x + threadId.y * 8;

    // load first 10x6
    if (threadIdLinear < 30)    // first 30 threads makes 2x loads to populate first 10x6 luma
    {
        uint x = threadIdLinear / 3;            // compiler does well with this even on platforms with no integer divide
        uint y = threadIdLinear - (x * 3);

        uint2 ldsId = uint2(x, y * 2);
        uint2 loadId = (tileId.xy * 8) + ldsId;

        // loads would go out of bounds except we are not calculating the shading rate for the safe area
        g_Luma10x10[ldsId.x][ldsId.y    ] = BuildLuma(RenderedImageVRSSRV[int2(loadId.x - 1, loadId.y - 1)].xyz);
        g_Luma10x10[ldsId.x][ldsId.y + 1] = BuildLuma(RenderedImageVRSSRV[int2(loadId.x - 1, loadId.y    )].xyz);
    }
    // calculate shading rate for first 8x4
    float luma[MAX_OFFSETS];
    int i;

    [unroll]
    for (i = 0; i < MAX_OFFSETS; ++i)
    {
        uint2 coord = threadId + offsets[i];
        luma[i] = g_Luma10x10[coord.x][coord.y];
    }
    uint count1x2 = 0;
    uint count2x1 = 0;
    uint tileShadingRate = GetInitialTileShadingRate(luma, count1x2, count2x1);
    if (D3D12_SHADING_RATE_1X1 == tileShadingRate)
    {
        if (all(threadId == 0))
        {
            WriteShadingRate(tileShadingRate, tileId);
        }
        return;     // yay!
    }
    // load second 10x4
    if (threadIdLinear < 20)    // first 20 threads makes 2x loads to populate remaining 10x4 luma
    {
        uint x = threadIdLinear / 2;            // compiler does well with this even on platforms with no integer divide
        uint y = threadIdLinear - (x * 2);

        uint2 ldsId = uint2(x, y * 2 + 6);      // already done six rows
        uint2 loadId = (tileId.xy * 8) + ldsId;

        // loads would go out of bounds except we are not calculating the shading rate for the safe area
        g_Luma10x10[ldsId.x][ldsId.y    ] = BuildLuma(RenderedImageVRSSRV[int2(loadId.x - 1, loadId.y - 1)].xyz);
        g_Luma10x10[ldsId.x][ldsId.y + 1] = BuildLuma(RenderedImageVRSSRV[int2(loadId.x - 1, loadId.y    )].xyz);
    }
    // calculate shading rate for second 8x4
    uint2 baseCoord = threadId;
    baseCoord.y += 4;

    [unroll]
    for (i = 0; i < MAX_OFFSETS; ++i)
    {
        uint2 coord = baseCoord + offsets[i];
        luma[i] = g_Luma10x10[coord.x][coord.y];
    }
    tileShadingRate = GetFinalTileShadingRate(luma, count1x2, count2x1);
    if (all(threadId == 0))
    {
        WriteShadingRate(tileShadingRate, tileId);
    }
}

#endif
#else // WAVE64
#if HALF_RES == 1

// half res wave64, group size 8x8
// use tex unit to average each 2x2
// deriving shading rate for each 4x4 threads representing a 8x8 tile each, write 4x results
void BuildShadingRate(uint2 tileId, uint2 threadId)
{
    uint threadIdLinear = threadId.x + threadId.y * 8;

    if (threadIdLinear < 50)    // first 50 threads makes 2x loads to populate 10x10 luma
    {
        uint x = threadIdLinear / 5;            // compiler does well with this even on platforms with no integer divide
        uint y = threadIdLinear - (x * 5);

        uint2 ldsId = uint2(x, y * 2);
        uint2 loadId = (tileId.xy * 8) + ldsId;

        // loads would go out of bounds except we are not calculating the shading rate for the safe area
        // use texture unit to average each 2x2        
        g_Luma10x10[ldsId.x][ldsId.y    ] = BuildLuma(RenderedImageVRSSRV.SampleLevel(LinearClampSampler, IndexToFullResUV(int2(loadId.x - 1, loadId.y - 1)), 0).xyz);
        g_Luma10x10[ldsId.x][ldsId.y + 1] = BuildLuma(RenderedImageVRSSRV.SampleLevel(LinearClampSampler, IndexToFullResUV(int2(loadId.x - 1, loadId.y    )), 0).xyz);
    }
    float luma[MAX_OFFSETS];
    int i;

    [unroll]
    for (i = 0; i < MAX_OFFSETS; ++i)
    {
        uint2 coord = threadId + offsets[i];
        luma[i] = g_Luma10x10[coord.x][coord.y];
    }
    uint tileShadingRate = 0;
    for (i = 0; i < 4; ++i)
    {
        uint2 threadMin = int2((i & 1) * 4, (i & 2) * 2);   // {0,0}, {4,0}, {0,4}, {4,4}
        uint2 threadMax = threadMin + 4;                    // {4,4}, {8,4}, {4,8}, {8,8}

        if (all(threadId >= threadMin) && all(threadId < threadMax))
        {
            tileShadingRate = GetTileShadingRate(luma);
        }
    }
    if (all(threadId & 3) == 0)   // one thread per 4x4
    {
        WriteShadingRate(tileShadingRate, (tileId << 1) + (threadId.xy >> 2));
    }
}

#else

// full res wave64, group size 8x8
// possibly the simplest case
void BuildShadingRate(uint2 tileId, uint2 threadId)
{
    uint threadIdLinear = threadId.x + threadId.y * 8;

    if (threadIdLinear < 50)    // first 50 threads makes 2x loads to populate 10x10 luma
    {
        uint x = threadIdLinear / 5;            // compiler does well with this even on platforms with no integer divide
        uint y = threadIdLinear - (x * 5);

        uint2 ldsId = uint2(x, y * 2);
        uint2 loadId = (tileId.xy * 8) + ldsId;

        // loads would go out of bounds except we are not calculating the shading rate for the safe area
		g_Luma10x10[ldsId.x][ldsId.y    ] = BuildLuma(RenderedImageVRSSRV[int2(loadId.x - 1, loadId.y - 1)].xyz);
		g_Luma10x10[ldsId.x][ldsId.y + 1] = BuildLuma(RenderedImageVRSSRV[int2(loadId.x - 1, loadId.y    )].xyz);
    }
	float luma[MAX_OFFSETS];
    int i;

	[unroll]
	for (i = 0; i < MAX_OFFSETS; ++i)
	{
		uint2 coord = threadId + offsets[i];
		luma[i] = g_Luma10x10[coord.x][coord.y];
	}
    uint tileShadingRate = GetTileShadingRate(luma);
    if (all(threadId == 0)) 
    {
        WriteShadingRate(tileShadingRate, tileId);
    }
}

#endif
#endif



[RootSignature(GlobalRS)]
#if __XBOX_ENABLE_WAVE32 == 1
[numthreads(8, 4, 1)]
#else
[numthreads(8, 8, 1)]
#endif
void main(uint2 groupThreadId : SV_GroupThreadId, uint2 groupId : SV_GroupID)
{
    uint2 tileId = groupId;
/*
    Shading rate all the way round the perimeter is always min and so we don't dispatch groups for the 4x edges, edges are 16 pixels wide
    this also prevents the shader having to care about threads which be processing pixels off the edge of the SRV, which are in the TV safe area anyway /
    think of leaving the border at 2x2 always as foveated rendering and a nice speed up ;)

    If we did care about the border, reading off the edge of the SRV is going to return 0, which is not necessarily a great number for the edge detection
    and we may boost the shading rate when we don't want to simply because of edge pixels having zero luminance in the matrix.

    Solutions include:
    1. Turning off exec mask for edge pixels, this is probably the ideal but involves extra VALU
    2. Using clamp sampling, although sample can be slower than a direct load (the half res modes are using this sampler already to filter 4x pixels)
*/
#if HALF_RES == 1
    tileId += uint2(1, 1);
#else
    tileId += uint2(2, 2);
#endif
    BuildShadingRate(tileId, groupThreadId);
}
