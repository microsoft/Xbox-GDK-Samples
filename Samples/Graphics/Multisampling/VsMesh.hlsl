//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Renders a lit mesh
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

cbuffer cbTransform : register(b0)
{
    float4x4    g_mWorldViewProj;
    float4x4    g_mWorld;
};

struct VertexMesh
{
    float4 position     : POSITION0;
    float3 normal       : NORMAL0;
    float2 texcoord     : TEXCOORD0;
    float3 tangent      : TANGENT0;
    float3 binormal     : BINORMAL0;
};

struct InterpolantsMesh
{
    float4 position     : SV_POSITION0;
    float3 normal       : NORMAL0;
    float2 texcoord     : TEXCOORD0;
    float3 tangent      : TANGENT0;
    float3 binormal     : BINORMAL0;
    float3 posworld        : TEXCOORD1;
};


//-------------------------------------------------------------------------------------------------------------
// Name: VSMesh()
// Desc: Draw a mesh with textures and tangent frames
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
InterpolantsMesh main(VertexMesh In)
{
    InterpolantsMesh Out;

    Out.position = mul(In.position, g_mWorldViewProj);
    Out.texcoord = In.texcoord;
    Out.normal = mul(In.normal, (float3x3) g_mWorld);
    Out.tangent = mul(In.tangent, (float3x3) g_mWorld);
    Out.binormal = mul(In.binormal, (float3x3) g_mWorld);
    Out.posworld = mul(In.position.xyz, (float3x3) g_mWorld);

    return Out;
}
