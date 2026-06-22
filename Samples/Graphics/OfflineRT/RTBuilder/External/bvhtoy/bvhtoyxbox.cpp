/** This file is part of bvhtoy library; see bvhtoy.h for version/license details */

#include <d3d12_xs.h>
#include "bvhtoyxbox.h"

const unsigned GEmptyVertex[] = { 0x7fc00000, 0x7fc00000, 0x7fc00000 };

void bvhtoyXboxConvertTrianglesAndQuadsToCanonical(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outLeafNodes, const uint32_t *quads, uint32_t quadCount, const uint32_t *indices, uint32_t triangleCount, const float *vertices, uint32_t vertexCount, uint32_t vertexStrideInFloats)
{
    for (uint32_t i = 0; i < quadCount; ++i)
    {
        const uint32_t t0 = quads ? quads[i * 2 + 0] : (i * 2 + 0), t1 = quads ? quads[i * 2 + 1] : (i * 2 + 1);

        const uint32_t t0indices[] = { indices[t0 * 3 + 1], indices[t0 * 3 + 2], indices[t0 * 3 + 0], indices[t0 * 3 + 1], indices[t0 * 3 + 2] };
        const uint32_t t1indices[] = { indices[t1 * 3 + 1], indices[t1 * 3 + 2], indices[t1 * 3 + 0], indices[t1 * 3 + 1], indices[t1 * 3 + 2] };

        uint32_t rotA, rotB;
        for (rotB = 0; rotB < 3; ++rotB)
            for (rotA = 0; rotA < 3; ++rotA)
                if (t0indices[3 - rotA] == t1indices[2 - rotB] && t0indices[4 - rotA] == t1indices[4 - rotB])
                    goto foundRotation;

        BVHTOY_ASSERT(!"Unreachable");

    foundRotation:

        const uint32_t a = t0indices[2 - rotA];
        const uint32_t b = t0indices[3 - rotA];
        const uint32_t c = t0indices[4 - rotA];
        const uint32_t d = t1indices[3 - rotB];

        outLeafNodes[i].Type = D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE_TYPE_QUAD;

        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[0], vertices + a * vertexStrideInFloats, sizeof(float) * 3);
        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[1], vertices + b * vertexStrideInFloats, sizeof(float) * 3);
        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[2], vertices + c * vertexStrideInFloats, sizeof(float) * 3);
        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[3], vertices + d * vertexStrideInFloats, sizeof(float) * 3);
        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[4], GEmptyVertex, sizeof(float) * 3);

        memset(&outLeafNodes[i].TriangleNodeData.PrimitiveIndices[0], 0xff, sizeof(outLeafNodes[i].TriangleNodeData.PrimitiveIndices));
        memset(&outLeafNodes[i].TriangleNodeData.RemapIndices[0], 0x00, sizeof(outLeafNodes[i].TriangleNodeData.RemapIndices));

        outLeafNodes[i].TriangleNodeData.GeometryIndex = 0;

        static const uint8_t rotationToRemapIndices[] = { 2, 0, 1, 2 };

        const uint32_t indicesA[] = { b, c, a, b, c };
        const uint32_t indicesB[] = { d, c, b, d, c };

        for (rotA = 0; rotA < 3; ++rotA)
            if (indicesA[2 - rotA] == indices[t0 * 3 + 0] && indicesA[3 - rotA] == indices[t0 * 3 + 1] && indicesA[4 - rotA] == indices[t0 * 3 + 2])
                break;

        for (rotB = 0; rotB < 3; ++rotB)
            if (indicesB[2 - rotB] == indices[t1 * 3 + 0] && indicesB[3 - rotB] == indices[t1 * 3 + 1] && indicesB[4 - rotB] == indices[t1 * 3 + 2])
                break;

        outLeafNodes[i].TriangleNodeData.RemapIndices[0][0] = rotationToRemapIndices[2 - rotA];
        outLeafNodes[i].TriangleNodeData.RemapIndices[0][1] = rotationToRemapIndices[3 - rotA];

        outLeafNodes[i].TriangleNodeData.RemapIndices[1][0] = rotationToRemapIndices[2 - rotB];
        outLeafNodes[i].TriangleNodeData.RemapIndices[1][1] = rotationToRemapIndices[3 - rotB];

        outLeafNodes[i].TriangleNodeData.PrimitiveIndices[0] = t0;
        outLeafNodes[i].TriangleNodeData.PrimitiveIndices[1] = t1;

    }

    /** convert triangles */
    uint32_t unmergedTriangleCount = triangleCount - quadCount * 2;

    /** offset the destination pointer by the number of quads */
    outLeafNodes += quadCount;

    for (uint32_t i = 0; i < unmergedTriangleCount; ++i)
    {
        const uint32_t ti = quads ? quads[quadCount * 2 + i] : (quadCount * 2 + i);

        const uint32_t a = indices[ti * 3 + 0], b = indices[ti * 3 + 1], c = indices[ti * 3 + 2];
        BVHTOY_ASSERT(a < vertexCount && b < vertexCount && c < vertexCount);

        outLeafNodes[i].Type = D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE_TYPE_TRIANGLE;

        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[0], vertices + a * vertexStrideInFloats, sizeof(float) * 3);
        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[1], vertices + b * vertexStrideInFloats, sizeof(float) * 3);
        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[2], vertices + c * vertexStrideInFloats, sizeof(float) * 3);
        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[3], GEmptyVertex, sizeof(float) * 3);
        memcpy(&outLeafNodes[i].TriangleNodeData.VertexPositions[4], GEmptyVertex, sizeof(float) * 3);

        memset(&outLeafNodes[i].TriangleNodeData.PrimitiveIndices[0], 0xff, sizeof(outLeafNodes[i].TriangleNodeData.PrimitiveIndices));
        memset(&outLeafNodes[i].TriangleNodeData.RemapIndices[0], 0x00, sizeof(outLeafNodes[i].TriangleNodeData.RemapIndices));

        outLeafNodes[i].TriangleNodeData.GeometryIndex = 0;

        // NOTE: for a single triangle baryI => Vtx 1, BaryJ => Vtx 2
        outLeafNodes[i].TriangleNodeData.RemapIndices[0][0] = 1;
        outLeafNodes[i].TriangleNodeData.RemapIndices[0][1] = 2;

        outLeafNodes[i].TriangleNodeData.PrimitiveIndices[0] = ti;
    }
}

void bvhtoyXboxConvertBvh2ToCanonical(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outBvh2Nodes, D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *leafNodes, const bvhtoy_Aabb *leafAabbs, uint32_t leafNodeCount, const bvhtoy_Aabb *bvh2Aabbs, const bvhtoy_Bvh2 *bvh2Nodes, uint32_t bvh2NodeCount)
{
    for (uint32_t i = 0; i < bvh2NodeCount; ++i)
    {
        D3D12XBOX_RAYTRACING_CANONICAL_FP32_AABB_DATA* data = &outBvh2Nodes[i].FP32AABBData;

        BVHTOY_ASSERT(i < bvh2NodeCount);

        uint32_t childIdA = bvh2Nodes[i].id[0];
        uint32_t childIdB = bvh2Nodes[i].id[1];

        outBvh2Nodes[i].Type = D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE_TYPE_FP32_AABB;

        data->ChildNodes[0] = NULL;
        data->ChildNodes[1] = NULL;
        data->ChildNodes[2] = NULL;
        data->ChildNodes[3] = NULL;

        data->UnreservedSpace[0] = 0;
        data->UnreservedSpace[1] = 0;
        data->UnreservedSpace[2] = 0;
        data->UnreservedSpace[3] = 0;

        data->TriangleIndices[0] = 0xff;
        data->TriangleIndices[1] = 0xff;
        data->TriangleIndices[2] = 0xff;
        data->TriangleIndices[3] = 0xff;

        if (childIdA < leafNodeCount)
        {
            data->TriangleIndices[0] = 0;
            data->ChildNodes[0] = leafNodes + childIdA;
            uint32_t type = data->ChildNodes[0]->Type;
            BVHTOY_ASSERT(type == D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE_TYPE_TRIANGLE || type == D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE_TYPE_QUAD);
            memcpy(&data->AABBs[0], &leafAabbs[childIdA], sizeof(bvhtoy_Aabb));
        }
        else
        {
            childIdA -= leafNodeCount;
            BVHTOY_ASSERT(childIdA < i);
            data->ChildNodes[0] = outBvh2Nodes + childIdA;
            memcpy(&data->AABBs[0], &bvh2Aabbs[childIdA], sizeof(bvhtoy_Aabb));
        }

        if (childIdB < leafNodeCount)
        {
            data->TriangleIndices[1] = 0;
            data->ChildNodes[1] = leafNodes + childIdB;
            uint32_t type = data->ChildNodes[1]->Type;
            BVHTOY_ASSERT(type == D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE_TYPE_TRIANGLE || type == D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE_TYPE_QUAD);
            memcpy(&data->AABBs[1], &leafAabbs[childIdB], sizeof(bvhtoy_Aabb));
        }
        else
        {
            childIdB -= leafNodeCount;
            BVHTOY_ASSERT(childIdB < i);
            data->ChildNodes[1] = outBvh2Nodes + childIdB;
            memcpy(&data->AABBs[1], &bvh2Aabbs[childIdB], sizeof(bvhtoy_Aabb));
        }
    }
}

void bvhtoyXboxConvertBvh4ToCanonical(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outInternalNodes, D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *leafNodes, const bvhtoy_Aabb *leafBoxes, uint32_t leafNodeCount, const bvhtoy_Aabb *bvhAabbs, uint32_t bvhAabbCount, const bvhtoy_Bvh4 *bvh4Nodes, uint32_t bvh4NodeCount)
{
    for (uint32_t i = 0; i < bvh4NodeCount; ++i)
    {
        const uint32_t* merge4OpIds = bvh4Nodes[i].id;
        const uint32_t* boxIds = bvh4Nodes[i].boxId;

        outInternalNodes[i].Type = D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE_TYPE_FP32_AABB;

        D3D12XBOX_RAYTRACING_CANONICAL_FP32_AABB_DATA* data = &outInternalNodes[i].FP32AABBData;

        data->UnreservedSpace[0] = 0;
        data->UnreservedSpace[1] = 0;
        data->UnreservedSpace[2] = 0;
        data->UnreservedSpace[3] = 0;

        data->TriangleIndices[0] = 0xff;
        data->TriangleIndices[1] = 0xff;
        data->TriangleIndices[2] = 0xff;
        data->TriangleIndices[3] = 0xff;

        for (uint32_t j = 0; j < 4; ++j)
        {
            uint32_t id = merge4OpIds[j];

            if (id != ~0)
            {
                const uint32_t boxId = boxIds[j];
                if (id < leafNodeCount)
                {
                    BVHTOY_ASSERT(boxId < leafNodeCount);
                    memcpy(&data->AABBs[j], &leafBoxes[boxId], sizeof(bvhtoy_Aabb));

                    data->TriangleIndices[j] = 0;
                    data->ChildNodes[j] = leafNodes + id;
                }
                else
                {
                    BVHTOY_ASSERT(boxId < bvhAabbCount);
                    memcpy(&data->AABBs[j], &bvhAabbs[boxId], sizeof(bvhtoy_Aabb));

                    id -= leafNodeCount;
                    BVHTOY_ASSERT(id < bvh4NodeCount);
                    data->ChildNodes[j] = outInternalNodes + id;
                }
            }
            else
            {
                data->ChildNodes[j] = NULL;
            }
        }
    }
}

static uint32_t validateBvh4CanonicalNodes(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE* root, D3D12_RAYTRACING_AABB* aabb)
{
    if (root->Type == D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE_TYPE_FP32_AABB)
    {
        D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE** childNodes = root->FP32AABBData.ChildNodes;
        D3D12_RAYTRACING_AABB* aabbs = root->FP32AABBData.AABBs;

        uint32_t sum = 0;
        for (uint32_t i = 0; i < 4; ++i)
        {
            if (childNodes[i])
            {
                if (aabb)
                {
                    BVHTOY_ASSERT(aabb->MinX <= aabbs[i].MinX && aabb->MinY <= aabbs[i].MinY && aabb->MinZ <= aabbs[i].MinZ);
                    BVHTOY_ASSERT(aabb->MaxX >= aabbs[i].MaxX && aabb->MaxY >= aabbs[i].MaxY && aabb->MaxZ >= aabbs[i].MaxZ);
                }

                sum += validateBvh4CanonicalNodes(childNodes[i], &aabbs[i]);
            }
        }
        return sum;
    }
    else
    {
        return 1;
    }
}

uint32_t bvhtoyXboxRunDefault(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outInternalNodes,
                              uint32_t *inoutInternalNodeCount,
                              D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outLeafNodes,
                              uint32_t *inoutLeafNodeCount,
                              const uint32_t *indices,
                              uint32_t indexCount,
                              const float *vertices,
                              uint32_t vertexCount, uint32_t
                              vertexSize,
                              uint32_t useTriangleLeafNodesOnly)
{
    const uint32_t triangleCount = indexCount / 3;
    BVHTOY_ASSERT(indexCount % 3 == 0);

    const uint32_t vertexStrideInFloats = vertexSize / sizeof(float);
    BVHTOY_ASSERT(vertexSize % sizeof(float) == 0);

    /** assume that the number of leaf nodes is the number of triangles and adjust it leater */
    uint32_t leafCount = triangleCount;

    bvhtoy_Aabb *leafAabbs = bvhtoyAlloc(bvhtoy_Aabb, leafCount);
    bvhtoy_Centroid *leafCentroids = bvhtoyAlloc(bvhtoy_Centroid, leafCount);

    uint32_t* quads = NULL;
    uint32_t quadCount = 0;

    bvhtoy_Aabb leafCentroidsAabb;
    if (useTriangleLeafNodesOnly == 0)
    {
        quads = bvhtoyAlloc(uint32_t, triangleCount);
        quadCount = bvhtoyGenerateLeafAabbsAndCentroidsAndQuadify(leafAabbs, &leafCentroidsAabb, leafCentroids, quads, indices, indexCount, vertices, vertexCount, vertexStrideInFloats);
        leafCount -= quadCount;

    }
    else
    {
        bvhtoyGenerateAabbsAndCentroidsFromTriangles(leafAabbs, &leafCentroidsAabb, leafCentroids, indices, indexCount, vertices, vertexCount, vertexStrideInFloats);
    }

    bvhtoyXboxConvertTrianglesAndQuadsToCanonical(outLeafNodes, quads, quadCount, indices, triangleCount, vertices, vertexCount, vertexStrideInFloats);

    const uint32_t bvh2NodeCount = leafCount - 1;

    bvhtoy_Aabb *bvh2Aabbs = bvhtoyAlloc(bvhtoy_Aabb, bvh2NodeCount);
    bvhtoy_Bvh2 *bvh2Nodes = bvhtoyAlloc(bvhtoy_Bvh2, bvh2NodeCount);

    bvhtoyBuildBvh2(bvh2Aabbs, bvh2Nodes, leafAabbs, &leafCentroidsAabb, leafCentroids, leafCount);

#define USE_BVH4 1
#if USE_BVH4
    /**TODO: need tighter bound here? */
    const uint32_t bvh4CountMax = bvh2NodeCount;

    bvhtoy_Bvh4 *bvh4Nodes = bvhtoyAlloc(bvhtoy_Bvh4, bvh4CountMax);

    const uint32_t bvh4NodeCount = bvhtoyConvertBvh2IntoBvh4(bvh4Nodes, bvh4CountMax, bvh2Nodes, bvh2NodeCount, leafCount);
    bvhtoyXboxConvertBvh4ToCanonical(outInternalNodes, outLeafNodes, leafAabbs, leafCount, bvh2Aabbs, bvh2NodeCount, bvh4Nodes, bvh4NodeCount);

    bvhtoyDealloc(bvh4Nodes);

    BVHTOY_ASSERT(bvh4NodeCount <= *inoutInternalNodeCount);
    *inoutInternalNodeCount = bvh4NodeCount;

    const uint32_t rootNodeIndex = 0;

#if _DEBUG
    uint32_t validatedLeafCount = validateBvh4CanonicalNodes(outInternalNodes, NULL);
    BVHTOY_ASSERT(validatedLeafCount == leafCount);
#endif

#else
    bvhtoyXboxConvertBvh2ToCanonical(outInternalNodes, outLeafNodes, leafAabbs, leafCount, bvh2Aabbs, bvh2Nodes, bvh2NodeCount);

    BVHTOY_ASSERT(bvh2NodeCount <= *inoutInternalNodeCount);
    *inoutInternalNodeCount = bvh2NodeCount;

    const uint32_t rootNodeIndex = bvh2NodeCount - 1;
#endif

    BVHTOY_ASSERT(leafCount <= *inoutLeafNodeCount);
    *inoutLeafNodeCount = leafCount;

    bvhtoyDealloc(bvh2Nodes);
    bvhtoyDealloc(bvh2Aabbs);

    if (quads != NULL)
        bvhtoyDealloc(quads);

    bvhtoyDealloc(leafCentroids);
    bvhtoyDealloc(leafAabbs);

    return rootNodeIndex;
}
