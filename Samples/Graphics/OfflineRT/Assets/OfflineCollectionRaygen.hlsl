//--------------------------------------------------------------------------------------
// OfflineCollectionRaygen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "OfflineCollectionCommon.hlsli"

[shader("raygeneration")]
void OfflineCollection_RayGenShader()
{
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    float2 xy = DispatchRaysIndex().xy + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), g_sceneCB.projectionViewWorld);

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
