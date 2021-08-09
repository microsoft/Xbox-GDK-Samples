//--------------------------------------------------------------------------------------
// ResetIgnoredParticlesCS.hlsl
//
// Compute Shader to Update all particles that have been ignored prior to this 
// point, so that their emitter location and orientation are correct.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

[RootSignature(ComputeRS)]
[numthreads(128, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    // Calcuate the index of the particle we'll be working on, based on the DispatchThreadID plus the 
    // base offset.
    uint ParticleID = ActiveCount + DispatchThreadID.x;

    // Reset the life, position, and direction values.
    g_motionData[ParticleID].RemainingLife = g_resetData[ParticleID].InitLife;
    g_motionData[ParticleID].LastPosition = EmitterPosition.xyz;

    float3 EmitDir = RotateVectorByQuaternion(EmitterRotation, g_resetData[ParticleID].Direction.xyz);
    g_motionData[ParticleID].Velocity = EmitDir * g_resetData[ParticleID].Speed;
}