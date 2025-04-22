/**********************************************************************
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
RTShadowDenoiser
********************************************************************/
//Common helpers and IO definitions for the filter passes

struct FFX_DNSR_Shadows_Data_Defn
{
    float4x4 ProjectionInverse;
    int2     BufferDimensions;
    float2   InvBufferDimensions;
    float    DepthSimilaritySigma;
};

cbuffer cbPassData : register(b0)
{
    FFX_DNSR_Shadows_Data_Defn FFX_DNSR_Shadows_Data;
};

Texture2D              DepthBuffer  : register(t0);
Texture2D<float16_t4>  NormalBuffer : register(t1);

float2 FFX_DNSR_Shadows_GetInvBufferDimensions()
{
    return FFX_DNSR_Shadows_Data.InvBufferDimensions;
}

int2 FFX_DNSR_Shadows_GetBufferDimensions()
{
    return FFX_DNSR_Shadows_Data.BufferDimensions;
}

float4x4 FFX_DNSR_Shadows_GetProjectionInverse()
{
    return FFX_DNSR_Shadows_Data.ProjectionInverse;
}

float FFX_DNSR_Shadows_GetDepthSimilaritySigma()
{
    return FFX_DNSR_Shadows_Data.DepthSimilaritySigma;
}

float FFX_DNSR_Shadows_ReadDepth(int2 p)
{
    return DepthBuffer.Load(int3(p, 0)).x;
}

float16_t3 FFX_DNSR_Shadows_ReadNormals(int2 p)
{
    return (float16_t3)NormalBuffer.Load(int3(p, 0)).xyz;
}
 
bool FFX_DNSR_Shadows_IsShadowReciever(uint2 p)
{
    float depth = FFX_DNSR_Shadows_ReadDepth(p);
    return (depth > 0.0f) && (depth < 1.0f);
}


