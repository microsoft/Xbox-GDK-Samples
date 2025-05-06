/**********************************************************************
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
RTShadowDenoiser

tile_classification

********************************************************************/

struct FFX_DNSR_Shadows_Data_Defn
{
    float3   Eye;
    int      FirstFrame;
    int2     BufferDimensions;
    float2   InvBufferDimensions;
    float4x4 ProjectionInverse;
    float4x4 ReprojectionMatrix;
    float4x4 ViewProjectionInverse;
};

cbuffer cbPassData : register(b0)
{
    FFX_DNSR_Shadows_Data_Defn FFX_DNSR_Shadows_Data;
}

float4x4 FFX_DNSR_Shadows_GetViewProjectionInverse()
{
    return FFX_DNSR_Shadows_Data.ViewProjectionInverse;
}

float4x4 FFX_DNSR_Shadows_GetReprojectionMatrix()
{
    return FFX_DNSR_Shadows_Data.ReprojectionMatrix;
}

float4x4 FFX_DNSR_Shadows_GetProjectionInverse()
{
    return FFX_DNSR_Shadows_Data.ProjectionInverse;
}

float2 FFX_DNSR_Shadows_GetInvBufferDimensions()
{
    return FFX_DNSR_Shadows_Data.InvBufferDimensions;
}

int2 FFX_DNSR_Shadows_GetBufferDimensions()
{
    return FFX_DNSR_Shadows_Data.BufferDimensions;
}

int FFX_DNSR_Shadows_IsFirstFrame()
{
    return FFX_DNSR_Shadows_Data.FirstFrame;
}

float3 FFX_DNSR_Shadows_GetEye()
{
    return FFX_DNSR_Shadows_Data.Eye;
}


Texture2D<float>            DepthBuffer              : register(t0);
Texture2D<float2>           VelocityBuffer           : register(t1);
Texture2D<float3>           NormalBuffer             : register(t2);
Texture2D<float>            HistoryBuffer            : register(t3);
Texture2D<float3>           PreviousMomentsBuffer    : register(t4);
Texture2D<float>            PreviousDepthBuffer      : register(t5);
StructuredBuffer<uint>      RaytracerResult          : register(t6);

RWStructuredBuffer<uint>    TileMetaData             : register(u0);
RWTexture2D<float3>         MomentsBuffer            : register(u1); 
RWTexture2D<float2>         ReprojectionResults      : register(u2); 

SamplerState HistorySampler : register(s0);


float FFX_DNSR_Shadows_ReadDepth(int2 p)
{
    return DepthBuffer.Load(int3(p, 0)).x;
}

float FFX_DNSR_Shadows_ReadPreviousDepth(int2 p)
{
    return PreviousDepthBuffer.Load(int3(p, 0)).x;
} 

float3 FFX_DNSR_Shadows_ReadNormals(int2 p)
{
    return NormalBuffer.Load(int3(p, 0)).xyz;
}

float2 FFX_DNSR_Shadows_ReadVelocity(int2 p)
{
    return VelocityBuffer.Load(int3(p, 0));
}

float FFX_DNSR_Shadows_ReadHistory(float2 p)
{
    return HistoryBuffer.SampleLevel(HistorySampler, p, 0.0f);
}

float3 FFX_DNSR_Shadows_ReadPreviousMomentsBuffer(int2 p)
{
    return PreviousMomentsBuffer.Load(int3(p, 0)).xyz;
}

uint  FFX_DNSR_Shadows_ReadRTShadowMask(uint p)
{
    return RaytracerResult[p];
}

void  FFX_DNSR_Shadows_WriteMetadata(uint p, uint val)
{
    TileMetaData[p] = val;
}

void  FFX_DNSR_Shadows_WriteMoments(uint2 p, float3 val)
{
    MomentsBuffer[p] = val;
}

void FFX_DNSR_Shadows_WriteReprojectionResults(uint2 p, float2 val)
{
    ReprojectionResults[p] = val;
}

bool FFX_DNSR_Shadows_IsShadowReciever(uint2 p)
{
    float depth = FFX_DNSR_Shadows_ReadDepth(p);
    return (depth > 0.0f) && (depth < 1.0f);
}

#include "ffx_rtshadowdenoiser/ffx_denoiser_shadows_tileclassification.h"

#define ComputeRS \
        "CBV(b0),"\
        "DescriptorTable (SRV(t0) ),"\
        "DescriptorTable (SRV(t1) ),"\
        "DescriptorTable (SRV(t2) ),"\
        "DescriptorTable (SRV(t3) ),"\
        "DescriptorTable (SRV(t4) ),"\
        "DescriptorTable (SRV(t5) ),"\
        "DescriptorTable (SRV(t6) ),"\
        "DescriptorTable (UAV(u0) ),"\
        "DescriptorTable (UAV(u1) ),"\
        "DescriptorTable (UAV(u2) ),"\
        "StaticSampler(s0,"\
                "filter = FILTER_MIN_MAG_MIP_LINEAR,"\
                "addressU = TEXTURE_ADDRESS_CLAMP,"\
                "addressV = TEXTURE_ADDRESS_CLAMP,"\
                "addressW = TEXTURE_ADDRESS_CLAMP,"\
                "mipLODBias = 0.f,"\
                "maxAnisotropy = 1,"\
                "comparisonFunc = COMPARISON_NEVER,"\
                "minLOD = 0.f,"\
                "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,"\
                "maxLOD = 3.402823466e+38f,"\
                "space = 0,"\
                "visibility = SHADER_VISIBILITY_ALL )"

[RootSignature(ComputeRS)]
[numthreads(8, 8, 1)]
void main(uint group_index : SV_GroupIndex, uint2 gid : SV_GroupID)
{
    FFX_DNSR_Shadows_TileClassification(group_index, gid);
}
