//--------------------------------------------------------------------------------------
// ParticleCompute.hlsl
//
// Compute shader that updates position of particles and culls offscreen ones.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "shared.hlsli"

struct UpdateConstantsCB
{
    float4x4    g_View;
    float4      g_ViewFrustum[6];
    float4      g_ParticleData; // x = movement step, y = radii, z particle count
};
ConstantBuffer<UpdateConstantsCB> UpdateConstants : register(b0); // Not sure if used

struct ParticleMotionData
{
    float3 OrbitCenter;
    float2 Radii;
    float AngleSpeed;
    float Range;
};

StructuredBuffer<ParticleMotionData> g_MotionData   : register(t0);
RWBuffer<float> g_ParticleMotionAngles              : register(u0);
AppendStructuredBuffer<float4> g_CSLightOut         : register(u1);

[RootSignature(ComputeParticlesRS)]
[numthreads(128, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    // Get the particle index (1 particle per thread)
    uint particleIndex = DispatchThreadID.x;

    if (particleIndex >= UpdateConstants.g_ParticleData.z)
    {
        return;
    }
    
    // Get the particle info from the buffer (passed from cpu)
    ParticleMotionData md = g_MotionData[particleIndex];
    
    // Perform the update on the particle angle.
    float angle = g_ParticleMotionAngles[particleIndex];
    angle += md.AngleSpeed * UpdateConstants.g_ParticleData.x;
    g_ParticleMotionAngles[particleIndex] = angle;

    // Calculate the world-space position for the particle this frame.
    float3 particleWorld = md.OrbitCenter + float3(md.Radii.x * sin(angle), 0.0f, md.Radii.y * cos(angle));
    float4 particleView = mul(float4(particleWorld, 1.0f), UpdateConstants.g_View);

    float lightRadius = UpdateConstants.g_ParticleData.y;
    
    // Clip against frustum
    bool InFrustum = true;
    [unroll]
    for (uint i = 0; i < 6; ++i)
    {
        // If distance is negative and greater in magnitude than the radius, then light is 100% outside of fustrum
        float signedDist = dot(UpdateConstants.g_ViewFrustum[i], float4(particleView.xyz, 1.0f));
        InFrustum = InFrustum && (signedDist >= -lightRadius);
    }

    // If the light does not effect anything inside the camera frustum don't add it.
    if (!InFrustum)
    {
        return;
    }

    // We will use particle world, since view clashes with how consumers expect this data
    g_CSLightOut.Append(float4(particleWorld.xyz, asfloat(particleIndex)));
}
