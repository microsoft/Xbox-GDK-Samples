//--------------------------------------------------------------------------------------
// RaytracingInline.hlsl
//
// Inline raytraced variant of Raytracing.hlsl.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define HLSL

#include "HlslInterface.h"
#include "Helpers.hlsli"
#include "XDXRStandaloneTraverseShortStack.hlsli"

// Root Parameters:
// 0) Raytracing output
// 1) Scene constant buffer
// 2) Cube constant buffer
// 3) Constant buffer holding screen dimensions 
// 4) Acceleration structure
// 5) Vertex and index buffers

#define InlineRT \
	"DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
	"CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
	"CBV(b1, visibility=SHADER_VISIBILITY_ALL),"\
    "CBV(b2, visibility=SHADER_VISIBILITY_ALL),"\
	"SRV(t0, visibility=SHADER_VISIBILITY_ALL),"\
	"DescriptorTable(SRV(t1, numDescriptors=2), visibility=SHADER_VISIBILITY_ALL),"\

ConstantBuffer<ScreenConstants> g_screenResolution : register(b2);

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], float2 barycentrics)
{
    return vertexAttribute[0] +
        barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

[RootSignature(InlineRT)]
[numthreads(THREAD_GROUP_X, THREAD_GROUP_Y, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    float3 rayDir;
    float3 origin;

    XboxRayQuery xrq;
    
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(DTid.xy, uint2(g_screenResolution.dimensions.xy), origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = g_sceneCB.minRayExtent;
    ray.TMax = g_sceneCB.maxRayExtent;

    // Note: XboxRayQuery does not support RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES.
    // The GDK documentation recommends manually separating TLASes into different batches.
    xrq.TraceRayInlineNoAHSNoIS(
        Scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xFF,
        ray);

    xrq.Proceed();

    // Miss shading is done by default.
    float4 color = g_sceneCB.backgroundColor;

    if (xrq.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        // Closest hit shading code
        color = (xrq.CommittedInstanceIndex() == 2 ? g_sceneCB.pointLightColor : g_sceneCB.directionalLightColor);

        if (xrq.CommittedInstanceIndex() == 0) // Spinning center cube requires diffuse lighting calculations
        {
            color = g_sceneCB.directionalLightColor; // Ambient color
            float3 hitPosition = xrq.WorldRayOrigin() + xrq.CommittedRayT() * xrq.WorldRayDirection();
            const uint16_t3 indices = Indices[xrq.CommittedPrimitiveIndex()];

            // Retrieve corresponding vertex normals for the triangle vertices.
            float3 vertexNormals[3] =
            {
                Vertices[indices[0]].normal,
                Vertices[indices[1]].normal,
                Vertices[indices[2]].normal
            };

            // Compute the triangle's normal.
            // This is redundant and done for illustration purposes 
            // as all the per-vertex normals are the same and match triangle's normal in this sample. 
            float3 triangleNormal = mul(HitAttribute(vertexNormals, xrq.CommittedTriangleBarycentrics()), ((float3x3) g_sceneCB.rotate));

            // Light center cube with the static and dynamic light sources.
            float4 diffuseColor = CalculateDiffuseLighting(g_cubeCB.albedo, g_sceneCB.pointLightColor, triangleNormal, normalize(g_sceneCB.lightPosition.xyz - hitPosition));
            float4 directionalLightingContribution = CalculateDiffuseLighting(g_cubeCB.albedo, g_sceneCB.directionalLightColor, triangleNormal, float3(g_sceneCB.staticLightDirection.xyz));

            color = color + directionalLightingContribution + diffuseColor;
        }
    }

    RenderTarget[DTid.xy] = color;
}
