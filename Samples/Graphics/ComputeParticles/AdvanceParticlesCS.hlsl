//--------------------------------------------------------------------------------------
// AdvanceParticlesCS.hlsl
//
// Compute shader to advance particle physics by one frame. 128 threads per group.
// For input we only care about the DispatchThreadID, which gives us an absolute
// index from 0-NumParticles. Each thread deals with only one particle.
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
        float3 EmitDir = RotateVectorByQuaternion(EmitterRotation, g_resetData[ParticleID].Direction.xyz);

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

        // Check against the ground plane.
        if (ParticlePosition.y < g_particleRadius &&
            abs(ParticlePosition.x) <= Plane.w + g_particleRadius &&
            abs(ParticlePosition.z) <= Plane.w + g_particleRadius &&
            md.LastPosition.y >= g_particleRadius)
        {
            // Collide with plane. Bounce
            UpdatedVelocity.y = -UpdatedVelocity.y * ParticleData.y;
            ParticlePosition = md.LastPosition + UpdatedVelocity * ParticleData.x;
        }

        // Check against the spheres.
        // (Ensure we loop in this case, since otherwise, our GPR usage becomes significant and impacts performance.
        [loop]
        for (uint i = 0; i < g_sphereCount; ++i)
        {
            // Check against the spheres.
            float3 d = ParticlePosition - Spheres[i].xyz;
            float Speed = length(UpdatedVelocity);
            float3 v = UpdatedVelocity / Speed;
            if (length(d) < Spheres[i].w)
            {
                // Once we find a sphere intersection, we reflect our velocity vector in the sphere's normal, and 
                // multiply the result by our "bounciness" factor.
                float3 n = normalize(d);
                float3 reflectDir = normalize(v - (2 * dot(n, v) * n));
                UpdatedVelocity = reflectDir * Speed * ParticleData.y;
                ParticlePosition = md.LastPosition + UpdatedVelocity * ParticleData.x + g_particleRadius * n * 1.3f;

                // Don't let the bounce push the particle through the floor (it looks bad!).
                if (ParticlePosition.y < g_particleRadius)
                    ParticlePosition.y = g_particleRadius;
            }
        }

        // Store out new particle info.
        g_motionData[ParticleID].LastPosition = ParticlePosition;
        g_motionData[ParticleID].Velocity = UpdatedVelocity;
        g_motionData[ParticleID].RemainingLife = Life;

        // Set the final particle position for this frame.
        Instance.xyz = ParticlePosition.xyz;
    }

    // Frustum cull the particle.
    // Now, this isn't a huge performance win (if at all), but it serves to demonstrate the benefits of using 
    // AppendStructuredBuffer in this case. We can cull geometry in the compute shader (fast!) and have a variable size
    // buffer we use for instancing later in the pipeline. 
    bool inFrustum = true;
    for (uint i = 0; i < 6; ++i)
    {
        float distance = dot(ViewFrustum[i], float4(Instance.xyz, 1));
        inFrustum = inFrustum && (distance >= -g_particleRadius);
    }

    // If the particle is visible, add to our AppendStructuredBuffer.
    if (inFrustum)
    {
        // Make sure we store out the instance data, and append to our AppendStructuredBuffer.
        g_particleInstance.Append(Instance);
    }
}