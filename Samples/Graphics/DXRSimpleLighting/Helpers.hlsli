//--------------------------------------------------------------------------------------
// Helpers.hlsli
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

RWTexture2D<float4> RenderTarget : register(u0);

RaytracingAccelerationStructure Scene : register(t0, space0);
StructuredBuffer<uint16_t3> Indices : register(t1, space0);
StructuredBuffer<Vertex> Vertices : register(t2, space0);

ConstantBuffer<SceneConstants> g_sceneCB : register(b0);
ConstantBuffer<CubeConstants> g_cubeCB : register(b1);

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
void GenerateCameraRay(uint2 index, uint2 rayDims, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / rayDims * 2.0 - 1.0; // transform from screenspace to NDC

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), g_sceneCB.projectionToWorld);

    world.xyz /= world.w;
    origin = g_sceneCB.cameraPosition.xyz;
    direction = normalize(world.xyz - origin);
}

// Diffuse lighting calculation.
float4 CalculateDiffuseLighting(float4 albedoColor, float4 lightColor, float3 normal, float3 lightDirection)
{
    return albedoColor * lightColor * saturate(dot(lightDirection, normal));
}
