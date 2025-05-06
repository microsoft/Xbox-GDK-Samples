//--------------------------------------------------------------------------------------
// SceneVS.hlsl
//
// Vertex shader for scene rendering
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

[RootSignature(SceneRS)]
SceneInterpolants main(in Vertex In)
{
    SceneInterpolants Out = (SceneInterpolants)0;

    // Transform to clip space
    Out.Position = mul(float4(In.Position, 1), g_model.WorldViewProj);

    // Pass through texture UVs
    Out.UV = In.UV;

    // Transform normal and tangent into world space, and derive the binormal.
    Out.WorldNormal = mul(In.Normal, (float3x3)g_model.WorldRot);
    Out.WorldTangent = mul(In.Tangent, (float3x3)g_model.WorldRot);
    Out.WorldBinormal = cross(Out.WorldNormal, Out.WorldTangent);

    // Generate world position.
    Out.WorldPosition = mul(float4(In.Position, 1), g_model.World).xyz;

    return Out;
}