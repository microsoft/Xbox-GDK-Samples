//--------------------------------------------------------------------------------------
// AdvanceParticlesCS.hlsl
//
// Modifications Copyright (C) 2022. Advanced Micro Devices, Inc. All Rights Reserved.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

[RootSignature(ComputeRS)]
[numthreads(128, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    // Grab the particle ID. Each thread works on one particle. We use the Dispatch Thread ID
    // to give us the particle index.
    uint ParticleID = DispatchThreadID.x;

    // Grab the current state of the particle
    ParticleMotionData md = g_motionData[ParticleID];

    // Update the life value.
    float Life = md.RemainingLife + ParticleData.x;
    float4 Instance;

    // Grab the particle's lifetime
    float Lifetime = g_resetData[ParticleID].AllottedLife;

    // The particle instance value's w-component stores the particle's life as a normalized
    // value from 1 (fully alive) to 0 (expired).
    Instance.w = min(1, (Lifetime - Life) / Lifetime);

    // If the particle is done...
    if (Life > Lifetime)
    {
        // Reset the particle's data to the initial state....
        // First, save out the start position (the emitter position)
        g_motionData[ParticleID].LastPosition = EmitterPosition.xyz;

        // Calculate the initial direction by rotating the default direction by the quaternion containing the 
        // emitter direction.
        float3 EmitDir = float3(0.0f, -1.0f, 0.0f) + g_resetData[ParticleID].Direction.xyz;

        // Now the velocity is just that direction times the default (initial) speed.
        g_motionData[ParticleID].Velocity = EmitDir * g_resetData[ParticleID].Speed;

        // Restore life value to zero - this particle is starting over.
        g_motionData[ParticleID].RemainingLife = 0;
        Instance.xyz = EmitterPosition.xyz;
    }
    else
    {
        // Update the velocity by apply "gravity". 
        float3 UpdatedVelocity = md.Velocity - float3(0, g_gravitationalConstant * g_motionData[ParticleID].Mass * ParticleData.x, 0);

        // Update the position based on the new velocity
        float3 ParticlePosition = md.LastPosition + UpdatedVelocity * ParticleData.x; // = frame time

        // Check against the ground plane
        if (ParticlePosition.y < Plane.y)
        {
            // Collide with plane. Bounce
            UpdatedVelocity.y = -UpdatedVelocity.y * ParticleData.y * 0.025f;
            ParticlePosition = md.LastPosition + UpdatedVelocity * ParticleData.x;
            Instance.w = 0.005f;
        }

        // Store out new particle info.
        g_motionData[ParticleID].LastPosition = ParticlePosition;
        g_motionData[ParticleID].Velocity = UpdatedVelocity;
        g_motionData[ParticleID].RemainingLife = Life;

        // Set the final particle position for this frame.
        Instance.xyz = ParticlePosition.xyz;
    }

    g_particleInstance[ParticleID] = Instance;
}
