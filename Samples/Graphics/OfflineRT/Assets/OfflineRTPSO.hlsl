//--------------------------------------------------------------------------------------
// OfflineRTPSO.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "OfflineRT/Common.hlsli"

TriangleHitGroup OfflineRTPSO_HitGroup =
{
    "",                             // AnyHit
    "OfflineRTPSO_ClosestHitShader" // ClosestHit
};

RaytracingShaderConfig  OfflineRTPSO_ShaderConfig =
{
    16,                             // Max payload size (sizeof(RayPayload))
    8                               // Max attribute size (sizeof(BuiltInTriangleIntersectionAttributes))
};

RaytracingPipelineConfig OfflineRTPSO_PipelineConfig =
{
    1                               // Max trace recursion depth
};

struct RayPayload
{
    float4 color;
};

[shader("raygeneration")]
void OfflineRTPSO_RayGenShader()
{
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    float2 xy = DispatchRaysIndex().xy + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(g_sceneCB.projectionViewWorld, float4(screenPos, 0, 1));

    world.xyz /= world.w;
    float3 origin = g_sceneCB.cameraWorldPos;
    float3 direction = normalize(world.xyz - origin);

    // Trace the ray
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;

    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = g_sceneCB.rayMaxLength;
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(g_tlas, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    g_rtOutput[DispatchRaysIndex().xy] = payload.color;
}

[shader("closesthit")]
void OfflineRTPSO_ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float4 primColors[] =
    {
        float4(1.0f, 0.0f, 0.0f, 1.0f),
        float4(0.0f, 1.0f, 0.0f, 1.0f),
        float4(0.0f, 0.0f, 1.0f, 1.0f),
        float4(1.0f, 1.0f, 0.0f, 1.0f),
        float4(0.0f, 1.0f, 1.0f, 1.0f),
        float4(1.0f, 0.0f, 1.0f, 1.0f)
    };

    // Load index buffer
    uint indexOffset = PrimitiveIndex() * 3 /* indices per primitive */ * 4 /* 4 bytes per index*/;
    const uint3 indices = g_indices.Load3(indexOffset);

    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 vertexNormals[3] = { g_vertices[indices[0]].normal, g_vertices[indices[1]].normal, g_vertices[indices[2]].normal };

    // Compute the triangle's interpolated normal using the hit's barycentrics
    float3 objectSpaceNormal = vertexNormals[0] + attr.barycentrics.x * (vertexNormals[1] - vertexNormals[0]) + attr.barycentrics.y * (vertexNormals[2] - vertexNormals[0]);

    // Diffuse lighting calculation
    float4 albedoColor = 0.75f * primColors[PrimitiveIndex() % 6] + 0.25f * g_materialCB.albedo;

    float3 worldHitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    float3 pixelToLight = normalize(g_sceneCB.lightWorldPos - worldHitPosition);
    float3 worldSpaceNormal = mul(normalize(objectSpaceNormal), (float3x3)ObjectToWorld3x4());     // Assume orthogonal transform (i.e. 'N = N * M^(-1T) = N * M)
    float nDotL = max(0.0f, dot(pixelToLight, worldSpaceNormal));
    float4 diffuseColor = albedoColor * g_sceneCB.lightDiffuseColor * nDotL;

    float4 color = g_sceneCB.lightAmbientColor + diffuseColor;

    payload.color = color;
}

[shader("miss")]
void OfflineRTPSO_MissShader(inout RayPayload payload)
{
    float4 background = float4(0.0f, 0.2f, 0.4f, 1.0f);
    payload.color = background;
}
