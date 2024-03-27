//--------------------------------------------------------------------------------------
// GeometryPassVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "GeometryPassCommon.hlsli"

ConstantBuffer<SceneCB> sceneCts : register(b0);
ConstantBuffer<PerObjectCB> perObjCts : register(b1);

[RootSignature(GeometryPassRS)]
PSInterpolators main(VSInputs IN)
{
    PSInterpolators vsOut;
    vsOut.texcoords = IN.texcoords;

    float4 worldPos = mul(float4(IN.position, 1.0f), perObjCts.world);
    vsOut.position = mul(worldPos, sceneCts.viewProj);

    // Store current clip position for motion vectors.
    vsOut.currentFrameClip = vsOut.position.xyw;
                                                  
    // Previous position for motion vectors.
    float4 previousWorldPos = mul(float4(IN.position, 1.0f), perObjCts.previousWorld);
    float4 previousClipPos = mul(previousWorldPos, sceneCts.viewProj);
    vsOut.previousFrameClip = previousClipPos.xyw;

    vsOut.normals = mul(float4(IN.normals, 1.0f), perObjCts.normalTransform);

	return vsOut;
}
