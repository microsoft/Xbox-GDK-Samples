//--------------------------------------------------------------------------------------
// InstancedLightVolumesVS.hlsl
//
// Vertex Shader for sphere shaped lights (instanced).
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "InstancedLightVolumesCommon.hlsli"
#include "shared.hlsli"

ConstantBuffer<SceneConstants> sceneConsts : register(b0);

///ConstantBuffer<PerInstanceCB> PerInstance : register(b1);
StructuredBuffer<Light> lightsInScene : register(t3);

// VSFullScreenQuad
[RootSignature(ROOT_SIGNATURE_LIGHT_VOLUME)]
InstancedLightVolumeInterpolants main(LightVolumeInstancedVertex input, uint instanceId : SV_InstanceID)
{
    InstancedLightVolumeInterpolants Out = (InstancedLightVolumeInterpolants)0;
    
    float4 lightPosWorld = lightsInScene[instanceId].lightPosition;
    uint lightIndex = asuint(lightPosWorld.w);
    
    float4x4 instanceWorld =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Scale and Translation
    instanceWorld._11 = sceneConsts.lightRadius * 2.0f;
    instanceWorld._22 = sceneConsts.lightRadius * 2.0f;
    instanceWorld._33 = sceneConsts.lightRadius * 2.0f;
    instanceWorld._41 = lightPosWorld.x;
    instanceWorld._42 = lightPosWorld.y;
    instanceWorld._43 = lightPosWorld.z;
    
    float4x4 mvp = mul(instanceWorld, sceneConsts.viewProj);

    Out.LightWorldPos = lightPosWorld.xyz;
    Out.Position = mul(float4(input.Position, 1.0f), mvp);
    Out.Color = CachedColors[lightIndex % COLOR_COUNT];
    Out.Range = sceneConsts.lightRadius;

    return Out;
}
