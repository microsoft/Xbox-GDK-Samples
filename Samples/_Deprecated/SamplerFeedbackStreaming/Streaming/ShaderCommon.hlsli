//--------------------------------------------------------------------------------------
// ShaderCommon.hlsli
//
// HLSL functions used in emulating Sampler Feedback, including expansions of texture
// sampling to perform MinLOD map sampling and MinLOD feedback map writing.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma warning(disable: 4000)

#if !XBOX_SAMPLER_FEEDBACK
typedef RWBuffer<uint> FeedbackTexture2DEmu;
#include "SamplerFeedbackEmulationBuffer.hlsli"

#include "MinLODFilterEmulation.hlsli"
#endif

struct VS_IN
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord0 : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Binormal : BINORMAL;
};

struct VS_OUT
{
    float2 TexCoord0 : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float3 Tangent : TEXCOORD2;
    float3 Binormal : TEXCOORD3;
    float4 Color : TEXCOORD4;
    float3 ViewDirection : TEXCOORD5;
    float4 Pos : SV_POSITION;
};

struct PS_IN
{
    float2 TexCoord0 : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float3 Tangent : TEXCOORD2;
    float3 Binormal : TEXCOORD3;
    float4 Color : TEXCOORD4;
    float3 ViewDirection : TEXCOORD5;
};

cbuffer cbModel : register(b0)
{
    row_major float4x4 g_WorldTransform : packoffset(c0);

#if !XBOX_SAMPLER_FEEDBACK
    uint4 g_ObjectDiffuseLayer1_FeedbackDimensions   : packoffset(c4);
    uint4 g_ObjectNormalLayer1_FeedbackDimensions    : packoffset(c5);
    uint4 g_ObjectSpecularLayer1_FeedbackDimensions  : packoffset(c6);
    uint4 g_ObjectDiffuseLayer2_FeedbackDimensions  : packoffset(c7);
    uint4 g_ObjectNormalLayer2_FeedbackDimensions   : packoffset(c8);
    uint4 g_ObjectSpecularLayer2_FeedbackDimensions : packoffset(c9);
    uint4 g_ObjectDiffuseLayer3_FeedbackDimensions  : packoffset(c10);
    uint4 g_ObjectNormalLayer3_FeedbackDimensions   : packoffset(c11);
    uint4 g_ObjectSpecularLayer3_FeedbackDimensions : packoffset(c12);
#endif
};

cbuffer cbScene : register(b1)
{
    row_major float4x4 g_VPTransform    : packoffset(c0);
    float4 g_Zero                       : packoffset(c4);
    float4 g_CameraPosWorld             : packoffset(c5);
    float4 g_FilterSlopes               : packoffset(c6);
    float4 g_NonResidentColor           : packoffset(c7);
    float4 g_StochasticConstants	    : packoffset(c8);
};

SamplerState g_BilinearSampler : register(s0);
SamplerState g_MinMipMapSampler : register(s13);
SamplerState g_PointSampler : register(s14);
SamplerState g_MaxReductionSampler : register(s15);

#if XBOX_SAMPLER_FEEDBACK

#define DeclareTextureObject(Name, RegIndex) \
    Texture2D Name : register(t##RegIndex, space0); \
    Texture2D Name##_MinLOD : register(t##RegIndex, space1); \
    FeedbackTexture2D<SAMPLER_FEEDBACK_MIN_MIP> Name##_Feedback : register(u##RegIndex, space2);

#else

#define DeclareTextureObject(Name, RegIndex) \
    Texture2D Name : register(t##RegIndex, space0); \
    Texture2D Name##_MinLOD : register(t##RegIndex, space1); \
    RWBuffer<uint> Name##_Feedback : register(u##RegIndex, space2);

#endif

DeclareTextureObject(g_ObjectDiffuseLayer1,  0);
DeclareTextureObject(g_ObjectNormalLayer1,   1);
DeclareTextureObject(g_ObjectSpecularLayer1, 2);
DeclareTextureObject(g_ObjectDiffuseLayer2,  3);
DeclareTextureObject(g_ObjectNormalLayer2,   4);
DeclareTextureObject(g_ObjectSpecularLayer2, 5);
DeclareTextureObject(g_ObjectDiffuseLayer3,  6);
DeclareTextureObject(g_ObjectNormalLayer3,   7);
DeclareTextureObject(g_ObjectSpecularLayer3, 8);

#if _XBOX
float NormalizedRandom(float2 UnusedInput)
{
    uint LaneID = __XB_GetLaneID();
    uint SwizzledLaneID = LaneID ^ (LaneID << 2) ^ (LaneID << 5);
    uint RandomBits = __XB_MemTime().x ^ SwizzledLaneID;
    float RandomValue = (float)(RandomBits & 0x7F) / 128.0f;
    return RandomValue;
}
#else
float NormalizedRandom(float2 Input)
{
    Input += g_StochasticConstants.yy;
    float3 PhaseConstants = float3(23.14069263277926, 2.665144142690225, 157.32479);
    return frac(cos(dot(Input, PhaseConstants.xy)) * PhaseConstants.z);
}
#endif

float4 LODValueToColor(float LODValue)
{
    uint MipIndex = (uint)LODValue;
    MipIndex = min(MipIndex, 6);

    float4 Colors[7] = {
        float4(1, 0, 0, 1),
        float4(1, 0.5f, 0, 1),
        float4(1, 1, 0, 1),
        float4(0, 1, 0, 1),
        float4(0, 1, 1, 1),
        float4(0, 0, 1, 1),
        float4(1, 0, 1, 1),
    };

    return Colors[MipIndex];
}

float4 LODValueToColorBlended(float LODValue)
{
    float FracValue = frac(LODValue);
    uint MipIndex = (uint)LODValue;
    MipIndex = min(MipIndex, 6);

    float4 Colors[8] = {
        float4(1, 0, 0, 1),
        float4(1, 0.5f, 0, 1),
        float4(1, 1, 0, 1),
        float4(0, 1, 0, 1),
        float4(0, 1, 1, 1),
        float4(0, 0, 1, 1),
        float4(1, 0, 1, 1),
        float4(0.25, 0.25, 0.25, 1),
    };

    return lerp(Colors[MipIndex], Colors[MipIndex + 1], FracValue);
}

bool ShouldWriteFeedback(float2 TexCoord)
{
    if (g_StochasticConstants.x >= 1.0f)
    {
        return true;
    }

    float RandomValue = NormalizedRandom(TexCoord);
    return RandomValue < g_StochasticConstants.x;
}

#if XBOX_SAMPLER_FEEDBACK

float4 SampleTextureExpansion(SamplerState samp, Texture2D tex, Texture2D minlodtex, FeedbackTexture2D<SAMPLER_FEEDBACK_MIN_MIP> feedbacktex, float2 TexCoord)
{
    // Sample from the MinLOD map using a hardware native max reduction sampler
    float EncodedLODClamp = minlodtex.Sample(g_MaxReductionSampler, TexCoord).x;

    // The MinLOD map is sampled as a R8_UNORM texture, but the elements within are encoded as 5.3 unsigned fixed.
    // To convert the [0, 1) results from filtering to the proper [0, 32) 5.3 ufixed range, simply multiply by 32.
    float LODClamp = EncodedLODClamp * 32.0f;

    // Sample from the tiled texture using the LOD clamp value
    int2 NullOffset = int2(0, 0);
    float4 Result = tex.Sample(samp, TexCoord, NullOffset, LODClamp);

    if (ShouldWriteFeedback(TexCoord))
    {
        // Write sampler feedback to the feedback map based on the sample patterns upon the tiled texture
        feedbacktex.WriteSamplerFeedback(tex, samp, TexCoord);
    }

    return Result;
}

float4 SampleTextureExpansionWithFilter(SamplerState samp, Texture2D tex, Texture2D minlodtex, FeedbackTexture2D<SAMPLER_FEEDBACK_MIN_MIP> feedbacktex, float2 TexCoord)
{
    // Sample from the MinLOD map using the hardware MinLOD map filter
    float LODClamp = minlodtex.Sample(g_MinMipMapSampler, TexCoord).x;

    // Sample from the tiled texture using the LOD clamp value
    int2 NullOffset = int2(0, 0);
    float4 Result = tex.Sample(samp, TexCoord, NullOffset, LODClamp);

    if (ShouldWriteFeedback(TexCoord))
    {
        // Write sampler feedback to the feedback map based on the sample patterns upon the tiled texture
        feedbacktex.WriteSamplerFeedback(tex, samp, TexCoord);
    }

    return Result;
}

#if USE_MINMIP_FILTER
#define SampleTexture(S,T,C) SampleTextureExpansionWithFilter(S,T,T##_MinLOD,T##_Feedback,C)
#else
#define SampleTexture(S,T,C) SampleTextureExpansion(S,T,T##_MinLOD,T##_Feedback,C)
#endif  

#else

float4 SampleTextureExpansion(SamplerState samp, Texture2D tex, Texture2D minlodtex, FeedbackTexture2DEmu feedbacktex, uint4 feedbackDim, float2 TexCoord)
{
    // Sample from the MinLOD map using a hardware native max reduction sampler
    float EncodedLODClamp = minlodtex.Sample(g_MaxReductionSampler, TexCoord).x;

    // The MinLOD map is sampled as a R8_UNORM texture, but the elements within are encoded as 5.3 unsigned fixed.
    // To convert the [0, 1) results from filtering to the proper [0, 32) 5.3 ufixed range, simply multiply by 32.
    float LODClamp = EncodedLODClamp * 32.0f;

    // Sample from the tiled texture using the LOD clamp value
    int2 NullOffset = int2(0, 0);
    float4 Result = tex.Sample(samp, TexCoord, NullOffset, LODClamp);

    if (ShouldWriteFeedback(TexCoord))
    {
        // Write sampler feedback to the feedback map based on the sample patterns upon the tiled texture
        FeedbackTexture2D_WriteSamplerFeedback(feedbacktex, feedbackDim, tex, samp, TexCoord);
    }

    return Result;
}

float4 SampleTextureExpansionWithFilterEmu(SamplerState samp, Texture2D tex, Texture2D minlodtex, FeedbackTexture2DEmu feedbacktex, uint4 feedbackDim, float2 TexCoord)
{
    // Sample from the MinLOD map using an emulated MinLOD map filter
    // The filter parameters are passed in from a constant buffer
    float EncodedLODClamp = SampleMinLODFilter2D(tex, minlodtex, TexCoord, g_BilinearSampler, g_FilterSlopes).x;

    // The MinLOD map is sampled as a R8_UNORM texture, but the elements within are encoded as 5.3 unsigned fixed.
    // To convert the [0, 1) results from filtering to the proper [0, 32) 5.3 ufixed range, simply multiply by 32.
    float LODClamp = EncodedLODClamp * 32.0f;

    // Sample from the tiled texture using the LOD clamp value
    int2 NullOffset = int2(0, 0);
    float4 Result = tex.Sample(samp, TexCoord, NullOffset, LODClamp);

    if (ShouldWriteFeedback(TexCoord))
    {
        // Write sampler feedback to the feedback map based on the sample patterns upon the tiled texture
        FeedbackTexture2D_WriteSamplerFeedback(feedbacktex, feedbackDim, tex, samp, TexCoord);
    }

    return Result;
}

#if USE_MINMIP_FILTER
#define SampleTexture(S,T,C) SampleTextureExpansionWithFilterEmu(S,T,T##_MinLOD,T##_Feedback,T##_FeedbackDimensions,C)
#else
#define SampleTexture(S,T,C) SampleTextureExpansion(S,T,T##_MinLOD,T##_Feedback,T##_FeedbackDimensions,C)
#endif  

#endif  
