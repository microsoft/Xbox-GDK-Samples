//--------------------------------------------------------------------------------------
// RenderVS.hlsl
//
// Simple vertex shader for rendering a cubic volume.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RenderCommon.hlsli"

static const float3 g_cubeVerts[] =
{
    float3(-1.0f, 1.0f, 1.0f),  
    float3(1.0f, 1.0f, -1.0f),  
    float3(-1.0f, 1.0f, -1.0f), 

    float3(1.0f, 1.0f, 1.0f),   
    float3(1.0f, 1.0f, -1.0f),  
    float3(-1.0f, 1.0f, 1.0f),  

    float3(-1.0f, 1.0f, -1.0f), 
    float3(1.0f, -1.0f, -1.0f), 
    float3(-1.0f, -1.0f, -1.0f),

    float3(1.0f, 1.0f, -1.0f),  
    float3(1.0f, -1.0f, -1.0f), 
    float3(-1.0f, 1.0f, -1.0f), 

    float3(-1.0f, 1.0f, 1.0f),  
    float3(-1.0f, -1.0f, -1.0f),
    float3(-1.0f, -1.0f, 1.0f), 

    float3(-1.0f, 1.0f, -1.0f), 
    float3(-1.0f, -1.0f, -1.0f),
    float3(-1.0f, 1.0f, 1.0f),  

    float3(1.0f, 1.0f, -1.0f),  
    float3(1.0f, -1.0f, 1.0f),  
    float3(1.0f, -1.0f, -1.0f), 

    float3(1.0f, 1.0f, 1.0f),   
    float3(1.0f, -1.0f, 1.0f),  
    float3(1.0f, 1.0f, -1.0f),  

    float3(1.0f, 1.0f, 1.0f),   
    float3(-1.0f, -1.0f, 1.0f), 
    float3(1.0f, -1.0f, 1.0f),  

    float3(-1.0f, 1.0f, 1.0f),  
    float3(-1.0f, -1.0f, 1.0f), 
    float3(1.0f, 1.0f, 1.0f),   

    float3(1.0f, -1.0f, 1.0f),  
    float3(-1.0f, -1.0f, -1.0f),
    float3(1.0f, -1.0f, -1.0f), 

    float3(-1.0f, -1.0f, 1.0f), 
    float3(-1.0f, -1.0f, -1.0f),
    float3(1.0f, -1.0f, 1.0f),  
};

[RootSignature(RenderRS)]
PS_INPUT main(uint VertexId : SV_VertexID)
{
    float4 vertex = float4(g_cubeVerts[VertexId], 1.0f);

    PS_INPUT output = (PS_INPUT)0;
    output.Pos      = mul(vertex, WorldViewProj);
    output.WorldPos = mul(vertex, World).xyz;
	
    return output;
}
