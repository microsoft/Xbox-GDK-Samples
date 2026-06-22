//--------------------------------------------------------------------------------------
// OfflineCollectionHitgroup.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "OfflineCollectionCommon.hlsli"

TriangleHitGroup OfflineCollection_HitGroup =
{
    "",   		                          	// AnyHit
    "OfflineCollection_ClosestHitShader"	// ClosestHit
};

[shader("closesthit")]
void OfflineCollection_ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
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
    float4 albedoColor = 0.25f * primColors[PrimitiveIndex() % 6] + 0.75f * g_materialCB.albedo;

    float3 worldHitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    float3 pixelToLight = normalize(g_sceneCB.lightWorldPos - worldHitPosition);
    float3 worldSpaceNormal = mul(normalize(objectSpaceNormal), (float3x3)ObjectToWorld3x4());     // Assume orthogonal transform (i.e. 'N = N * M^(-1T) = N * M)
    float nDotL = max(0.0f, dot(pixelToLight, worldSpaceNormal));
    float4 diffuseColor = albedoColor * g_sceneCB.lightDiffuseColor * nDotL;

    float4 color = g_sceneCB.lightAmbientColor + diffuseColor;

    payload.color = color;
}

[shader("miss")]
void OfflineCollection_MissShader(inout RayPayload payload)
{
    float4 background = float4(0.0f, 0.2f, 0.4f, 1.0f);
    payload.color = background;
}
