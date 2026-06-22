//--------------------------------------------------------------------------------------
// Render3DTexture.hlsli
//
// Common shader code to draw a 3D texture
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef __RENDER_3D_TEXTURE_HLSLI__
#define __RENDER_3D_TEXTURE_HLSLI__

#define Render3DTextureRS \
    "RootFlags ( DENY_VERTEX_SHADER_ROOT_ACCESS |" \
    "            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "            DENY_HULL_SHADER_ROOT_ACCESS )"

struct VS_OUT
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    uint LayerIndex : SV_RenderTargetArrayIndex;
};

struct GS_IN
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    uint LayerIndex : SV_RenderTargetArrayIndex;
};

struct PS_IN
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    uint LayerIndex : SV_RenderTargetArrayIndex;
};

#endif