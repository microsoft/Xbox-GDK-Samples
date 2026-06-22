//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Global.hlsli"
#include "ComputeShaderQuads.hlsli"

//--------------------------------------------------------------------------------------
// Settings
//--------------------------------------------------------------------------------------

// Dynamic Resolution Scaling supported? Meaning previous and current frames may be different resolutions
#ifndef DRS_SUPPORTED
# define DRS_SUPPORTED                          0
#endif

// Compute shading rate for whole target? Otherwise ignore safe area
#ifndef COMPUTE_SAFE_AREA
# define COMPUTE_SAFE_AREA                      0
#endif

// Check for depth discontinuties?
// The shader only checks for depth discontinuities for 1x1 rate tiles, or a tile with a high rate axis
// (see CHECK_DEPTH_FOR_ANY_HIGH_RATE), which is a major optimisation for a stand alone shader.
// The optimisations is not loading depth data for low rate tiles.
// 
// It is also possibly a real title might combine shading rate generation with a shader which is reading
// depth anyway, in which case you might want to do the depth check for all tiles.
#ifndef CHECK_DEPTH_DISCONTINUINTY
# define CHECK_DEPTH_DISCONTINUINTY             0
#endif

// Set to 1 to check depth for 2x1 and 1x2 rate tiles
// Set to 0 to only check depth for 1x1 rate tiles
#ifndef CHECK_DEPTH_FOR_ANY_HIGH_RATE
# define CHECK_DEPTH_FOR_ANY_HIGH_RATE          1
#endif

// Depth delta, anything larger than this value is considered a discontinuity in depth
#ifndef DEPTH_DISCONTINUINTY_TOLERANCE
# define DEPTH_DISCONTINUINTY_TOLERANCE         0.001
#endif

// Kernel choices
// Preserve single pixel artefacts adds the absolute luminance deltas, so if you have a single pixel
// spike or peak value, this will in effect gain double weighting vs min/max approach
//
// Favour temporal stability use min3/max3 of the lumiance value and its neighbours, this slightly
// improves the temporal stability of shading rate choice, but places less emphasis on single pixel
// peak values
#define KERNEL_PRESERVE_SINGLE_PIXEL_ARTEFACTS  0
#define KERNEL_FAVOUR_TEMPORAL_STABILITY        1

#ifndef KERNEL
# define KERNEL                                 KERNEL_PRESERVE_SINGLE_PIXEL_ARTEFACTS
#endif

// Graph to apply to luminance values
#define LUMA_CURVE_NONE                         0   // i.e. linear
#define LUMA_CURVE_SQRT                         1   // 
#define LUMA_CURVE_REINHARD_2X                  2   // recommended

#ifndef LUMA_CURVE
# define LUMA_CURVE                             LUMA_CURVE_REINHARD_2X
#endif

// Set this if you support 2x2 tile size for deferred variable rate compute shaders
// +3-4 us slower to support this even if its then disabled, cost of ICache and branches
#ifndef SUPPORT_2x2_TILE_SIZE
# define SUPPORT_2x2_TILE_SIZE                  1
#endif

// Number of pixels asking for a higher rate must be higher than this
// Increasing this can reduce shading rate, but it can also add more to temporal instability
// Note: two adjacent pixels will both ask for a high rate, so this number should be a multiple of two
// Note: this is sensitive to wave size, since this a count over the entire wave, not the entire tile
#define IGNORE_HIGH_RATE_PIXEL_COUNT            0

// Only turn this on for debugging the multiple wave code path on Xbox, not for production code!
// Anaconda @ 2160p with depth testing: wave32 = 163us, wave64 = 107us
// Anaconda @ 2160p with no depth testing: wave32 = 150us, wave64 = 107us
#define XBOX_DEBUG_TEST_MULTIPLE_WAVES          0




//--------------------------------------------------------------------------------------
// Macros below here are derived from the settings
//--------------------------------------------------------------------------------------
#define WAVE_SIZE_UNKNOWN                       0

#if defined __XBOX_ONE || defined __XBOX_SCARLETT
# if XBOX_DEBUG_TEST_MULTIPLE_WAVES != 0
#  define __XBOX_ENABLE_WAVE32
#  define WAVE_SIZE                             32
# else
#  ifdef __XBOX_ENABLE_WAVE32
#   error "Shader requires wave64!"
#  endif
#  define WAVE_SIZE                             64
# endif
# define TILE_SIZE                              8       // always 8 on Scarlett
#else
# ifndef TILE_SIZE
#  error "TILE_SIZE must be queried from the device and a shader with that size used, see D3D12_FEATURE_DATA_D3D12_OPTIONS6::ShadingRateImageTileSize"
# endif
# define WAVE_SIZE                              WAVE_SIZE_UNKNOWN
# define __XB_MadU24(a, b, c)                   (((a) * (b)) + (c))
#endif

// do we have multiple waves per tile? Alot slower if we do
#if WAVE_SIZE == TILE_SIZE * TILE_SIZE
# define SINGLE_WAVE_PER_TILE                   1
#else
# define SINGLE_WAVE_PER_TILE                   0
#endif

// It is more expensive to use a sampler, though costs may be hidden behind bandwidth/memory fetch if
// this shader is not combined with another pass. Using a sample with clamp means we don't need to
// specially handle about threads which are reading off the render target
#define USE_SAMPLER                             (DRS_SUPPORTED | COMPUTE_SAFE_AREA)

// FIXME if you have DRS 
#if DRS_SUPPORTED != 0
# error "Supply prev frame values"
#else
# define prevFrameInvRenderTargetDim             invRenderTargetDim
# define prevFrameRenderTargetHalfPixelOffset    renderTargetHalfPixelOffset 
#endif


// Gears5 optimisation, compute shading rate for tile interiors only, not border pixels
// bonus, reduces LDS use and makes shader simpler since load TILE_SIZE+2 samples is awkward
groupshared float g_luma[TILE_SIZE][TILE_SIZE];
groupshared float g_depth[TILE_SIZE][TILE_SIZE];
#if SINGLE_WAVE_PER_TILE == 0
groupshared uint g_tileShadingRate;
#endif


// skip because the thread is on a tile edge and cannot read outside of LDS
// could be used to avoid reading off render target if not using a clamp sampler
void ThreadSkipShadingRateCalculation(out bool doVertical, out bool doHorizontal, uint2 src, uint2 threadId)
{
    doHorizontal = (threadId.x != 0) && (threadId.x != TILE_SIZE - 1);
    doVertical = (threadId.y != 0) && (threadId.y != TILE_SIZE - 1);
}


#if CHECK_DEPTH_DISCONTINUINTY != 0
uint CalcShadingRateWithDepth(bool doVertical, bool doHorizontal, uint2 src, uint2 threadId, float depth, float luma, float lumaDeltaH1, float lumaDeltaH2, float lumaDeltaV1, float lumaDeltaV2)
{
    float centralDepth = g_depth[threadId.x][threadId.y];
#if KERNAL == KERNEL_FAVOUR_TEMPORAL_STABILITY
    float centralLuma = g_luma[threadId.x][threadId.y];
#endif
    uint shadingRateH = D3D12_SHADING_RATE_2X1;
    uint shadingRateV = D3D12_SHADING_RATE_1X2;

    if (doHorizontal)
    {
        float adjDepth1 = g_depth[threadId.x - 1][threadId.y];
        float adjDepth2 = g_depth[threadId.x + 1][threadId.y];
#if KERNAL == KERNEL_PRESERVE_SINGLE_PIXEL_ARTEFACTS
        float depthValid1 = abs(centralDepth - adjDepth1) <= DEPTH_DISCONTINUINTY_TOLERANCE;
        float depthValid2 = abs(centralDepth - adjDepth2) <= DEPTH_DISCONTINUINTY_TOLERANCE;

        shadingRateH = ((lumaDeltaH1 * depthValid1 + lumaDeltaH2 * depthValid2) <= shadingRateTolerance) ? D3D12_SHADING_RATE_2X1 : D3D12_SHADING_RATE_1X1;
#elif KERNAL == KERNEL_FAVOUR_TEMPORAL_STABILITY
        bool depthValid1 = abs(centralDepth - adjDepth1) <= DEPTH_DISCONTINUINTY_TOLERANCE;
        bool depthValid2 = abs(centralDepth - adjDepth2) <= DEPTH_DISCONTINUINTY_TOLERANCE;

        float adjLuma1 = depthValid1 ? lumaDeltaH1 : centralLuma;
        float adjLuma2 = depthValid2 ? lumaDeltaH2 : centralLuma;

        shadingRateH = ((max3(centralLuma, adjLuma1, adjLuma2) - min3(centralLuma, adjLuma1, adjLuma2)) <= shadingRateTolerance) ? D3D12_SHADING_RATE_2X1 : D3D12_SHADING_RATE_1X1;
#else
# error
#endif
    }
    if (doVertical)
    {
        float adjDepth1 = g_depth[threadId.x][threadId.y - 1];
        float adjDepth2 = g_depth[threadId.x][threadId.y + 1];
#if KERNAL == KERNEL_PRESERVE_SINGLE_PIXEL_ARTEFACTS
        float depthValid1 = abs(centralDepth - adjDepth1) <= DEPTH_DISCONTINUINTY_TOLERANCE;
        float depthValid2 = abs(centralDepth - adjDepth2) <= DEPTH_DISCONTINUINTY_TOLERANCE;

        shadingRateV = ((lumaDeltaV1 * depthValid1 + lumaDeltaV2 * depthValid2) <= shadingRateTolerance) ? D3D12_SHADING_RATE_1X2 : D3D12_SHADING_RATE_1X1;
#elif KERNAL == KERNEL_FAVOUR_TEMPORAL_STABILITY
        bool depthValid1 = abs(centralDepth - adjDepth1) <= DEPTH_DISCONTINUINTY_TOLERANCE;
        bool depthValid2 = abs(centralDepth - adjDepth2) <= DEPTH_DISCONTINUINTY_TOLERANCE;

        float adjLuma1 = depthValid1 ? lumaDeltaV1 : centralLuma;
        float adjLuma2 = depthValid2 ? lumaDeltaV2 : centralLuma;

        shadingRateV = ((max3(centralLuma, adjLuma1, adjLuma2) - min3(centralLuma, adjLuma1, adjLuma2)) <= shadingRateTolerance) ? D3D12_SHADING_RATE_1X2 : D3D12_SHADING_RATE_1X1;
#else
# error
#endif
    }
    return shadingRateH | shadingRateV;
}
#endif


uint CalcShadingRate(bool doVertical, bool doHorizontal, uint2 src, uint2 threadId, float luma, out float lumaDeltaH1, out float lumaDeltaH2, out float lumaDeltaV1, out float lumaDeltaV2)
{
    float centralLuma = g_luma[threadId.x][threadId.y];
    uint shadingRateH = D3D12_SHADING_RATE_2X1;
    uint shadingRateV = D3D12_SHADING_RATE_1X2;

    // keep shader compiler happy with its warnings, although these values cannot be used uninitialized
    lumaDeltaH1 = 0.0;
    lumaDeltaH2 = 0.0;
    lumaDeltaV1 = 0.0;
    lumaDeltaV2 = 0.0;

    if (doHorizontal)
    {
        float adjLuma1 = g_luma[threadId.x - 1][threadId.y];
        float adjLuma2 = g_luma[threadId.x + 1][threadId.y];

#if KERNAL == KERNEL_PRESERVE_SINGLE_PIXEL_ARTEFACTS
        lumaDeltaH1 = abs(centralLuma - adjLuma1);
        lumaDeltaH2 = abs(centralLuma - adjLuma2);

        shadingRateH = ((lumaDeltaH1 + lumaDeltaH2) <= shadingRateTolerance) ? D3D12_SHADING_RATE_2X1 : D3D12_SHADING_RATE_1X1;
#elif KERNAL == KERNEL_FAVOUR_TEMPORAL_STABILITY
        lumaDeltaH1 = adjLuma1;     // cache for depth test
        lumaDeltaH2 = adjLuma2;

        shadingRateH = ((max3(centralLuma, adjLuma1, adjLuma2) - min3(centralLuma, adjLuma1, adjLuma2)) <= shadingRateTolerance) ? D3D12_SHADING_RATE_2X1 : D3D12_SHADING_RATE_1X1;
#else
# error
#endif
    }
    if (doVertical)
    {
        float adjLuma1 = g_luma[threadId.x][threadId.y - 1];
        float adjLuma2 = g_luma[threadId.x][threadId.y + 1];

#if KERNAL == KERNEL_PRESERVE_SINGLE_PIXEL_ARTEFACTS
        lumaDeltaV1 = abs(centralLuma - adjLuma1);
        lumaDeltaV2 = abs(centralLuma - adjLuma2);

        shadingRateV = ((lumaDeltaV1 + lumaDeltaV2) <= shadingRateTolerance) ? D3D12_SHADING_RATE_1X2 : D3D12_SHADING_RATE_1X1;
#elif KERNAL == KERNEL_FAVOUR_TEMPORAL_STABILITY
        lumaDeltaV1 = adjLuma1;     // cache for depth test
        lumaDeltaV2 = adjLuma2;

        shadingRateV = ((max3(centralLuma, adjLuma1, adjLuma2) - min3(centralLuma, adjLuma1, adjLuma2)) <= shadingRateTolerance) ? D3D12_SHADING_RATE_1X2 : D3D12_SHADING_RATE_1X1;
#else
# error
#endif
    }
    return shadingRateH | shadingRateV;
}


uint CalcWaveShadingRate(uint shadingRate)
{
    // WaveActiveBitAnd(shadingRate) works, but its no faster on XBox as its implemented by the shader compiler as a parallel reduction
    // This alternative is the same speed, but has the advantage that we can ignore single sample counts, where the rate has been boosted
    // probably by just a single delta. Except for at boundaries, a single delta may be counted twice
    uint count1x1 = WaveActiveCountBits(shadingRate == D3D12_SHADING_RATE_1X1);

    if (count1x1 > IGNORE_HIGH_RATE_PIXEL_COUNT)
    {
        return D3D12_SHADING_RATE_1X1;
    }
    uint count1x2 = WaveActiveCountBits(shadingRate == D3D12_SHADING_RATE_1X2);
    uint count2x1 = WaveActiveCountBits(shadingRate == D3D12_SHADING_RATE_2X1);

    if ((count1x2 | count2x1) > IGNORE_HIGH_RATE_PIXEL_COUNT)
    {
        if ((count1x2 > IGNORE_HIGH_RATE_PIXEL_COUNT) && (count2x1 > IGNORE_HIGH_RATE_PIXEL_COUNT))
        {
            return D3D12_SHADING_RATE_1X1;
        }
        return (count1x2 > count2x1) ? D3D12_SHADING_RATE_1X2 : D3D12_SHADING_RATE_2X1;
    }
    return D3D12_SHADING_RATE_2X2;
}


uint CalcTileShadingRate(uint shadingRate, uint threadIndex)
{
    uint waveShadingRate = CalcWaveShadingRate(shadingRate);

#if SINGLE_WAVE_PER_TILE != 0
    return waveShadingRate;
#else
    // combine results from multiple waves. I wish we had SV_GroupWaveIndex, and could size LDS arrays by the number of
    // waves in a group. This would allow wave0 to loop over the results from the other waves, and avoid using atomics
    if ((threadIndex == 0) && (waveShadingRate != D3D12_SHADING_RATE_2X2))
    {
        InterlockedMin(g_tileShadingRate, waveShadingRate);
    }
    GroupMemoryBarrierWithGroupSync();

    return g_tileShadingRate;
#endif
}


float GetLumaAndApplyCurve(float3 colour)
{
    float luma = BuildLuma(colour);
#if LUMA_CURVE == LUMA_CURVE_NONE
    // linear = nop
#elif LUMA_CURVE == LUMA_CURVE_SQRT
    luma = sqrt(luma);
#elif LUMA_CURVE == LUMA_CURVE_REINHARD_2X
    luma = (2.0 * luma) / (1.0 + luma);
#else
# error "Undefined luma curve"
#endif
    return luma;
}


#if CHECK_DEPTH_DISCONTINUINTY != 0
bool CheckDepthCondition(uint tileShadingRate)
{
# if CHECK_DEPTH_FOR_ANY_HIGH_RATE != 0
    return D3D12_SHADING_RATE_2X2 != tileShadingRate;
# else
    return D3D12_SHADING_RATE_1X1 == tileShadingRate;
# endif
}
#endif



[RootSignature(GlobalRS)]
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
#if SINGLE_WAVE_PER_TILE != 0
void GenerateShadingRate(uint2 GTid : SV_GroupThreadId, uint2 Gid : SV_GroupID)
#else
void GenerateShadingRate(uint2 GTid : SV_GroupThreadId, uint2 Gid : SV_GroupID, uint GTI : SV_GroupIndex)
#endif
{
    // If we have multiple waves involved in producing the shading rate
    // have the first wave set the shading rate in LDS, such that we can update via atomic min
#if SINGLE_WAVE_PER_TILE == 0
    if (0 == GTI)
    {
        g_tileShadingRate = D3D12_SHADING_RATE_2X2;
    }
#endif
    // a real title would likely drop these options, they're here for experimentation / demo purposes
    bool vrsEnabled = rootConstantCB1 & 1;
    bool generateSparseLighting2x2TileSize = rootConstantCB1 & 2;
    /*
        Shading rate all the way round the perimeter is always min (2x2 rate) and so we don't dispatch groups for the 4x edges, edges are 16 pixels wide
        this also prevents the shader having to care about threads which be processing pixels off the edge of the SRV, which are in the TV safe area anyway /
        think of leaving the border at 2x2 always as foveated rendering and a nice speed up ;)

        When DRS is enabled however and we have a resolution change, we have to update the right and bottom safe area with 2x2 rate
        the left and top edges are memset to 2x2 rate just once and never change, the right and bottom need to write 2x2 only on DRS resolution change
    */
#if COMPUTE_SAFE_AREA != 0
    uint2 tileId = Gid;
#else
    uint2 tileId = Gid + uint2(2, 2);   // skip 16x16 border in the safe area

    // Do we have to clear the right or bottom tiles? This test should only ever pass on DRS resolution change
    // when we do not change resolution, the CPU does not dispatch enough tiles for this to pass, on resolution
    // change extra thread groups are dispatched to process right and bottom edges, and this check will pass
# if DRS_SUPPORTED != 0
    if (any(tileId.xy >= vrsBottomRightTileSize.xy))
    {
        if (0 == GTI)
        {
            ShadingRateImage8x8_UAV[tileId.xy] = D3D12_SHADING_RATE_2X2;
        }
        return;
    }
# endif
#endif
    // calculate 2D tile local coord
    uint threadIndex = WaveGetLaneIndex();
    uint2 threadId = GTid;

#if SUPPORT_2x2_TILE_SIZE != 0
    if (generateSparseLighting2x2TileSize)
    {
        // for using the 4x lane crossbar, we need to change the thread ordering so 4 threads in linear order cover a 2x2 pixel region
        // this means constructing 4x4 quads of 2x2 pixels each
        threadId = GetCSQuadCoord_Wave64_88(threadIndex);
    }
#endif
    // which pixel is this thread processing?
    uint2 src;
    src.x = __XB_MadU24(tileId.x, TILE_SIZE, threadId.x);
    src.y = __XB_MadU24(tileId.y, TILE_SIZE, threadId.y);

#if USE_SAMPLER != 0
    // could do (coord + 0.5) * prevFrameInvRenderTargetDim, but this way the compiler gets to use vMAD instructions
    float2 uv = float2(src)*prevFrameInvRenderTargetDim + prevFrameRenderTargetHalfPixelOffset;

    // clamping handles sampling off the texture, however this doesn't happen in this implementationa as safe area is not processed
    // could use .Load if dynamic resolution hasn't changed, which can be faster on Scarlett HW if TEX performance is a bottleneck
    float3 colour = PostProcessSRV.SampleLevel(LinearClampSampler, uv, 0.0).xyz;
#else
    float3 colour = PostProcessSRV[src];
#endif
    float luma = GetLumaAndApplyCurve(colour);

    g_luma[threadId.x][threadId.y] = luma;

    // work out what calculations to perform
    bool doVertical, doHorizontal;
    ThreadSkipShadingRateCalculation(doVertical, doHorizontal, src, threadId);

    // Make sure LDS is fully populated AND g_tileShadingRate has been initialized
#if SINGLE_WAVE_PER_TILE == 0
    GroupMemoryBarrierWithGroupSync();
#endif
    // calculate the shading rate
    float lumaDeltaH1, lumaDeltaH2, lumaDeltaV1, lumaDeltaV2;
    uint waveShadingRate = CalcShadingRate(doVertical, doHorizontal, src, threadId, luma, lumaDeltaH1, lumaDeltaH2, lumaDeltaV1, lumaDeltaV2);

    if (vrsEnabled)
    {
        // compute single shading rate for the tile
        uint tileShadingRate = CalcTileShadingRate(waveShadingRate, threadIndex);

        // check if we need to load depth an perform depth testing
#if CHECK_DEPTH_DISCONTINUINTY != 0
        bool doDepthCheck = CheckDepthCondition(tileShadingRate);

        if (doDepthCheck)   // scalar per thread group
        {
            // load depth
# if USE_SAMPLER != 0
            float depth = LinearDepthSRV.SampleLevel(LinearClampSampler, uv, 0.0).x;
# else
            float depth = LinearDepthSRV[src];
# endif
            g_depth[threadId.x][threadId.y] = depth;

            // Make sure LDS is fully populated
#if SINGLE_WAVE_PER_TILE == 0
            g_tileShadingRate = D3D12_SHADING_RATE_2X2;

            GroupMemoryBarrierWithGroupSync();
#endif
            // Re-evalulate shading rate with depth deltas
            waveShadingRate = CalcShadingRateWithDepth(doVertical, doHorizontal, src, threadId, depth, luma, lumaDeltaH1, lumaDeltaH2, lumaDeltaV1, lumaDeltaV2);

            // compute single shading rate for the tile
            tileShadingRate = CalcTileShadingRate(waveShadingRate, threadIndex);
        }
#endif
#if SINGLE_WAVE_PER_TILE == 0
        // first wave makes write to UAV
        if (0 == GTI)
        {
            ShadingRateImage8x8_UAV[tileId.xy] = tileShadingRate;
        }
#else
        // just one wave per tile, write directly to UAV
        if (threadIndex == 0)
        {
            ShadingRateImage8x8_UAV[tileId.xy] = tileShadingRate;
        }
#endif
    }
#if SUPPORT_2x2_TILE_SIZE != 0
    // XBox uses 4 lane cross bar, which PC would likely have to emulate at cost using group shared memory
    if (generateSparseLighting2x2TileSize)
    {
        // one shading rate per 2x2 pixels, 4 threads contribute to each shading rate
        // using bitwise AND, we only get a bit set (low rate), if all pixels agree on the low rate
        // diagonal automatically incorporated here
        waveShadingRate &= FastReadAcrossX(waveShadingRate);
        waveShadingRate &= FastReadAcrossY(waveShadingRate);

        // Each 8x8 input tile has 4x4 outputs
        uint2 sparseLightingCoord;      // __XB_MadU24(tileId, 4, threadId >> 1); wish this compiled
        sparseLightingCoord.x = __XB_MadU24(tileId.x, 4, threadId.x >> 1);
        sparseLightingCoord.y = __XB_MadU24(tileId.y, 4, threadId.y >> 1);

        if ((threadIndex & 3) == 0)     // one thread per 2x2
        {
            ShadingRateImage2x2_UAV[sparseLightingCoord] = waveShadingRate;
        }
    }
#endif
}
