//--------------------------------------------------------------------------------------
// GPassVertex.hlsl
//
// Vertex Shader for the GPass (rendering into GBuffer for deferred).
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "../shared.h"
#include "GPassCommon.hlsli"

ConstantBuffer<SceneConstants> sceneConstants   : register(b0);
ConstantBuffer<PerObjectCB> perObject           : register(b1);

[RootSignature(ROOT_SIGNATURE)]
SceneInterpolants main(in Vertex In)
{
    SceneInterpolants Out;

    // Transform to clip space
    Out.Position = mul(float4(In.Position, 1.0f), perObject.matWorld);
    Out.Position = mul(Out.Position, sceneConstants.viewProj);

    // Pass through texture UVs
    Out.UV = In.UV;

    // Transform normal and tangent into world space, and derive the binormal.
    Out.WorldNormal = mul(In.Normal, (float3x3) perObject.matWorldRotation);
    Out.WorldTangent = mul(In.Tangent, (float3x3) perObject.matWorldRotation);
    Out.WorldBinormal = cross(Out.WorldNormal, Out.WorldTangent);

    // Generate world position.
    Out.WorldPosition = mul(float4(In.Position, 1.0f), perObject.matWorld).xyz;

    return Out;
}
