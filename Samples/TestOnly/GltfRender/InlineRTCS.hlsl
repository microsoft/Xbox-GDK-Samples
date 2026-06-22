//--------------------------------------------------------------------------------------
// InlineRTCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef __XBOX_SCARLETT
#define __XBOX_ENABLE_WAVE32 1
#include "XDXRStandaloneTraverseShortStack.hlsli"
#endif
#include "Constants.h"

#define MainRS \
    "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED)," \
    "CBV(b0),"\
    "SRV(t0),"\
    "DescriptorTable(UAV(u0)),"\
    "DescriptorTable(SRV(t1)),"

RWTexture2D<float4> g_rtOutput : register(u0);
RaytracingAccelerationStructure g_tlas : register(t0);
StructuredBuffer<MeshInfo> g_meshInfo : register(t1);
ConstantBuffer<SceneConstants> g_sceneCB : register(b0);

uint3 GetIndices(uint index, uint primitiveID)
{
    Buffer<uint> indexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
    uint indexID = primitiveID * 3;
    return uint3(indexBuffer[indexID], indexBuffer[indexID + 1], indexBuffer[indexID + 2]);
}

float3 GetVertexNormal(MeshInfo currentMeshInfo, uint primitiveID, float2 barycentrics)
{
    const uint3 indices = GetIndices(currentMeshInfo.indicesIndex, primitiveID);
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(currentMeshInfo.vbIndex)];
    uint normalIndexBytes0 = currentMeshInfo.vertexStride * indices[0] + currentMeshInfo.normalOffset;
    uint normalIndexBytes1 = currentMeshInfo.vertexStride * indices[1] + currentMeshInfo.normalOffset;
    uint normalIndexBytes2 = currentMeshInfo.vertexStride * indices[2] + currentMeshInfo.normalOffset;
    float3 vertexNormals[3] = { asfloat(vertexBuffer.Load3(normalIndexBytes0)), asfloat(vertexBuffer.Load3(normalIndexBytes1)), asfloat(vertexBuffer.Load3(normalIndexBytes2)) };

    // Compute the triangle's interpolated normal using the hit's barycentrics
    float3 objectSpaceNormal = vertexNormals[0] + barycentrics.x * (vertexNormals[1] - vertexNormals[0]) + barycentrics.y * (vertexNormals[2] - vertexNormals[0]);
    return objectSpaceNormal;
}

float2 GetTexCoords(MeshInfo currentMeshInfo, uint primitiveID, float2 barycentrics)
{
    const uint3 indices = GetIndices(currentMeshInfo.indicesIndex, primitiveID);
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(currentMeshInfo.vbIndex)];
    uint texIndexBytes0 = currentMeshInfo.vertexStride * indices[0] + currentMeshInfo.texOffset;
    uint texIndexBytes1 = currentMeshInfo.vertexStride * indices[1] + currentMeshInfo.texOffset;
    uint texIndexBytes2 = currentMeshInfo.vertexStride * indices[2] + currentMeshInfo.texOffset;
    float2 vertexUVs[3] = { asfloat(vertexBuffer.Load2(texIndexBytes0)), asfloat(vertexBuffer.Load2(texIndexBytes1)), asfloat(vertexBuffer.Load2(texIndexBytes2)) };

    // Compute the triangle's interpolated UVs using the hit's barycentrics
    return vertexUVs[0] + barycentrics.x * (vertexUVs[1] - vertexUVs[0]) + barycentrics.y * (vertexUVs[2] - vertexUVs[0]);
}

float4 ShadeClosestHit(float3 worldHitPosition, uint instanceID, uint geometryID, uint primitiveID,
                       float2 barycentrics, float3x3 objectToWorld)
{
    MeshInfo currentMeshInfo = g_meshInfo[instanceID + geometryID];

    float3 objectSpaceNormal = GetVertexNormal(currentMeshInfo, primitiveID, barycentrics);

    // Diffuse lighting calculation
    Texture2D materialTex = ResourceDescriptorHeap[NonUniformResourceIndex(currentMeshInfo.texIndex)];
    float2 texCoord = GetTexCoords(currentMeshInfo, primitiveID, barycentrics);
    SamplerState texSamp = SamplerDescriptorHeap[NonUniformResourceIndex(currentMeshInfo.samplerIndex)];
    float4 albedoColor = materialTex.Sample(texSamp, texCoord);

    float3 lightDir = normalize(g_sceneCB.lightPosition - worldHitPosition);
    float3 worldSpaceNormal = normalize(mul(normalize(objectSpaceNormal), objectToWorld)); // Assume orthogonal transform (i.e. 'N = N * M^(-1T) = N * M)
    float nDotL = max(0.0f, dot(lightDir, worldSpaceNormal));
    float4 diffuseColor = g_sceneCB.lightDiffuseColor * nDotL;

    return (g_sceneCB.lightAmbientColor + diffuseColor) * albedoColor;
}

[RootSignature(MainRS)]
[numthreads(THREAD_GROUP_X, THREAD_GROUP_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    float2 xy = DTid.xy + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy * g_sceneCB.invScreenDimensions * 2.0 - 1.0;

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

#ifdef __XBOX_SCARLETT
    XboxRayQuery q;
    q.TraceRayInlineNoAHSNoIS(g_tlas, RAY_FLAG_NONE, 0xFF, ray);
#else
    RayQuery<RAY_FLAG_NONE> q;
    q.TraceRayInline(g_tlas, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES, 0xFF, ray);
#endif
    q.Proceed();

    float4 color;

    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        float3 worldHitPosition = q.WorldRayOrigin() + q.CommittedRayT() * q.WorldRayDirection();
        uint instanceID = q.CommittedInstanceID();
        uint geometryID = q.CommittedGeometryIndex();
        uint primitiveID = q.CommittedPrimitiveIndex();
        float2 barycentrics = q.CommittedTriangleBarycentrics();
        float3x4 ow = q.CommittedObjectToWorld3x4();
        // transpose the matrix
        float3x3 objectToWorld = float3x3(ow[0].x, ow[1].x, ow[2].x, ow[0].y, ow[1].y, ow[2].y, ow[0].z, ow[1].z, ow[2].z);

        color = ShadeClosestHit(worldHitPosition, instanceID, geometryID, primitiveID, barycentrics, objectToWorld);
    }
    else
    {
        color = float4(0.0f, 0.2f, 0.4f, 1.0f);
    }

    // Write the raytraced color to the output texture.
    g_rtOutput[DTid.xy] = color;
}

