//--------------------------------------------------------------------------------------
// GsPointSprite.hlsl
//
// Geometry shaders for rendering dots
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

cbuffer PointSpriteTransform : register(b0)
{
    float4x4    g_mPositionTransform;
    float4x4    g_mTexcoordTransform;
};

struct InterpolantsPointSprite
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
    nointerpolation bool fragment : FRAGMENT;
};


//-------------------------------------------------------------------------------------------------------------
// Name: GSPointSprite()
// Desc: Generic expansion of a point into a textured sprite
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
[maxvertexcount(4)]
void main(point InterpolantsPointSprite In[1], inout TriangleStream<InterpolantsPointSprite> Out)
{
    if (In[0].position.x < -1.1f || In[0].position.x > 1.1f || In[0].position.y < -1.1f || In[0].position.y > 1.1f)
    {
        return; // Poor man's culling ... but does the job here.
    }

    struct VertexOffset
    {
        float2 position     : SV_POSITION0;
        float2 texcoord     : TEXCOORD0;
    } spriteOffsets[4] =
    {
        { { -0.5f, -0.5f, }, { 0.0f, 0.0f, }, },
        { { -0.5f, +0.5f, }, { 0.0f, 1.0f, }, },
        { { +0.5f, -0.5f, }, { 1.0f, 0.0f, }, },
        { { +0.5f, +0.5f, }, { 1.0f, 1.0f, }, },
    };

    for (uint i = 0; i < 4; ++i)
    {
        InterpolantsPointSprite spriteVertex;
        float scale = In[0].fragment ? 1.5f : 1.0f;
        spriteVertex.position = In[0].position + scale * float4(mul(spriteOffsets[i].position, (float2x2) g_mPositionTransform), 0.0f, 0.0f);
        spriteVertex.texcoord = In[0].texcoord + mul(spriteOffsets[i].texcoord, (float2x2) g_mTexcoordTransform);
        spriteVertex.fragment = In[0].fragment;
        Out.Append(spriteVertex);
    }

    Out.RestartStrip();
}
