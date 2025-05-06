//--------------------------------------------------------------------------------------
// ComputeCommon.hlsli
//
// Root signature and common types, functions, and resources for particle simulation.
//
// Modifications Copyright (C) 2022. Advanced Micro Devices, Inc. All Rights Reserved.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.h"

// Rotates a vector, V, by a quaternion, Q.
float3 RotateVectorByQuaternion(float4 Q, float3 V)
{
    return V + 2.0f * cross(Q.xyz, cross(Q.xyz, V) + Q.w * V);
}

RWStructuredBuffer<ParticleMotionData> g_motionData : register(u0); // Buffer storing the current state of each particle's simulation.
RWStructuredBuffer<float4> g_particleInstance : register(u1);   //  buffer for active particles to be passed to the rendering shaders.
StructuredBuffer<ParticleResetData> g_resetData : register(t0);     // Read-only buffer used to reset properties of inactive particles.

// Common root signature for both ResetIgnoredParticlesCS & AdvanceParticlesCS shaders
#define ComputeRS \
    "CBV(b0, visibility=SHADER_VISIBILITY_ALL),\
    DescriptorTable(UAV(u0, numDescriptors=2), visibility=SHADER_VISIBILITY_ALL),\
    DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)"
