//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// lifted from the spec https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html
#define D3D12_SHADING_RATE_1X1 0x0      // No change to shading rate
#define D3D12_SHADING_RATE_1X2 0x1      // Reduces vertical resolution 2x
#define D3D12_SHADING_RATE_2X1 0x4      // Reduces horizontal resolution 2x
#define D3D12_SHADING_RATE_2X2 0x5      // Reduces both axes by 2x


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
    "DescriptorTable(SRV(t0, numDescriptors=13, flags = DATA_VOLATILE), visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(UAV(u0, numDescriptors=17, flags = DATA_VOLATILE), visibility=SHADER_VISIBILITY_ALL),"\
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

Texture2D<float> SourceHeightMapSRV0						: register(t0);
Texture2D<float> SourceHeightMapSRV1						: register(t1);
Texture2D<float> HeightMapSRV								: register(t2);
Texture2D<float2> NormalMapSRV								: register(t3);
Buffer<uint> AABBTileMinMaxSRV								: register(t4);     // tile size for culling
Buffer<uint> InstanceBufferSRV[4]      						: register(t5);     // verylow/low/medium/high res
Texture2D<float4> RenderedImage1x1SRV 	    				: register(t9);
Texture2D<float4> RenderedImageVRSSRV 	    				: register(t10);
Texture2D<uint> ShadingRateMapSRV	          				: register(t11);
Texture2D<float> LowResMaxHeightSRV			    			: register(t12);    // 8x8 for ray marching

RWTexture2D<float> SourceHeightMapUAV0						: register(u0);
RWTexture2D<float> SourceHeightMapUAV1						: register(u1);
RWTexture2D<float> HeightMapUAV								: register(u2);
RWTexture2D<float2> NormalMapUAV[4]							: register(u3);
RWBuffer<uint> AABBTileMinMaxUAV							: register(u7);     
RWBuffer<uint> InstanceBufferUAV[4]  						: register(u8);     // verylow/low/medium/high res
RWStructuredBuffer<DrawInstancedIndexedArgs> DrawArgs		: register(u12);	// 2x args per LOD level = 8
RWTexture2D<float4> RenderedImage1x1UAV						: register(u13);
RWTexture2D<float4> RenderedImageVRSUAV						: register(u14);
RWTexture2D<uint> ShadingRateMapUAV			    			: register(u15);
RWTexture2D<float> LowResMaxHeightUAV						: register(u16);

SamplerState AnisoSampler									: register(s0);
SamplerState LinearClampSampler								: register(s1);
SamplerState PointClampSampler								: register(s2);
SamplerState LinearBorderSampler							: register(s3);





cbuffer RootConstants : register(b1)
{
    uint            sourceBufferIndex;
}

cbuffer TerrainCB : register(b0)
{
    // terrain 
    float			invSourceTextureSize;
    float           invTextureSize;
    uint			tileSizeLog2;
    uint			tileCountPerAxis;

    uint			frameIndex;
    float           worldScaleY;
    float           worldScale;
    float           xzTranslation;

    float           halfTexelOffsetSourceSize;
    float           sourceToHeightMapSizeMultiplier;
    uint            sourceToHeightMapSizeLog2;
    float           worldToTextureCoordMul;

    // for ray marching
    float           lowResMaxHeightMapSize; 
    float3          rayMarchLightDir;                   // dir is not normal, unit in xz
    float3          lightDir;
    float           stepSize;

    // camera
    float4x4		wvpMatrix;
    float3          cameraPos;
    uint            pad2;
    float3          frustumHDelta;
    uint            pad3;
    float3          frustumVDelta;
    uint            pad4;
    float3          frustumOrigin;
    uint            pad5;
    Frustum         viewFrustum;

    // shading rate generation 
    float           depthCompare;
    uint            discardSampleCountMaxRate;
    uint            discardSampleCountHalfRate;
    float           depthTolerance;

    float           SobelTolerance;
    float2          invInputDim;	// 1 / width, 1 / height
    uint            halfResInputMode;

    float2          shadingRateSurfaceSizeF;
    uint2           shadingRateSurfaceSizeU;
};



float3 ComputeDirection(float2 uv)
{
    float3 dir = frustumOrigin.xyz;
    dir += uv.x * frustumHDelta.xyz;
    dir += uv.y * frustumVDelta.xyz;
    return normalize(dir);
}
