//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct VSSceneIn
{
    float3 m_position                   : POSITION;
};

struct GSSceneIn
{
    float4 m_position                   : SV_Position;
    float3 m_worldPos                   : POSITION1;
};

struct PSSceneIn
{
    float4 m_position                   : SV_Position;
    nointerpolation float3 m_normal     : NORMAL;
};

cbuffer cbMatrix : register(b0)
{
    float4x4    g_worldViewProj;
    float4x4    g_world;
};

cbuffer cbTint : register(b1)
{
    float4      g_color;
    float3      g_lightDir;
};

#define ROOT_SIGNATURE_MESH \
    RootSignature\
    (\
       "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
        CBV(b0, visibility = SHADER_VISIBILITY_VERTEX),\
        CBV(b1, visibility = SHADER_VISIBILITY_PIXEL),"\
    )

