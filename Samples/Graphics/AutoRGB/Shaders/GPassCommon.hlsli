//--------------------------------------------------------------------------------------
// GPassCommon.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Name: Vertex
// Desc: Vertex attributes for Geometry Pass.
//--------------------------------------------------------------------------------------
struct Vertex
{
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float2 UV : TEXCOORD0;
    float3 Tangent : TANGENT0;
};

//--------------------------------------------------------------------------------------
// Name: SceneInterpolants
// Desc: Interpolants passed to pixel shader for scene rendering.
//--------------------------------------------------------------------------------------
struct SceneInterpolants
{
    float4 Position :       SV_Position;
    float2 UV :             INT_TEXCOORD0;
    float3 WorldNormal :    INT_NORMAL0;
    float3 WorldTangent :   INT_TANGENT0;
    float3 WorldBinormal :  INT_BINORMAL0;
    float3 WorldPosition :  INT_WORLDPOS0;
};

//--------------------------------------------------------------------------------------
// Name: DeferredOut
// Desc: Render targets for Geometry Pass.
//--------------------------------------------------------------------------------------
struct DeferredOut
{
    float4 Albedo :     SV_Target0;
    float4 Normal :     SV_Target1;
    float4 Position :   SV_Target2;
};
