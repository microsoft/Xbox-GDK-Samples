//--------------------------------------------------------------------------------------
// Raytracing.hlsl
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define HLSL
#include "HlslInterface.h"
#include "Helpers.hlsli"

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct RayPayload
{
    float4 color;
};

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

[shader("raygeneration")]
void MyRaygenShader()
{
    float3 rayDir;
    float3 origin;

    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(DispatchRaysIndex().xy, DispatchRaysDimensions().xy, origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = g_sceneCB.minRayExtent;
    ray.TMax = g_sceneCB.maxRayExtent;
    RayPayload payload = { float4(0.0f, 0.0f, 0.0f, 0.0f) };

    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    RenderTarget[DispatchRaysIndex().xy] = payload.color;
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr)
{
    float4 color = (InstanceID() == 2 ? g_sceneCB.pointLightColor : g_sceneCB.directionalLightColor);

    if (InstanceID() == 0) // Spinning center cube requires diffuse lighting calculations
    {
        color = g_sceneCB.directionalLightColor; // Ambient color
        float3 hitPosition = HitWorldPosition();
        const uint16_t3 indices = Indices[PrimitiveIndex()];
        
        // Retrieve corresponding vertex normals for the triangle vertices.
        float3 vertexNormals[3] = {
            Vertices[indices[0]].normal,
            Vertices[indices[1]].normal,
            Vertices[indices[2]].normal
        };

        // Compute the triangle's normal.
        // This is redundant and done for illustration purposes 
        // as all the per-vertex normals are the same and match triangle's normal in this sample. 
        float3 triangleNormal = mul(HitAttribute(vertexNormals, attr), ((float3x3)g_sceneCB.rotate));

        // Light center cube with the static and dynamic light sources.
        float4 diffuseColor = CalculateDiffuseLighting(g_cubeCB.albedo, g_sceneCB.pointLightColor, triangleNormal, normalize(g_sceneCB.lightPosition.xyz - hitPosition));
        float4 directionalLightingContribution = CalculateDiffuseLighting(g_cubeCB.albedo, g_sceneCB.directionalLightColor, triangleNormal, float3(g_sceneCB.staticLightDirection.xyz));

        color = color + directionalLightingContribution + diffuseColor;
    }

    payload.color = color;
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    payload.color = g_sceneCB.backgroundColor;
}
