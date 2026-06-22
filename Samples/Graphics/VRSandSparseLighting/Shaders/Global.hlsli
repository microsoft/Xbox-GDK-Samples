//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define __XBOX_SV_POS_ZW_VIA_ATTRIBUTE  // sensible default

#define BUILD_LAKES             0

#define FLT_MAX                 3.402823466e+38F

// lifted from the spec https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html
#define D3D12_SHADING_RATE_1X1  0x0      // No change to shading rate
#define D3D12_SHADING_RATE_1X2  0x1      // Reduces vertical resolution 2x
#define D3D12_SHADING_RATE_2X1  0x4      // Reduces horizontal resolution 2x
#define D3D12_SHADING_RATE_2X2  0x5      // Reduces both axes by 2x


struct DrawInstancedIndexedArgs
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int  BaseVertexLocation;
    uint StartInstanceLocation;
};

struct AABB
{
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;
};

struct Plane
{
    float3 normal;
    float D;
};

struct Frustum
{
    Plane planes[5];
};


#define GlobalRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS | DENY_HULL_SHADER_ROOT_ACCESS),"\
	"CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(SRV(t0, numDescriptors=23, flags = DATA_VOLATILE), visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(UAV(u0, numDescriptors=27, flags = DATA_VOLATILE), visibility=SHADER_VISIBILITY_ALL),"\
    "RootConstants(num32BitConstants=1, b1),"\
    "StaticSampler(s0,"\
    "           filter = FILTER_ANISOTROPIC,"\
    "           addressU = TEXTURE_ADDRESS_CLAMP,"\
    "           addressV = TEXTURE_ADDRESS_CLAMP,"\
    "           addressW = TEXTURE_ADDRESS_CLAMP,"\
    "           maxAnisotropy = 16,"\
    "           comparisonFunc = COMPARISON_ALWAYS,"\
    "           visibility = SHADER_VISIBILITY_ALL ),"\
	"StaticSampler(s1,"\
    "           filter = FILTER_MIN_MAG_LINEAR_MIP_POINT,"\
    "           addressU = TEXTURE_ADDRESS_CLAMP,"\
    "           addressV = TEXTURE_ADDRESS_CLAMP,"\
    "           addressW = TEXTURE_ADDRESS_CLAMP,"\
    "           maxAnisotropy = 1,"\
    "           comparisonFunc = COMPARISON_ALWAYS,"\
    "           visibility = SHADER_VISIBILITY_ALL ),"\
	"StaticSampler(s2,"\
    "           filter = FILTER_MIN_MAG_MIP_POINT,"\
    "           addressU = TEXTURE_ADDRESS_CLAMP,"\
    "           addressV = TEXTURE_ADDRESS_CLAMP,"\
    "           addressW = TEXTURE_ADDRESS_CLAMP,"\
    "           maxAnisotropy = 1,"\
    "           comparisonFunc = COMPARISON_ALWAYS,"\
    "           visibility = SHADER_VISIBILITY_ALL ),"\
	"StaticSampler(s3,"\
    "           filter = FILTER_MIN_MAG_LINEAR_MIP_POINT,"\
    "           addressU = TEXTURE_ADDRESS_BORDER,"\
    "           addressV = TEXTURE_ADDRESS_BORDER,"\
    "           addressW = TEXTURE_ADDRESS_CLAMP,"\
    "           borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,"\
    "           maxAnisotropy = 1,"\
    "           comparisonFunc = COMPARISON_ALWAYS,"\
    "           visibility = SHADER_VISIBILITY_ALL )"

// SRVs
// Terrain
Texture2D<float> SourceHeightMapSRV0						: register(t0);
Texture2D<float> SourceHeightMapSRV1						: register(t1);
Texture2D<float> HeightMapSRV								: register(t2);
Texture2D<float2> NormalMapSRV								: register(t3);
Buffer<uint> AABBTileMinMaxSRV								: register(t4);     // tile size for culling
Texture2D<float> LowResMaxHeightSRV			    			: register(t5);     // 8x8 for ray marching
Buffer<uint> InstanceBufferSRV[4]      						: register(t6);     // verylow/low/medium/high res

// Post processing
Texture2D<float3> LitOutputSRV       	    				: register(t10);
Texture2D<float3> PostProcessSRV       	    				: register(t11);

// GBuffers
Texture2D<float3> GbufferAlbedoSRV       	    			: register(t12);
Texture2D<float4> GBufferRoughnessNormalSRV       	    	: register(t13);
Texture2D<float4> GBufferDebugAndShadingRateSRV    	    	: register(t14);

// Depth, linear depth, sparse coordinates and shading rate images
Texture2D<float> LinearDepthSRV                    	    	: register(t15);
Texture2D<float> LinearDepthHalfResSRV                    	: register(t16);
StructuredBuffer<uint> SparseLightingCountSRV               : register(t17);
Buffer<uint> SparseLightingCoordsSRV                        : register(t18);
Texture2D<float> DepthSRV                                   : register(t19);
ByteAddressBuffer HTileSRV                                  : register(t20);
Texture2D<uint> ShadingRateImage8x8_SRV	          			: register(t21);
Texture2D<uint> ShadingRateImage2x2_SRV	    	            : register(t22);    



// UAVS
// Terrain
RWTexture2D<float> SourceHeightMapUAV0						: register(u0);
RWTexture2D<float> SourceHeightMapUAV1						: register(u1);
RWTexture2D<float> HeightMapUAV								: register(u2);
RWTexture2D<float2> NormalMapUAV[4]							: register(u3);     // with mipmaps
RWBuffer<uint> AABBTileMinMaxUAV							: register(u7);     
RWBuffer<uint> InstanceBufferUAV[4]  						: register(u8);     // verylow/low/medium/high res
RWStructuredBuffer<DrawInstancedIndexedArgs> DrawArgs		: register(u12);	// 2x args per LOD level = 8

// Post processing
RWTexture2D<float3> LitOutputUAV					    	: register(u13);
RWTexture2D<float3> PostProcessUAV					    	: register(u14);

// GBuffers
RWTexture2D<float3> GbufferAlbedoUAV       	    			: register(u15);
RWTexture2D<float4> GBufferRoughnessNormalUAV      	    	: register(u16);
RWTexture2D<float4> GBufferDebugAndShadingRateUAV        	: register(u17);

// Depth, linear depth, sparse coordinates and shading rate images
RWTexture2D<uint> ShadingRateImage8x8_UAV			    	: register(u18);
RWTexture2D<uint> ShadingRateImage2x2_UAV			        : register(u19);
RWTexture2D<float> LowResMaxHeightUAV						: register(u20);
RWTexture2D<float> LinearDepthUAV                           : register(u21);
RWTexture2D<float> LinearDepthHalfResUAV                    : register(u22);
RWStructuredBuffer<uint> SparseLightingCountUAV             : register(u23);
RWBuffer<uint> SparseLightingCoordsUAV                      : register(u24);

// Back buffers
// Unfortunately cannot use a descriptor array. The validation layer cannot detect that we are not accessing the UAV for the other back buffer thats in present state
//RWTexture2D<float3> BackBufferUAV[2]                      : register(u25);
RWTexture2D<float3> BackBuffer0UAV                          : register(u25);
RWTexture2D<float3> BackBuffer1UAV                          : register(u26);


// Static samplers
SamplerState AnisoSampler									: register(s0);
SamplerState LinearClampSampler								: register(s1);
SamplerState PointClampSampler								: register(s2);
SamplerState LinearBorderSampler							: register(s3);





cbuffer RootConstants : register(b1)
{
    uint            rootConstantCB1;
}

cbuffer TerrainCB : register(b0)
{
    // terrain 
    float			invSourceHeightMapSize;
    float           invHeightMapSize;
    uint			terrainTileSizeLog2;
    uint			terrainTileCountPerAxis;

    uint			frameIndex;
    float           worldScaleY;
    float           worldScale;
    float           xzTranslation;

    float           halfTexelHeightMapSize;
    float           sourceToHeightMapSizeMultiplier;
    uint            sourceToHeightMapSizeLog2;
    float           worldToHeightMapCoordMul;

    // for ray marching
    float           lowResMaxHeightMapSize; 
    float3          rayMarchLightDir;                   // dir is not normal, unit in xz
    float3          lightDir;
    float           stepSize;

    // camera
    float4x4		wvpMatrix;
    float3          cameraPos;
    float           depthRange;                     // fRange from XMMatrixPerspectiveFovLH (FarZ / (FarZ - NearZ))
    float3          frustumHDelta;
    float           depthRangeNegNearZDivFarZ;      // (-fRange * NearZ from XMMatrixPerspectiveFovLH) / FarZ
    float3          frustumVDelta;
    uint            htileInfo;                      // htile data for decoding compressed depth
    float3          frustumOrigin;
    float           farZ;
    float3          cameraForwardVector;
    uint            pad1;

    uint2           renderTargetDims;
    uint            sparseLightingBufferWidth;
    uint            sparseLightingBufferHeight;

    Frustum         viewFrustum;

    // pixel.xy to UV
    float2          invRenderTargetDim;
    float2          renderTargetHalfPixelOffset;

    // shading rate generation 
    float           shadingRateTolerance;

    // sparse lighting
    uint            sparseLightingFrameIndex;       // legal values are only 0..3

    uint            sparseLightingLCopyCodeTL;      // top left
    uint            sparseLightingLCopyCodeTR;      // top right
    uint            sparseLightingLCopyCodeBL;      // bottom left
    uint            sparseLightingLCopyCodeBR;      // bottom right

    // count of tiles in shading rate image, including safe area on left and top, but not including safe area on bottom and right
    uint2           vrsBottomRightTileSize;

    // debug visualizations
    float2          debugSurfaceSizeF;
    uint2           debugSurfaceSizeU;
};


// Note I'm not fully convinced this function is 100% safe on PC
uint GetGroupWaveIndex(uint svGroupIndex)
{
#if defined __XBOX_ONE || defined __XBOX_SCARLETT
# ifdef __XBOX_ENABLE_WAVE32
    return WaveReadLaneFirst(svGroupIndex) / 32;
# else
    return WaveReadLaneFirst(svGroupIndex) / 64;
# endif
#else
    return WaveReadLaneFirst(svGroupIndex) / WaveGetLaneCount();    // issues a divide on XBox, we prefer shifts of course!
#endif
}


uint AlignUp(uint value, uint alignment)
{
    uint mask = alignment - 1;
    return (value + mask) & ~mask;
}


bool TestEquality(float v1, float v2, float tolerance)
{
    return abs(v1 - v2) < tolerance;
}


float TestEquality(float v1, float v2)
{
    return abs(v1 - v2);
}


float TestEquality(float3 c1, float3 c2)
{
    return __XB_Max3_F32(TestEquality(c1.x, c2.x), TestEquality(c1.y, c2.y), TestEquality(c1.z, c2.z));
}


bool TestEquality(float3 c1, float3 c2, float tolerance)
{
    return TestEquality(c1, c2) < tolerance;
}


// Convert SV_ShadingRate to only 2 bits
uint CompactShadingRate(uint svCoverage)
{
    return (svCoverage & D3D12_SHADING_RATE_1X2) | ((svCoverage & D3D12_SHADING_RATE_2X1) >> 1);
}


float BuildLuma(float3 colour)
{
    return dot(float3(0.299, 0.587, 0.114), colour);
}


// certainly lousy as a general purpose hash, but makes nice terrains
float RockyTerrainHash(float2 value)
{
    float3 hash = frac((1.0 / 3.14159265359) * float3(value.xy, value.x + value.y));
    hash += 3.14159265359 * dot(hash, hash.yzx);
    return frac(dot(hash, hash.zzz));
}


// 2D value noise and its analytical derivatives, from http://iquilezles.org/www/articles/morenoise/morenoise.htm
float3 noised(float2 pos)
{
    float2 p = floor(pos);
    float a = RockyTerrainHash(p + float2(0.5, 0.5));
    float b = RockyTerrainHash(p + float2(1.5, 0.5));
    float c = RockyTerrainHash(p + float2(0.5, 1.5));
    float d = RockyTerrainHash(p + float2(1.5, 1.5));

    float2 f = frac(pos);
    float2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    float k0 = b - a;
    float k1 = c - a;
    float k2 = a - b - c + d;

    return float3(  a + (k0 * u.x) + (k1 * u.y) + (k2 * u.x * u.y),			// value
                    6.0 * f * (1.0 - f) * (float2(k0, k1) + k2 * u.yx));	// derivatives
}


float Fbm(float2 pos, int octaves, float amp = 0.5, float lacunarity = 2.0, float gain = 0.5)
{
    const float2x2 m2 = float2x2(0.8, -0.6, 0.6, 0.8);
    float2 freq = float2(0.0, 0.0);
    float sum = 0.0;

    for (int i = 0; i < octaves; i++)
    {
        float3 noise = noised(pos);
        freq += noise.yz;
        sum += abs(amp * noise.x / (1.0 + dot(freq, freq)));
        amp *= gain;
        pos = mul(pos, m2) * lacunarity;
    }
    return sum;
}


// Jim Hejl's excellent tone mapper
// https://twitter.com/jimhejl/status/633777619998130176?lang=en    
float3 ToneMap(float3 hdr)
{
    hdr *= 3.0;     // brightness multiplier

    float4 vh = float4(hdr, 50.0);  // whitepoint
    float4 va = (1.435 * vh) + 0.05;
    float4 vf = ((vh * va + 0.004) / ((vh * (va + 0.55) + 0.0491))) - 0.0821;

    // for some reason DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 is giving me a white screen output...,
    // possible this is not working with autoHDR. So we do manual Gamma 2.2 in shader which is not ideal
    return pow(vf.xyz / vf.w, 2.2f);    // hopefully compiler manages to pre-compute reciprocal for vf.w since this is derived from literals
}


// convert from reverseZ depth to linear fowardZ as this saves ALU / is more intuitive
float LinearizeAndForwardZ(float depth)
{
    return depthRangeNegNearZDivFarZ / ((1.0 - depth) - depthRange);
}
