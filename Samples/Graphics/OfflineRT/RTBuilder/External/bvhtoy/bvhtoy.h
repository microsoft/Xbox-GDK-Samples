/**
 * bvhtoy - version 0.01
 *
 * Copyright (C) 2024, by Microsoft Corporation.
 *
 * This library is distributed under the MIT License. See notice at the end of this file.
 */
#ifndef BVHTOY_H
#define BVHTOY_H

#include <stdint.h>

#pragma once

#ifndef BVHTOY_API
#   ifdef __cplusplus
#       define BVHTOY_API extern "C"
#   else
#       define BVHTOY_API
#   endif
#endif

#ifndef BVHTOY_WHILE0
#   define BVHTOY_WHILE0 __pragma(warning(push)) __pragma(warning(disable : 4127)) while (0) __pragma(warning(pop))
#endif

#ifndef BVHTOY_DBGBRK
#   define BVHTOY_DBGBRK __debugbreak()
#endif

#ifndef BVHTOY_ASSERT
#   if _DEBUG
#       define BVHTOY_ASSERT_MSG(expr, format, ...) do { if (!(expr)) { bvhtoyPrint("%s:%u:%s\n\t"format"\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); BVHTOY_DBGBRK; } } BVHTOY_WHILE0
#   else
#       define BVHTOY_ASSERT_MSG(expr, format, ...) (void)(expr)
#   endif
#   define BVHTOY_ASSERT(expr) BVHTOY_ASSERT_MSG(expr, #expr)
#endif

typedef void bvhtoyPrintCallback(void *userdata, const char *format, char *args);

typedef void *bvhtoyAllocate(void *userdata, uint64_t size);
typedef void bvhtoyDeallocate(void *userdata, void *memory);

BVHTOY_API void bvhtoySetPrintCallback(bvhtoyPrintCallback *printCallback, void *printUserdata);
BVHTOY_API void bvhtoySetAllocatorCallbacks(bvhtoyAllocate *allocCallback, bvhtoyDeallocate *deallocCallback, void *userdata);

BVHTOY_API void bvhtoyPrint(const char *format, ...);

BVHTOY_API void *bvhtoyAllocGeneric(uint64_t size);
BVHTOY_API void bvhtoyDeallocGeneric(void* memoryBlock);

#define bvhtoyAlloc(T, count) (T *)bvhtoyAllocGeneric(sizeof(T) * (count));
#define bvhtoyDealloc(memory) bvhtoyDeallocGeneric(memory)

typedef struct bvhtoy_Aabb
{
    float vmin[3];
    float vmax[3];
} bvhtoy_Aabb;

typedef struct bvhtoy_Centroid
{
    float v[3];
} bvhtoy_Centroid;

typedef struct bvhtoy_Bvh2
{
    uint32_t id[2];     /**< an id could be an index of leaf node or another `bvhtoy_Bvh2` node */
} bvhtoy_Bvh2;

typedef struct bvhtoy_Bvh4
{
    uint32_t id[4];     /**< an id could be an index of leaf node or another `bvhtoy_Bvh4` node */
    uint32_t boxId[4];  /**< an id of the aabb of a triangle or a `bvhtoy_Bvh4` node defined by correponding `id` index */
} bvhtoy_Bvh4;

/**
 *  @brief  This function takes triangle data defined trough an array of `indices` and `vertices` where `vertices` points to the position data,
 *          merges triangles into quads which are returned as consecutive pairs of triangle indices in `outQuadAndTriangleIndices` array,
 *          appends triangle indices which were not merged into quads at the end of `outQuadAndTriangleIndices` array,
 *          computes axis-aligned bounding boxes and centroids for returned quads and triangles
 *
 * @param[out]  outLeafAabbs                A pointer to an array of axis-aligned bounding boxes for each returned quad and triangle, must be of size `indexCount / 3`
 * @param[out]  outLeafCentroidsAabb        A pointer to an axis-aligned bounding box of all centroids
 * @param[out]  outLeafCentroids            A pointer to an array of centroids for each returned quad and triangle, must be of size `indexCount / 3`
 * @param[out]  outQuadAndTriangleIndices   A pointer to an array of triangle indices `indexCount / 3`
 * @param[in]   indices                     A pointer to an array storing vertex indices
 * @param[in]   indexCount                  The number of vertex indices
 * @param[in]   vertices                    A pointer to an array storing vertices, the first 3 floats must position data
 * @param[in]   vertexCount                 The number of vertices
 * @param[in]   vertexSizeInFloats          The size of vertex data of floats
 *
 * @return  The number of quads. This number (`N`) defines that
 *          `outAabb`, `outCentroids` store `indexCount / 3 - N` valid elements in total,
 *          `outQuadAndTriangleIndices` contains N pairs of dword-sized triangle indices (for each quad) and also `indexCount / 3 - 2 * N` triangle indices which were not merged into quads
 */
BVHTOY_API uint32_t bvhtoyGenerateLeafAabbsAndCentroidsAndQuadify(bvhtoy_Aabb *outLeafAabbs, bvhtoy_Aabb *outLeafCentroidsAabb, bvhtoy_Centroid *outLeafCentroids, uint32_t *outQuadAndTriangleIndices, const uint32_t *indices, uint32_t indexCount, const float *vertices, uint32_t vertexCount, uint32_t vertexSizeInFloats);

/**
 *  @brief  This function takes triangle data defined trough an array of `indices` and `vertices` where `vertices` points to the position data,
 *          computes axis-aligned bounding boxes and centroids for supplied triangles
 *
 * @param[out]  outAabbs                    A pointer to an array of axis-aligned bounding boxes for each returned quad and triangle, must be of size `indexCount / 3`
 * @param[out]  outCentroidsAabb            A pointer to an axis-aligned bounding box of all centroids
 * @param[out]  outCentroids                A pointer to an array of centroids for each returned quad and triangle, must be of size `indexCount / 3`
 * @param[in]   indices                     A pointer to an array storing vertex indices
 * @param[in]   indexCount                  The number of vertex indices
 * @param[in]   vertices                    A pointer to an array storing vertices, the first 3 floats must position data
 * @param[in]   vertexCount                 The number of vertices
 * @param[in]   vertexSizeInFloats          The size of vertex data of floats
 */
BVHTOY_API void bvhtoyGenerateAabbsAndCentroidsFromTriangles(bvhtoy_Aabb *outAabbs, bvhtoy_Aabb *outCentroidsAabb, bvhtoy_Centroid *outCentroids, const uint32_t *indices, uint32_t indexCount, const float *vertices, uint32_t vertexCount, uint32_t vertexSizeInFloats);

/**
 *  @brief  This function takes axis-aligned bounding boxes of leaf nodes, their centroids and produces binary bounding volume hieararchy
 *
 * @param[out]  outBvh2Aabbs                A pointer to an array of axis-aligned bounding boxes for each corresponding Bvh2 node in `outBvh2Nodes` array. The calling code must pre-allocate `leafCount-1` AABB structures
 * @param[out]  outBvh2Nodes                A pointer to an array of Bvh2 nodes which combine `leafCount` leaf nodes into a binary a binary tree. The calling code must pre-allocate `leafCount-1` Bvh2 structures
 * @param[in]   leafAabbs                   A pointer to an array of axis-aligned bounding boxes for each correspodning leaf node.
 * @param[in]   leafCentroidsAabb           A pointer to an axis-aligned bounding box of all centroids
 * @param[in]   leafCentroids               A pointer to an array of position data defining centroids for each correspodning leaf node.
 * @param[in]   leafCount                   The number of leaf nodes defining how many elements are in `leafAabbs` and `leafCentroids` input array
 */
BVHTOY_API void bvhtoyBuildBvh2(bvhtoy_Aabb *outBvh2Aabbs, bvhtoy_Bvh2 *outBvh2Nodes, const bvhtoy_Aabb *leafAabbs, const bvhtoy_Aabb *leafCentroidsAabb, const bvhtoy_Centroid *leafCentroids, uint32_t leafCount);

/**
 *  @brief  This function takes binary bounding volume hierarchy as collection of connected Bvh2 nodes and output 4-ary bounding volume hierarchy by reducing the number of internal nodes
 *
 *  @param[out] outBvh4Node                 A pointer to an array of Bvh4 nodes defining output 4-ary bounding volume hierarchy
 *  @param[in]  bvh4NodeCountMax            The number of Bvh4 nodes in `outBvh4Node` array. It must be equal to at least `leafNodeCount - 1`
 *  @param[in]  bvh2Nodes                   A pointer to an array of connected Bvh2 nodes defining input binary bounding volume hierarchy from which to build 4-ary hierarchy
 *  @param[in]  bvh2NodeCount               The number of Bvh2 nodes in `bvh2Nodes` array. This number must be equal to at least `leafNodeCount - 1`
 *  @param[in]  leafNodeCount               The number of leaf nodes Bvh2 hierarchy refers
 *
 *  @return     The number of Bvh4 nodes in resulting 4-ary bounding volume hierarchy
 */
BVHTOY_API uint32_t bvhtoyConvertBvh2IntoBvh4(bvhtoy_Bvh4 *outBvh4Node, uint32_t bvh4NodeCountMax, const bvhtoy_Bvh2 *bvh2Nodes, uint32_t bvh2NodeCount, uint32_t leafNodeCount);

#endif /** BVHTOY_H */

/**
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
