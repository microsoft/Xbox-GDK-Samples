//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Global.hlsli"

#define NORMAL_ENCODING_SCALE                                           2.5     // scale normal.xy to use full range of S8 (since terrain does not generate normals such as -1,0,0)

#define TERRAIN_VERY_LOW_LOD_DISTANCE                                   60.0
#define TERRAIN_LOW_LOD_DISTANCE                                        45.0
#define TERRAIN_MED_LOD_DISTANCE                                        25.0
#define TERRAIN_TILE_CONSIDERD_FLAT_Y_DELTA_VERY_LOW                    0.5
#define TERRAIN_TILE_CONSIDERD_FLAT_Y_DELTA_LOW                         1.0
#define TERRAIN_TILE_CONSIDERD_FLAT_Y_DELTA_MED                         1.5
#define TERRAIN_TILE_Y_DELTA_SCALE                                      0.5
#define TERRAIN_TILE_HEIGHT_ABOVE_WHICH_EXTRA_WEIGHT                    20.0
#define TERRAIN_TILE_HEIGHT_ABOVE_WHICH_LOD_BUMP                        30.0

#define TERRAIN_SMOOTHING_HEIGHT_CAP                                    10.0

#define TERRAIN_VALUES_CAN_BE_NEGATIVE	                                0

#define DEBUG_LODS                                                      0

#define BRANCHLESS_FLOAT_COMPARISON_UINT                                0


// for positive floating point numbers, asuint(x) < asuint(y) works
// for positive floats, flip the sign bit, so positive floats have the MSB set
// for negative floats, flip all bits, so MSB is not set
uint FloatToComparisonUint(float f)
{
    uint u = asuint(f);
#if BRANCHLESS_FLOAT_COMPARISON_UINT == 1
    uint mask = ~(u >> 31);					// (x < 0.0) ? 0xfffffffe : 0xffffffff
    mask = 1 + mask;						// (x < 0.0) ? 0xffffffff : 0x00000000
    mask |= 0x80000000;						// (x < 0.0) ? 0xffffffff : 0x80000000
#else
    uint mask = (f < 0.0) ? 0xffffffff : 0x80000000;
#endif
    return u ^ mask;						// (x < 0.0) ? flip all bits : flip sign bit only
}


// if MSB is set, number is positive, just flip the sign bit
// if MSB is not set, number is negative, flip all bits
float ComparisonUintToFloat(uint u)
{
#if BRANCHLESS_FLOAT_COMPARISON_UINT == 1
    uint mask = ~(u >> 31);					// (MSB set) ? 0xfffffffe : 0xffffffff
    mask = 1 + mask;						// (MSB set) ? 0xffffffff : 0x00000000
    mask ^= 0xffffffff;						// (MSB set) ? 0x00000000 : 0xffffffff
    mask |= 0x80000000;						// (MSB set) ? 0x80000000 : 0xffffffff
#else
    uint mask = (u & 0x80000000) ? 0x80000000 : 0xffffffff;
#endif
    return asfloat(u ^ mask);				// (MSB set) ? flip sign bit only : flip all bits
}


uint TerrainFloatToComparisonUint(float f)
{
#if TERRAIN_VALUES_CAN_BE_NEGATIVE == 1
    return FloatToComparisonUint(f);
#else
    return asuint(f);
#endif
}


float TerrainComparisonUintToFloat(uint u)
{
#if TERRAIN_VALUES_CAN_BE_NEGATIVE == 1
    return ComparisonUintToFloat(u);
#else
    return asfloat(u);
#endif
}


uint TerrainAABBAddress(uint2 tileId, uint tileCountPerAxis)
{
    return (tileId.x + (tileId.y * tileCountPerAxis)) * 2;
}


uint TerrainAABBAddress(uint2 coord, uint tileSizeLog2, uint tileCountPerAxis)
{
    return TerrainAABBAddress(coord.xy >> tileSizeLog2, tileCountPerAxis);
}


float3 ToneMap(float3 hdr)  // https://twitter.com/jimhejl/status/633777619998130176?lang=en    
{
    float4 vh = float4(hdr, 50.0);  // whitepoint
    float4 va = (1.435 * vh) + 0.05;
    float4 vf = ((vh * va + 0.004) / ((vh * (va + 0.55) + 0.0491))) - 0.0821;
    return pow(vf.xyz / vf.www, 2.2f);
}


float2 PosToUV(float3 pos)
{
    return (pos.xz - xzTranslation) * worldToTextureCoordMul;
}


float3 LoadAndDecodeNormal(float3 worldPos)
{
    float2 normalXZ = NormalMapSRV.Sample(AnisoSampler, PosToUV(worldPos)).xy / NORMAL_ENCODING_SCALE;
    float3 normal;
    normal.x = normalXZ.x;
    normal.y = 2.0 * sourceToHeightMapSizeMultiplier;
    normal.z = normalXZ.y;
    return normalize(normal);
}
