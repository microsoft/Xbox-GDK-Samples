/** This file is part of bvhtoy library; see bvhtoy.h for version/license details */

#include <math.h> /** modff only */
#include <memory.h> /** memset only */
#include <stdarg.h>

#include "bvhtoy.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

#ifndef BVHTOY_USE_TRACE
#define BVHTOY_USE_TRACE 0
#endif

static const float kFltMax = 3.402823466e+38f;

static bvhtoyPrintCallback *GbvhtoyPrintCallback = NULL;
static void *GbvhtoyPrintUserdata = NULL;

void bvhtoySetPrintCallback(bvhtoyPrintCallback *printCallback, void *printUserdata)
{
    GbvhtoyPrintCallback = printCallback;
    GbvhtoyPrintUserdata = printUserdata;
}

static bvhtoyAllocate *GbvhtoyAllocCallback = NULL;
static bvhtoyDeallocate *GbvhtoyDeallocCallback = NULL;
static void *GbvhtoyAllocDeallocUserdata = NULL;

void bvhtoySetAllocatorCallbacks(bvhtoyAllocate *allocCallback, bvhtoyDeallocate *deallocCallback, void *userdata)
{
    GbvhtoyAllocCallback = allocCallback;
    GbvhtoyDeallocCallback = deallocCallback;
    GbvhtoyAllocDeallocUserdata = userdata;
}

void *bvhtoyAllocGeneric(uint64_t size)
{
    return GbvhtoyAllocCallback(GbvhtoyAllocDeallocUserdata, size);
}

void bvhtoyDeallocGeneric(void* memoryBlock)
{
    GbvhtoyDeallocCallback(GbvhtoyAllocDeallocUserdata, memoryBlock);
}

void bvhtoyPrint(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (GbvhtoyPrintCallback != NULL)
        GbvhtoyPrintCallback(GbvhtoyPrintUserdata, format, args);

    va_end(args);
}

#ifndef BVHTOY_TRACE
#   if BVHTOY_USE_TRACE
        BVHTOY_API void QueryPerformanceFrequency(uint64_t *frequency);
        BVHTOY_API void QueryPerformanceCounter(uint64_t *counter);

        uint64_t TimerRate(void) { uint64_t Rate; QueryPerformanceFrequency(&Rate); return Rate; }
        uint64_t TimerMark(void) { uint64_t Mark; QueryPerformanceCounter(&Mark); return Mark; }

        uint64_t CyclesToNs(uint64_t Cycles, uint64_t Rate) { return Cycles * 1000000000ull / Rate; }
        uint64_t CyclesToUs(uint64_t Cycles, uint64_t Rate) { return Cycles * 1000000ull / Rate; }
        uint64_t CyclesToMs(uint64_t Cycles, uint64_t Rate) { return Cycles * 1000ull / Rate; }

        uint64_t TimerNs(uint64_t StartMark, uint64_t Rate) { return CyclesToNs(TimerMark() - StartMark, Rate); }
        uint64_t TimerUs(uint64_t StartMark, uint64_t Rate) { return CyclesToUs(TimerMark() - StartMark, Rate); }
        uint64_t TimerMs(uint64_t StartMark, uint64_t Rate) { return CyclesToMs(TimerMark() - StartMark, Rate); }

        static uint32_t GbvhtoyTraceScopeLevel = 0;
        static uint64_t GbvhtoyTraceTimerMark = 0;
        static uint64_t GbvhtoyTraceTimerRate = 0;

#       define BVHTOY_BEGIN_TRACE_SCOPED(fmt, ...)      \
            do                                          \
            {                                           \
                GbvhtoyTraceTimerMark = TimerMark();    \
                GbvhtoyTraceTimerRate = TimerRate();    \
                bvhtoyPrint("%*s "fmt"...\n", 7 + GbvhtoyTraceScopeLevel * 4, "[Start]", __VA_ARGS__);\
                ++GbvhtoyTraceScopeLevel

#       define BVHTOY_END_TRACE_SCOPED()            \
                --GbvhtoyTraceScopeLevel;           \
                bvhtoyPrint("%*s took = %llu ms\n", 8 + GbvhtoyTraceScopeLevel * 4, "[Finish]", TimerMs(GbvhtoyTraceTimerMark, GbvhtoyTraceTimerRate));\
            }                                       \
            BVHTOY_WHILE0

#       define BVHTOY_BEGIN_TRACE(fmt, ...)         \
            do                                      \
            {                                       \
                GbvhtoyTraceTimerMark = TimerMark();\
                GbvhtoyTraceTimerRate = TimerRate();\
                bvhtoyPrint("%*s"fmt"...", GbvhtoyTraceScopeLevel * 4, "", __VA_ARGS__);

#       define BVHTOY_END_TRACE()                   \
                bvhtoyPrint(" took = %llu ms\n", TimerMs(GbvhtoyTraceTimerMark, GbvhtoyTraceTimerRate));\
            }                                       \
            BVHTOY_WHILE0

#       define BVHTOY_TRACE(fmt, ...)               \
            bvhtoyPrint("%*s"fmt"\n", GbvhtoyTraceScopeLevel * 4, "", __VA_ARGS__)

#   else
#       define BVHTOY_BEGIN_TRACE(fmt, ...)         \
            do                                      \
            {                                       \

#       define BVHTOY_END_TRACE()                   \
            }                                       \
            BVHTOY_WHILE0

#       define BVHTOY_BEGIN_TRACE_SCOPED BVHTOY_BEGIN_TRACE
#       define BVHTOY_END_TRACE_SCOPED BVHTOY_END_TRACE
#       define BVHTOY_TRACE(fmt, ...)

#   endif

#endif

static inline void accumulateSahFromExtent(uint32_t *inoutSahU32, float *inoutSahF32, float extent[3])
{
    const float sah = extent[0] * extent[1] +
                      extent[1] * extent[2] +
                      extent[2] * extent[0];

    // extract fractional and integer part of SAH
    float sahU32 = 0.0;
    float sahF32 = modff(sah, &sahU32);

    // accumulate integer part
    *inoutSahU32 += (uint32_t)sahU32;

    // accumulate fractional part
    *inoutSahF32 = modff(*inoutSahF32 + sahF32, &sahU32);

    // accumulate integer part once more to account for potential non-zero integer part from fractional accumulation
    *inoutSahU32 += (uint32_t)sahU32;
}

static uint32_t hashBuckets(uint32_t count)
{
    uint32_t buckets = 1;
    while (buckets < count + count / 4)
        buckets *= 2;

    return buckets;
}

#if 0
template <typename T, typename Hash>
static T *hashLookup(T *table, size_t buckets, const Hash & hash, const T & key, const T & empty)
{
    BVHTOY_ASSERT(buckets > 0);
    BVHTOY_ASSERT((buckets & (buckets - 1)) == 0);

    size_t hashmod = buckets - 1;
    size_t bucket = hash.hash(key) & hashmod;

    for (size_t probe = 0; probe <= hashmod; ++probe)
    {
        T& item = table[bucket];

        if (item == empty)
            return &item;

        if (hash.equal(item, key))
            return &item;

        // hash collision, quadratic probing
        bucket = (bucket + probe + 1) & hashmod;
    }

    BVHTOY_ASSERT(false && "Hash table is full"); // unreachable
    return NULL;
}
#else

#define hashLookup(outIndex, table, tableElemCount, hasherState, hasherHash, hasherEqual, key, empty)   \
    do                                                                                                  \
    {                                                                                                   \
        BVHTOY_ASSERT(tableElemCount > 0);                                                              \
        BVHTOY_ASSERT((tableElemCount & (tableElemCount - 1)) == 0);                                    \
                                                                                                        \
        uint32_t hashmod = tableElemCount - 1;                                                          \
        uint32_t bucket = hasherHash(hasherState, key) & hashmod;                                       \
                                                                                                        \
        for (uint32_t probe = 0; probe <= hashmod; ++probe)                                             \
        {                                                                                               \
            if ((table[bucket] == empty) || hasherEqual(hasherState, table[bucket], key))               \
            {                                                                                           \
                outIndex = bucket;                                                                      \
                break;                                                                                  \
            }                                                                                           \
                                                                                                        \
            /** hash collision, quadratic probing */                                                    \
            bucket = (bucket + probe + 1) & hashmod;                                                    \
        }                                                                                               \
    }                                                                                                   \
    BVHTOY_WHILE0

#endif

void bvhtoyGenerateAabbsAndCentroidsFromTriangles(bvhtoy_Aabb *outAabbs, bvhtoy_Aabb *outCentroidsAabb, bvhtoy_Centroid *outCentroids, const uint32_t *indices, uint32_t indexCount, const float *vertices, uint32_t vertexCount, uint32_t vertexSizeInFloats)
{
    uint32_t triCount = indexCount / 3;

    float gminx = kFltMax, gminy = kFltMax, gminz = kFltMax;
    float gmaxx = -kFltMax, gmaxy = -kFltMax, gmaxz = -kFltMax;
    for (size_t i = 0; i < triCount; ++i)
    {
        const uint32_t a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];
        BVHTOY_ASSERT(a < vertexCount && b < vertexCount && c < vertexCount);

        const float *p0 = vertices + a * vertexSizeInFloats;
        const float *p1 = vertices + b * vertexSizeInFloats;
        const float *p2 = vertices + c * vertexSizeInFloats;

        const float cx = (p0[0] + p1[0] + p2[0]) / 3.0f;
        const float cy = (p0[1] + p1[1] + p2[1]) / 3.0f;
        const float cz = (p0[2] + p1[2] + p2[2]) / 3.0f;

        outCentroids[i] = { { cx, cy, cz } };

        gminx = gminx > cx ? cx : gminx;
        gmaxx = gmaxx < cx ? cx : gmaxx;

        gminy = gminy > cy ? cy : gminy;
        gmaxy = gmaxy < cy ? cy : gmaxy;

        gminz = gminz > cz ? cz : gminz;
        gmaxz = gmaxz < cz ? cz : gmaxz;

        float minx = p0[0], miny = p0[1], minz = p0[2];
        float maxx = p0[0], maxy = p0[1], maxz = p0[2];

        minx = minx > p1[0] ? p1[0] : minx;
        minx = minx > p2[0] ? p2[0] : minx;
        maxx = maxx < p1[0] ? p1[0] : maxx;
        maxx = maxx < p2[0] ? p2[0] : maxx;

        miny = miny > p1[1] ? p1[1] : miny;
        miny = miny > p2[1] ? p2[1] : miny;
        maxy = maxy < p1[1] ? p1[1] : maxy;
        maxy = maxy < p2[1] ? p2[1] : maxy;

        minz = minz > p1[2] ? p1[2] : minz;
        minz = minz > p2[2] ? p2[2] : minz;
        maxz = maxz < p1[2] ? p1[2] : maxz;
        maxz = maxz < p2[2] ? p2[2] : maxz;

        outAabbs[i] = { { minx, miny, minz }, { maxx, maxy, maxz } };
    }

    *outCentroidsAabb = { { gminx, gminy, gminz }, { gmaxx, gmaxy, gmaxz } };
}

static float calculateMinimalMergeBoxAndSah(bvhtoy_Aabb *outMinBox, uint32_t *outMinIdx, bvhtoy_Aabb *aabbs, uint32_t minIdx, uint32_t maxIdx, uint32_t testIdx, float bestMinSah)
{
    const float *minSrc = aabbs[testIdx].vmin;
    const float *maxSrc = aabbs[testIdx].vmax;

    float minDst[3];
    float maxDst[3];

    for (uint32_t i = minIdx; i < maxIdx; ++i)
    {
        const float *minI = aabbs[i].vmin;
        const float *maxI = aabbs[i].vmax;

        for (uint32_t k = 0; k < 3; ++k)
        {
            minDst[k] = minSrc[k] < minI[k] ? minSrc[k] : minI[k];
            maxDst[k] = maxSrc[k] > maxI[k] ? maxSrc[k] : maxI[k];
        }

        const float testMergeDst[] = { maxDst[0] - minDst[0], maxDst[1] - minDst[1], maxDst[2] - minDst[2] };

        const float testMergeSah = testMergeDst[0] * testMergeDst[1] +
                                   testMergeDst[1] * testMergeDst[2] +
                                   testMergeDst[2] * testMergeDst[0];

        if (bestMinSah > testMergeSah && testIdx != i)
        {
            bestMinSah = testMergeSah;

            /** store better index and better box*/
            memcpy(&outMinBox->vmin, minDst, sizeof(float) * 3);
            memcpy(&outMinBox->vmax, maxDst, sizeof(float) * 3);
            *outMinIdx = i;
        }
    }
    return bestMinSah;
}

typedef struct EdgeHasherState { uint32_t dummy; } EdgeHasherState;

static uint32_t edgeHasherComputeHash(EdgeHasherState *state, uint64_t edge)
{
    const uint32_t e0 = (uint32_t)(edge >> 32);
    const uint32_t e1 = (uint32_t)(edge);

    uint32_t h1 = e0;
    uint32_t h2 = e1;

    const uint32_t m = 0x5bd1e995;

    // MurmurHash64B finalizer
    h1 ^= h2 >> 18;
    h1 *= m;
    h2 ^= h1 >> 22;
    h2 *= m;
    h1 ^= h2 >> 17;
    h1 *= m;
    h2 ^= h1 >> 19;
    h2 *= m;

    (void)state;

    return h2;
}

static uint32_t edgeHasherCompareEqual(EdgeHasherState *state, uint64_t lhs, uint64_t rhs)
{
    uint32_t l0 = (uint32_t)(lhs >> 32);
    uint32_t l1 = (uint32_t)(lhs);

    uint32_t r0 = (uint32_t)(rhs >> 32);
    uint32_t r1 = (uint32_t)(rhs);

    (void)state;

    return l0 == r0 && l1 == r1;
}

uint32_t bvhtoyGenerateLeafAabbsAndCentroidsAndQuadify(bvhtoy_Aabb *outLeafAabbs, bvhtoy_Aabb *outLeafCentroidsAabb, bvhtoy_Centroid *outLeafCentroids, uint32_t *outQuadAndTriangleIndices, const uint32_t *indices, uint32_t indexCount, const float *vertices, uint32_t vertexCount, uint32_t vertexSizeInFloats)
{
    BVHTOY_ASSERT(indexCount % 3 == 0);

    static const int next[3] = { 1, 2, 0 };

    EdgeHasherState edgeHasher;

    uint32_t edgeTableSize = hashBuckets(indexCount);

    uint32_t *edgeAdjacency = bvhtoyAlloc(uint32_t, indexCount);
    uint32_t *edgeIndices = bvhtoyAlloc(uint32_t, indexCount);

    uint64_t *edgeTable = bvhtoyAlloc(uint64_t, edgeTableSize);
    uint32_t *edgeIndexTable = bvhtoyAlloc(uint32_t, edgeTableSize);

    memset(edgeTable, -1, edgeTableSize * sizeof(uint64_t));
    memset(edgeIndexTable, -1, edgeTableSize * sizeof(uint32_t));

    uint32_t edgeCount = 0;

    BVHTOY_BEGIN_TRACE("Building edge list");
    /** for every pair of triangle indices we assign a generated 'edge index' to be able to address edges defined by a pair of integers as single integer */
    for (uint32_t i = 0; i < indexCount; i += 3)
    {
        for (uint32_t e = 0; e < 3; ++e)
        {
            const uint32_t i0 = indices[i + e];
            const uint32_t i1 = indices[i + next[e]];

            BVHTOY_ASSERT(i0 < vertexCount && i1 < vertexCount);

            const uint32_t imin = i0 < i1 ? i0 : i1;
            const uint32_t imax = i0 > i1 ? i0 : i1;

            const uint64_t edge = (((uint64_t)imax) << 32) | imin;

            /** hash "ordered" edge (an edge which goes from minimal index to maximal index of two) */
            uint32_t edgeTableIndex = 0;
            hashLookup(edgeTableIndex, edgeTable, edgeTableSize, &edgeHasher, edgeHasherComputeHash, edgeHasherCompareEqual, edge, ~0ull);

            if (edgeTable[edgeTableIndex] == ~0ull)
            {
                edgeTable[edgeTableIndex] = edge;

                /** allocated a unique index for an "ordered" edge */
                edgeIndexTable[edgeTableIndex] = edgeCount++;
            }
            const uint32_t edgeIndex = edgeIndexTable[edgeTableIndex];

            /** store edge index as it is per triangle if the minimal vertex index out of two is starting index, otherwise -- store it as inverted index */
            edgeIndices[i + e] = i0 == imin ? edgeIndex : ~edgeIndex;
        }
    }
    BVHTOY_END_TRACE();

    bvhtoyDealloc(edgeIndexTable);
    bvhtoyDealloc(edgeTable);

    uint32_t *edgeTriangleCounts = bvhtoyAlloc(uint32_t, edgeCount);
    uint32_t *edgeTriangleOffsets = bvhtoyAlloc(uint32_t, edgeCount);

    memset(edgeTriangleCounts, 0, edgeCount * sizeof(uint32_t));

    uint32_t triangleCount = (uint32_t)indexCount / 3;

    BVHTOY_BEGIN_TRACE("Building triangle adjacency");
    /** for each edge index, increment the number of triangles sharing this edge */
    for (uint32_t i = 0; i < indexCount; ++i)
    {
        const uint32_t e = edgeIndices[i];
        edgeTriangleCounts[e < edgeCount ? e : ~e] += 1;
    }

    /** compute prefix sum of triangle counts sharing a particular edge */
    uint32_t offset = 0;
    for (uint32_t i = 0; i < edgeCount; ++i)
    {
        edgeTriangleOffsets[i] = offset;
        offset += edgeTriangleCounts[i];
    }

    BVHTOY_ASSERT(offset == indexCount);

    /** iterate over triangle edges and write the triangle index into a list of triangles sharing an edge */
    for (uint32_t i = 0; i < triangleCount; ++i)
    {
        uint32_t e0 = edgeIndices[i * 3], e1 = edgeIndices[i * 3 + 1], e2 = edgeIndices[i * 3 + 2];

        e0 = e0 < edgeCount ? e0 : ~e0;
        e1 = e1 < edgeCount ? e1 : ~e1;
        e2 = e2 < edgeCount ? e2 : ~e2;

        edgeAdjacency[edgeTriangleOffsets[e0]++] = i;
        edgeAdjacency[edgeTriangleOffsets[e1]++] = i;
        edgeAdjacency[edgeTriangleOffsets[e2]++] = i;
    }

    /** decrement offsets, so they point to starts of per edge triangle lists */
    for (uint32_t i = 0; i < edgeCount; ++i)
    {
        edgeTriangleOffsets[i] -= edgeTriangleCounts[i];
    }
    BVHTOY_END_TRACE();

    float *triangleNormals = bvhtoyAlloc(float, indexCount);
    bvhtoy_Aabb *triangleAabbs = bvhtoyAlloc(bvhtoy_Aabb, triangleCount);
    bvhtoy_Centroid *triangleCentroids = bvhtoyAlloc(bvhtoy_Centroid, triangleCount);

    float gminx = kFltMax, gminy = kFltMax, gminz = kFltMax;
    float gmaxx = -kFltMax, gmaxy = -kFltMax, gmaxz = -kFltMax;

    BVHTOY_BEGIN_TRACE("Computing triangle normals and axis-aligned bounding boxes");
    for (uint32_t i = 0; i < triangleCount; ++i)
    {
        const uint32_t a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];

        const float *p0 = vertices + vertexSizeInFloats * a;
        const float *p1 = vertices + vertexSizeInFloats * b;
        const float *p2 = vertices + vertexSizeInFloats * c;

        const float cx = (p0[0] + p1[0] + p2[0]) / 3.0f;
        const float cy = (p0[1] + p1[1] + p2[1]) / 3.0f;
        const float cz = (p0[2] + p1[2] + p2[2]) / 3.0f;

        triangleCentroids[i] = { { cx, cy, cz } };

        gminx = gminx > cx ? cx : gminx;
        gmaxx = gmaxx < cx ? cx : gmaxx;

        gminy = gminy > cy ? cy : gminy;
        gmaxy = gmaxy < cy ? cy : gmaxy;

        gminz = gminz > cz ? cz : gminz;
        gmaxz = gmaxz < cz ? cz : gmaxz;

        float minx = p0[0], miny = p0[1], minz = p0[2];
        float maxx = p0[0], maxy = p0[1], maxz = p0[2];

        minx = minx > p1[0] ? p1[0] : minx;
        minx = minx > p2[0] ? p2[0] : minx;
        maxx = maxx < p1[0] ? p1[0] : maxx;
        maxx = maxx < p2[0] ? p2[0] : maxx;

        miny = miny > p1[1] ? p1[1] : miny;
        miny = miny > p2[1] ? p2[1] : miny;
        maxy = maxy < p1[1] ? p1[1] : maxy;
        maxy = maxy < p2[1] ? p2[1] : maxy;

        minz = minz > p1[2] ? p1[2] : minz;
        minz = minz > p2[2] ? p2[2] : minz;
        maxz = maxz < p1[2] ? p1[2] : maxz;
        maxz = maxz < p2[2] ? p2[2] : maxz;

        triangleAabbs[i] = { { minx, miny, minz }, { maxx, maxy, maxz } };

        float p10[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
        float p20[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };

        float nx = p10[1] * p20[2] - p10[2] * p20[1];
        float ny = p10[2] * p20[0] - p10[0] * p20[2];
        float nz = p10[0] * p20[1] - p10[1] * p20[0];

        float area = sqrtf(nx * nx + ny * ny + nz * nz);
        float invarea = (area == 0.f) ? 0.f : 1.f / area;

        triangleNormals[i * 3 + 0] = nx * invarea;
        triangleNormals[i * 3 + 1] = ny * invarea;
        triangleNormals[i * 3 + 2] = nz * invarea;
    }
    *outLeafCentroidsAabb = { { gminx, gminy, gminz }, { gmaxx, gmaxy, gmaxz } };
    BVHTOY_END_TRACE();

    uint32_t *candidates = bvhtoyAlloc(uint32_t, triangleCount);
    uint8_t *candidateUsed = bvhtoyAlloc(uint8_t, triangleCount);

    for (uint32_t i = 0; i < triangleCount; ++i)
        candidates[i] = i;

    memset(candidateUsed, 0, sizeof(uint8_t) * triangleCount);

    bvhtoy_Aabb *quadBestAabbs = bvhtoyAlloc(bvhtoy_Aabb, triangleCount);
    uint32_t *quadBestTriangleIdx = bvhtoyAlloc(uint32_t, triangleCount);

    uint32_t candidateCount = triangleCount;
    uint32_t quadCount = 0;

    for (uint32_t iteration = 0; ; ++iteration)
    {
        /** iterate over all candidate triangles to be merged into quads */
        BVHTOY_BEGIN_TRACE("Iteration %u: generating min-SAH adjacency for %u nodes", iteration, candidateCount);
        for (uint32_t c = 0; c < candidateCount; ++c)
        {
            /** fetch candidate triangle index */
            const uint32_t ci = candidates[c];

            quadBestTriangleIdx[ci] = ci;
            quadBestAabbs[ci] = { { kFltMax, kFltMax, kFltMax }, { -kFltMax, -kFltMax, -kFltMax } };

            const float nx = triangleNormals[ci * 3 + 0];
            const float ny = triangleNormals[ci * 3 + 1];
            const float nz = triangleNormals[ci * 3 + 2];

            float bestMinSah = kFltMax;

            /** for each edge ...*/
            for (uint32_t j = 0; j < 3; ++j)
            {
                const uint32_t e = edgeIndices[ci * 3 + j];

                /** decode the edge */
                const uint32_t d = e < edgeCount ? e : ~e;

                const uint32_t adjacentTriangleCount = edgeTriangleCounts[d];
                const uint32_t adjacentTriangleStart = edgeTriangleOffsets[d];

                /** check all neighbooring triangles sharing current edge */
                for (uint32_t k = 0; k < adjacentTriangleCount; ++k)
                {
                    /** fetch next neighboring triangle index (`ni`) sharing candidate edge `decodedEdge` */
                    const uint32_t ni = edgeAdjacency[adjacentTriangleStart + k];

                    /**
                    * Check:
                    *      a. the candidate triangle (`ci`) is not the current neighboring triangle (`ni`) (not self)
                    *      b. the current neighboring triangle (`ni`) didn't participate in any of quads in prior iteration steps
                    *      c. the current edge of candidate triangle processed triangle (`e`) is the opposite edge to any of edges of the neighboring triangle (`ni`)
                    */
                    if (ci != ni && candidateUsed[ni] == 0 && (e == ~edgeIndices[ni * 3 + 0] || e == ~edgeIndices[ni * 3 + 1] || e == ~edgeIndices[ni * 3 + 2]))
                    {
                        /** compute the the cosine between normals of the candidate triangle (`ci`) and its current neighbor(`ni`) */
                        const float tnx = triangleNormals[ni * 3 + 0];
                        const float tny = triangleNormals[ni * 3 + 1];
                        const float tnz = triangleNormals[ni * 3 + 2];

                        const float cosSq = nx * tnx + ny * tny + nz * tnz;

                        static const float cosSqThreshold01Deg = 0.999695414f;
                        static const float cosSqThreshold05Deg = 0.992403877f;
                        static const float cosSqThreshold10Deg = 0.969846319f;
                        static const float cosSqThreshold15Deg = 0.933012702f;
                        static const float cosSqThreshold30Deg = 0.75;

                        if (cosSq > cosSqThreshold15Deg)
                        {
                            bestMinSah = calculateMinimalMergeBoxAndSah(&quadBestAabbs[ci], &quadBestTriangleIdx[ci], triangleAabbs, ni, ni + 1, ci, bestMinSah);
                        }
                    }
                }
            }
        }
        BVHTOY_END_TRACE();

        uint32_t candidateNextCount = 0;

        BVHTOY_BEGIN_TRACE("Iteration %u: making quads from candidate triangles", iteration);
        for (uint32_t c = 0; c < candidateCount; ++c)
        {
            const uint32_t i = candidates[c];

            const uint32_t j = quadBestTriangleIdx[i];
            /**
            *  Check whether triangle was no merged because:
            *      a. there was no candiates (`i == j`)
            *      b. there was best candidate (such `j` that `i != j`) but the best candidate's best candidate is not as current triangle (`quadBestTriangleIdx[j] != i`)
            */
            if (i == j || i != quadBestTriangleIdx[j])
            {
                BVHTOY_ASSERT(candidateNextCount <= c);
                candidates[candidateNextCount ++] = i;
            }
            else if (i < j)
            {
                /** output aabb of resulting quad */
                outLeafAabbs[quadCount] = quadBestAabbs[i];

                const float *ci = triangleCentroids[i].v;
                const float *cj = triangleCentroids[j].v;

                /** output centroid of the resulting quad as a median of triangle centroids */
                outLeafCentroids[quadCount] = { { (ci[0] + cj[0]) * 0.5f, (ci[1] + cj[1]) * 0.5f, (ci[2] + cj[2]) * 0.5f } };

                /** output indices of merged triangles */
                outQuadAndTriangleIndices[quadCount * 2 + 0] = i;
                outQuadAndTriangleIndices[quadCount * 2 + 1] = j;

                /** mark both triangles as used, so they are not participating in next merge step */
                candidateUsed[i] = 0xff;
                candidateUsed[j] = 0xff;

                ++quadCount;
            }
        }
        BVHTOY_END_TRACE();

        if (candidateNextCount == candidateCount)
            break;

        candidateCount = candidateNextCount;
    }

    BVHTOY_ASSERT(quadCount * 2 + candidateCount == triangleCount);

    BVHTOY_BEGIN_TRACE("Appending triangle indices and axis-aligned bounding boxes to quads");

    /** append triangle indices which didn't form quads into the stream of pairs of triangle indices forming quads */
    for (uint32_t i = 0; i < candidateCount; ++i)
    {
        outLeafAabbs[quadCount + i] = triangleAabbs[candidates[i]];
        outLeafCentroids[quadCount + i] = triangleCentroids[candidates[i]];
        outQuadAndTriangleIndices[quadCount * 2 + i] = candidates[i];
    }
    BVHTOY_END_TRACE();

    BVHTOY_TRACE("Created %u quads with %u triangle left (out of %u input triangles)", quadCount, candidateCount, triangleCount);

    bvhtoyDealloc(quadBestTriangleIdx);
    bvhtoyDealloc(quadBestAabbs);
    bvhtoyDealloc(candidateUsed);
    bvhtoyDealloc(candidates);
    bvhtoyDealloc(triangleCentroids);
    bvhtoyDealloc(triangleAabbs);
    bvhtoyDealloc(triangleNormals);
    bvhtoyDealloc(edgeTriangleOffsets);
    bvhtoyDealloc(edgeTriangleCounts);
    bvhtoyDealloc(edgeIndices);
    bvhtoyDealloc(edgeAdjacency);
    return quadCount;
}

/**
 *  based on https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
 *
 * "Insert" two 0 bits after each of the 20 low bits of x
 */
 uint64_t Part1By2_64(uint64_t x)
{
    x &= 0x000fffffull;                          // x = ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- jihg fedc ba98 7654 3210
    x = (x ^ (x << 32)) & 0x000f00000000ffffull; // x = ---- ---- ---- jihg ---- ---- ---- ---- ---- ---- ---- ---- fedc ba98 7654 3210
    x = (x ^ (x << 16)) & 0x000f0000ff0000ffull; // x = ---- ---- ---- jihg ---- ---- ---- ---- fedc ba98 ---- ---- ---- ---- 7654 3210
    x = (x ^ (x <<  8)) & 0x000f00f00f00f00full; // x = ---- ---- ---- jihg ---- ---- fedc ---- ---- ba98 ---- ---- 7654 ---- ---- 3210
    x = (x ^ (x <<  4)) & 0x00c30c30c30c30c3ull; // x = ---- ---- ji-- --hg ---- fe-- --dc ---- ba-- --98 ---- 76-- --54 ---- 32-- --10
    x = (x ^ (x <<  2)) & 0x0249249249249249ull; // x = ---- --j- -i-- h--g --f- -e-- d--c --b- -a-- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
    return x;
}

uint64_t EncodeMorton3_64(uint64_t x, uint64_t y, uint64_t z)
{
    return (Part1By2_64(z) << 2) + (Part1By2_64(y) << 1) + Part1By2_64(x);
}


static void generateMortonCodes60(uint64_t *outMortonCodes, const bvhtoy_Aabb *centroidsAabb, const bvhtoy_Centroid  *centroids, uint32_t centroidCount)
{
    const float *maxv = centroidsAabb->vmax;
    const float *minv = centroidsAabb->vmin;

    float extent[3];

    extent[0] = maxv[0] - minv[0];
    extent[1] = maxv[1] - minv[1];
    extent[2] = maxv[2] - minv[2];

    float scale[3];
    scale[0] = extent[0] == 0.0f ? 0.0f : 1048575.0f / extent[0];
    scale[1] = extent[1] == 0.0f ? 0.0f : 1048575.0f / extent[1];
    scale[2] = extent[2] == 0.0f ? 0.0f : 1048575.0f / extent[2];

#if 0
    float extendMax = 0.0;
    extendMax = extent[0] > extendMax ? extent[0] : extendMax;
    extendMax = extent[1] > extendMax ? extent[1] : extendMax;
    extendMax = extent[2] > extendMax ? extent[2] : extendMax;
#endif

    for (uint32_t i = 0; i < centroidCount; ++i)
    {
        const float *v = centroids[i].v;
        int32_t x = (int32_t)((v[0] - minv[0]) * scale[0] + 0.5f);
        int32_t y = (int32_t)((v[1] - minv[1]) * scale[1] + 0.5f);
        int32_t z = (int32_t)((v[2] - minv[2]) * scale[2] + 0.5f);

#if 0
        BVHTOY_ASSERT(x >= 0.0 && x <= 1.0);
        BVHTOY_ASSERT(y >= 0.0 && y <= 1.0);
        BVHTOY_ASSERT(z >= 0.0 && z <= 1.0);

#endif
        outMortonCodes[i] = EncodeMorton3_64(x, y, z);
    }
}

static void radixSort(uint32_t *outRemap, uint64_t *keys, uint32_t count)
{
    static const uint32_t kBitCount = 10u;
    static const uint32_t kBinCount = 1u << kBitCount;

    uint32_t *histograms = bvhtoyAlloc(uint32_t, kBinCount * 6);
    memset(histograms, 0, sizeof(uint32_t) * kBinCount * 6);

    /** compute size 10-bit histograms */
    for (uint32_t i = 0; i < count; ++i)
    {
        uint64_t key = keys[i];

        histograms[kBinCount * 0 + ((key >> (kBitCount * 0)) & (kBinCount - 1))]++;
        histograms[kBinCount * 1 + ((key >> (kBitCount * 1)) & (kBinCount - 1))]++;
        histograms[kBinCount * 2 + ((key >> (kBitCount * 2)) & (kBinCount - 1))]++;
        histograms[kBinCount * 3 + ((key >> (kBitCount * 3)) & (kBinCount - 1))]++;
        histograms[kBinCount * 4 + ((key >> (kBitCount * 4)) & (kBinCount - 1))]++;
        histograms[kBinCount * 5 + ((key >> (kBitCount * 5)) & (kBinCount - 1))]++;
    }

    uint32_t sum[6] = { 0, 0, 0, 0, 0, 0 };

    /** replace histogram data with prefix histogram sums in - place */
    for (uint32_t i = 0; i < kBinCount; ++i)
    {
        const uint32_t h[6] =
        {
            histograms[kBinCount * 0 + i], histograms[kBinCount * 1 + i], histograms[kBinCount * 2 + i],
            histograms[kBinCount * 3 + i], histograms[kBinCount * 4 + i], histograms[kBinCount * 5 + i]
        };

        histograms[kBinCount * 0 + i] = sum[0];
        histograms[kBinCount * 1 + i] = sum[1];
        histograms[kBinCount * 2 + i] = sum[2];
        histograms[kBinCount * 3 + i] = sum[3];
        histograms[kBinCount * 4 + i] = sum[4];
        histograms[kBinCount * 5 + i] = sum[5];

        sum[0] += h[0];
        sum[1] += h[1];
        sum[2] += h[2];
        sum[3] += h[3];
        sum[4] += h[4];
        sum[5] += h[5];
    }

    BVHTOY_ASSERT(sum[0] == count && sum[1] == count && sum[2] == count && sum[3] == count && sum[4] == count && sum[5] == count);

    uint32_t *tmpRemap = bvhtoyAlloc(uint32_t, count);

    for (uint32_t i = 0; i < count; ++i)
        outRemap[i] = i;

    uint32_t *remaps[] = { outRemap, tmpRemap };

    for (uint32_t pass = 0; pass < 6; ++pass)
    {
        const uint32_t *srcRemap = remaps[(pass & 1)];
        uint32_t *dstRemap = remaps[(pass & 1) ^ 1];

        uint32_t *historgram = &histograms[kBinCount * pass];
        uint32_t bitStart = kBitCount * pass;

        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t subkey = (keys[srcRemap[i]] >> bitStart) & (kBinCount - 1);
            dstRemap[historgram[subkey] ++] = srcRemap[i];
        }
    }
    bvhtoyDealloc(tmpRemap);
    bvhtoyDealloc(histograms);
}

static float estimateSah(bvhtoy_Aabb const* aabbs, uint32_t aabbCount)
{
    uint32_t acc_sah_i = 0;
    float acc_sah_f = 0.0;

    for (uint32_t i = 0; i < aabbCount; ++i)
    {
        const float *maxv = aabbs[i].vmax;
        const float *minv = aabbs[i].vmin;

        float extent[3] = { maxv[0] - minv[0], maxv[1] - minv[1], maxv[2] - minv[2] };

        accumulateSahFromExtent(&acc_sah_i, &acc_sah_f, extent);
    }
    return (float)acc_sah_i + acc_sah_f;
}

void bvhtoyBuildBvh2(bvhtoy_Aabb *outBvh2Aabbs, bvhtoy_Bvh2 *outBvh2Nodes, const bvhtoy_Aabb *leafAabbs, const bvhtoy_Aabb *leafCentroidsAabb, bvhtoy_Centroid const *leafCentroids, uint32_t leafCount)
{
    uint64_t *mortonCodes = bvhtoyAlloc(uint64_t, leafCount);

    BVHTOY_BEGIN_TRACE_SCOPED("Building Bvh2 Hierarchy");

    // generate remap table for AABBs (based on centroids's morton codes)
    BVHTOY_BEGIN_TRACE("Generating morton codes (60bit) for %u leafs", leafCount);
        generateMortonCodes60(mortonCodes, leafCentroidsAabb, leafCentroids, leafCount);
    BVHTOY_END_TRACE();

    uint32_t *tmpBoxIndicesMemory = bvhtoyAlloc(uint32_t, leafCount * 2);
    uint32_t *tmpIndices[2] = { tmpBoxIndicesMemory, tmpBoxIndicesMemory + leafCount };

    BVHTOY_BEGIN_TRACE("Sorting %u leafs spatially (Radix)", leafCount);
        radixSort(tmpIndices[0], mortonCodes, leafCount);
    BVHTOY_END_TRACE();

    bvhtoy_Aabb *tmpBoxesMemory = bvhtoyAlloc(bvhtoy_Aabb, leafCount * 2);
    bvhtoy_Aabb *tmpBoxes[2] = { tmpBoxesMemory, tmpBoxesMemory + leafCount };

    uint32_t *adjacencyIndices = bvhtoyAlloc(uint32_t, leafCount);

    // remap boxes and their indices into temporal arrays, so boxes are sorted in morton order and their corresponding indices point to boxes in the original `inputBoxes` array
    BVHTOY_BEGIN_TRACE("Remapping %u leaf boxes", leafCount);

        bvhtoy_Aabb *dstBoxes = tmpBoxes[0];
        uint32_t *srcBoxIndices = tmpIndices[0];

        for (uint32_t i = 0; i < leafCount; ++i)
        {
            dstBoxes[i] = leafAabbs[srcBoxIndices[i]];
        }
    BVHTOY_END_TRACE();

#define REDUCE_SCAN_WINDOW 1
#if REDUCE_SCAN_WINDOW
    uint32_t mergeCountOnMinSahWindowShrink = 0;
    uint32_t kMinSahWindowSize = 32;
#else
    static const uint32_t kMinSahWindowSize = 256;
#endif

    uint32_t totalMergeCount = 0;
    uint32_t srcBoxCount = leafCount;

    uint32_t srcIndex = 0;
    uint32_t dstIndex = 1;

    for (uint32_t iteration = 0; ; ++iteration)
    {
        bvhtoy_Aabb *srcBoxes = tmpBoxes[srcIndex];
        bvhtoy_Aabb *dstBoxes = tmpBoxes[dstIndex];

        // generate adjacency based on Min SAH
        BVHTOY_BEGIN_TRACE("Level %u: generating min-SAH adjacency with %u-element window size for %u nodes", iteration, kMinSahWindowSize, srcBoxCount);
            uint32_t radius = kMinSahWindowSize;

            for (uint32_t i = 0; i < srcBoxCount; ++i)
            {
                uint32_t minIdx = i > radius ? i - radius : 0;
                uint32_t maxIdx = (i + radius + 1) < srcBoxCount ? (i + radius + 1) : srcBoxCount;
                calculateMinimalMergeBoxAndSah(&dstBoxes[i], &adjacencyIndices[i], srcBoxes, minIdx, maxIdx, i, kFltMax);
            }
        BVHTOY_END_TRACE();

        uint32_t dstBoxCount = 0;

        BVHTOY_BEGIN_TRACE("Level %u: merging candidates (in %u nodes)", iteration, srcBoxCount);

            uint32_t *dstBoxIndices = tmpIndices[dstIndex];
            const uint32_t *srcBoxIndices = tmpIndices[srcIndex];

            for (uint32_t i = 0; i < srcBoxCount; ++i)
            {
                const uint32_t j = adjacencyIndices[i];

                if (i != adjacencyIndices[j])
                {
                    /** compact boxes in-place */
                    if (dstBoxCount < i)
                        dstBoxes[dstBoxCount] = dstBoxes[i];

                    /** pass the index of the box as is for the next step */
                    dstBoxIndices[dstBoxCount ++] = srcBoxIndices[i];

                    /** The number of boxes remaining after merge step (created by merge operation from 2 boxes + unmerged) can't exceed the number of input boxes. */
                    BVHTOY_ASSERT(dstBoxCount <= leafCount);
                }
                else if (i < j)
                {
                    /** output newly merged box and corresponding indices (each index is either leaf node of one of boxes generated during previous iteration steps */
                    const bvhtoy_Bvh2 op = { { srcBoxIndices[i], srcBoxIndices[j] } };

                    outBvh2Aabbs[totalMergeCount] = dstBoxes[i];
                    outBvh2Nodes[totalMergeCount] = op;

                    /** compact boxes in-place */
                    if (dstBoxCount < i)
                        dstBoxes[dstBoxCount] = dstBoxes[i];

                    /** generate new index for the box created from merge operation for the next step */
                    dstBoxIndices[dstBoxCount ++] = leafCount + totalMergeCount++;

                    /** The number of boxes remaining after merge step (created by merge operation from 2 boxes + unmerged) can't exceed the number of input boxes. */
                    BVHTOY_ASSERT(dstBoxCount <= leafCount);

                    BVHTOY_ASSERT(totalMergeCount <= leafCount - 1);
                }
            }
        BVHTOY_END_TRACE();

        const uint32_t mergeCount = srcBoxCount - dstBoxCount;

#if REDUCE_SCAN_WINDOW
        if (mergeCountOnMinSahWindowShrink == 0)
            mergeCountOnMinSahWindowShrink = mergeCount;

        if (kMinSahWindowSize > 1 && mergeCountOnMinSahWindowShrink != 0 && mergeCount < mergeCountOnMinSahWindowShrink / 2)
        {
            mergeCountOnMinSahWindowShrink = mergeCount;
            kMinSahWindowSize >>= 1;
        }
#endif
        srcBoxCount = dstBoxCount;

        srcIndex ^= 1;
        dstIndex ^= 1;

        if (srcBoxCount == 1)
        {
            break;
        }
    }

    bvhtoyDealloc(adjacencyIndices);
    bvhtoyDealloc(tmpBoxesMemory);
    bvhtoyDealloc(tmpBoxIndicesMemory);
    bvhtoyDealloc(mortonCodes);
    BVHTOY_END_TRACE_SCOPED();
}

uint32_t bvhtoyConvertBvh2IntoBvh4(bvhtoy_Bvh4 *outBvh4Node, uint32_t bvh4NodeCountMax, const bvhtoy_Bvh2 *bvh2Nodes, uint32_t bvh2NodeCount, uint32_t leafNodeCount)
{
    uint32_t *bvh2IdQueue = bvhtoyAlloc(uint32_t, leafNodeCount);

    uint32_t bvh2IdQueueTop = 0;
    uint32_t bvh2IdQueueBot = 0;

    bvh2IdQueue[bvh2IdQueueTop ++] = bvh2NodeCount - 1;

    /** setup append counter */
    uint32_t bvh4Count = 0;

    while (bvh2IdQueueBot < bvh2IdQueueTop)
    {
        /** pop the index of the next bvh2 node from the queue */
        const uint32_t bvh2Id = bvh2IdQueue[bvh2IdQueueBot ++];

        /** fetch its `Bvh2` structure and retrive a pointer to an array of children node ids */
        const uint32_t *childrenIds = bvh2Nodes[bvh2Id].id;

        uint32_t dstChildIdx = 0;

        /** allocate bvh4 node */
        bvhtoy_Bvh4 *bvh4 = outBvh4Node + bvh4Count++;
        BVHTOY_ASSERT(bvh4Count < bvh4NodeCountMax);

        /** populate bvh4 node from children of current bvh2 node */
        for (uint32_t i = 0; i < 2; ++i)
        {
            uint32_t id = childrenIds[i];
            BVHTOY_ASSERT(id < leafNodeCount + bvh2NodeCount);

            /**
             *  look at the i-th child's id of the current bvh2 node
             *      - if id is not a leaf node (>= leafNodeCount), eliminate this i-th child (another bvh2 node) from hierarchy
             *        but look into its children (grandchildren of current bvh2 node) to make them direct children of currently
             *        created bvh4 node
             *      - if id is leaf node -- simply make it direct child of currently created bvh4 node
             */
            if (id >= leafNodeCount)
            {
                /** compute the actual id of bvh2 node because it's stored with `+ leafNodeCount`, so all node ids `id < leafNodeCount` refer to leaf nodes */
                id -= leafNodeCount;

                const uint32_t *grandchildrenIds = bvh2Nodes[id].id;

                for (uint32_t j = 0; j < 2; ++j)
                {
                    id = grandchildrenIds[j];
                    BVHTOY_ASSERT(id < leafNodeCount + bvh2NodeCount);

                    /** if it's not leaf node, push the id of bvh2 node (node `id - leafNodeCount`) onto the stack for further processing, otherwise record the id of leaf node in current merge4 operation */
                    if (id >= leafNodeCount)
                    {
                        bvh4->boxId[dstChildIdx] = id - leafNodeCount;
                        bvh4->id[dstChildIdx++] = bvh2IdQueueTop + leafNodeCount;

                        bvh2IdQueue[bvh2IdQueueTop++] = id - leafNodeCount;
                    }
                    else
                    {
                        bvh4->boxId[dstChildIdx] = id;
                        bvh4->id[dstChildIdx++] = id;
                    }
                }
            }
            else
            {
                bvh4->boxId[dstChildIdx] = id;
                bvh4->id[dstChildIdx++] = id;
            }
        }

        /** mark remaning ids unused */
        for (; dstChildIdx < 4; ++dstChildIdx)
        {
            bvh4->id[dstChildIdx] = ~0u;
            bvh4->boxId[dstChildIdx] = ~0u;
        }
    }
    bvhtoyDealloc(bvh2IdQueue);
    return bvh4Count;
}
