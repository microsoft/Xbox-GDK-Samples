/** This file is part of bvhtoy library; see bvhtoy.h for version/license details */

#ifndef BVHTOY_XBOX_H
#define BVHTOY_XBOX_H

#include "bvhtoy.h"

BVHTOY_API void bvhtoyXboxConvertTrianglesAndQuadsToCanonical(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outLeafNodes, const uint32_t *quads, uint32_t quadCount, const uint32_t *indices, uint32_t triangleCount, const float *vertices, uint32_t vertexCount, uint32_t vertexStrideInFloats);
BVHTOY_API void bvhtoyXboxConvertBvh2ToCanonical(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outBvh2Nodes, D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *leafNodes, const bvhtoy_Aabb *leafAabbs, uint32_t leafNodeCount, const bvhtoy_Aabb *bvh2Aabbs, const bvhtoy_Bvh2 *bvh2Nodes, uint32_t bvh2NodeCount);
BVHTOY_API void bvhtoyXboxConvertBvh4ToCanonical(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outBvh4Nodes, D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *leafNodes, const bvhtoy_Aabb *leafBoxes, uint32_t leafNodeCount, const bvhtoy_Aabb *bvhAabbs, uint32_t bvhAabbCount, const bvhtoy_Bvh4 *bvh4Nodes, uint32_t bvh4NodeCount);

BVHTOY_API uint32_t bvhtoyXboxRunDefault(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outInternalNodes,
                                         uint32_t *inoutInternalNodeCount,
                                         D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outLeafNodes,
                                         uint32_t *inoutLeafNodeCount,
                                         const uint32_t *indices,
                                         uint32_t indexCount,
                                         const float *vertices,
                                         uint32_t vertexCount, uint32_t
                                         vertexSize,
                                         uint32_t useTriangleLeafNodesOnly);


#endif /* BVHTOY_XBOX_H */
