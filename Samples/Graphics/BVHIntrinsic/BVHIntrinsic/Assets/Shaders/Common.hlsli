//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Set of common functionality for Voxels and Triangles.
// Contains a software emulation of the Scarlett image_bvh_intersect_ray intrinsic
// But it's not a perfect emulation by any means. Enough to get started with though.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef ENABLE_SCARLETT_INTERSECTION_TEST_INTRINSICS
#define GpuVA uint64_t
#else
#define GpuVA uint2
#endif

#define XboxBvh4NodePtr     uint32_t

// IEEE floating point specials
#define FLT_INF asfloat(0x7f800000)

#define NODE_TYPE_TRI0      0  // TriNode 0  {V0, V1, V2}
#define NODE_TYPE_TRI1      1  // TriNode 1  {V1, V3, V2}
#define NODE_TYPE_TRI2      2  // TriNode 2  {V2, V3, V4}
#define NODE_TYPE_TRI3      3  // TriNode 3  {V2, V4, V0}
#define NODE_TYPE_BOX16     4  // Box16
#define NODE_TYPE_BOX32     5  // Box32
#define NODE_TYPE_RESERVED  6  // Reserved
#define NODE_TYPE_USER      7  // Reserved (User)
#define NODE_TYPE_MASK      0x00000007

#define OFFSET_OF_V0        0
#define OFFSET_OF_V1        12
#define OFFSET_OF_V2        24
#define OFFSET_OF_V3        36
#define OFFSET_OF_V4        48
#define OFFSET_OF_REMAP     60
#define InvalidNodePtr      0xFFFFFFFF

#define RENDER_MODE_NORMAL 0
#define RENDER_MODE_DEPTH 1
#define RENDER_MODE_TRAVERSAL_COST 2
#define RENDER_MODE_PRIMITIVE 3

#if defined(__XBOX_ENABLE_WAVE32)
#define TG_WIDTH 8
#define TG_HEIGHT 4
#else
#define TG_WIDTH 8
#define TG_HEIGHT 8
#endif

static const uint WAVE_SIZE = (TG_WIDTH * TG_HEIGHT);

// Used on Xbox One where we have to fetch the BVH nodes manually rather than relying on the intrinsic
ByteAddressBuffer bvhNodes : register(t1);

// A small buffer for storing ray hits/misses for eventual display on the screen
RWStructuredBuffer<uint> statsBuffer : register(u1);

cbuffer Params
{
    float4x4 screenToCameraSpace;
    float4 cameraPosition;
    GpuVA vaBVH;
    uint bvhSize;
    uint color;
    uint firstLeafNodeIndex;
    uint renderMode;
    float elapsedTime;
};

inline float3 GetRayDirection(float2 pixelPos)
{
    float3 cameraSpace = mul(screenToCameraSpace, float4(pixelPos, 0, 1)).xyz;
    return normalize(cameraSpace);
}

uint GpuVALoad1(GpuVA address)
{
    return bvhNodes.Load((uint)address);
}

uint GpuVALoad1Offset(GpuVA address, uint offset)
{
    return GpuVALoad1((uint)address + offset);
}

uint2 GpuVALoad2(GpuVA address)
{
    return bvhNodes.Load2((uint)address);
}

uint3 GpuVALoad3(GpuVA address)
{
    return bvhNodes.Load3((uint)address);
}

uint3 GpuVALoad3Offset(GpuVA address, uint offset)
{
    return GpuVALoad3((uint)address + offset);
}

uint4 GpuVALoad4(GpuVA address)
{
    return bvhNodes.Load4((uint)address);
}

struct BoundingBox16
{
    half3 min;
    half3 max;
};
#define SizeOfBox16 12

struct BoundingBox32
{
    float3 min;
    float3 max;
};
#define SizeOfBox32 24

BoundingBox32 LoadBox32(GpuVA vaOfNode32, uint boxIndex)
{
    GpuVA boxAddress = vaOfNode32 + 16 + boxIndex * SizeOfBox32;
    BoundingBox32 ret;
    ret.min = asfloat(GpuVALoad3(boxAddress));
    ret.max = asfloat(GpuVALoad3Offset(boxAddress, 12));
    return ret;
}

BoundingBox32 LoadBox16(GpuVA vaOfNode16, uint boxIndex)
{
    GpuVA boxAddress = vaOfNode16 + 16 + boxIndex * SizeOfBox16;
    uint3 temp = GpuVALoad3(boxAddress);
    BoundingBox32 ret;
    ret.min = f16tof32(uint3(temp.x, temp.x >> 16, temp.y));
    ret.max = f16tof32(uint3(temp.y >> 16, temp.z, temp.z >> 16));
    return ret;
}

GpuVA NodePointerToVA(GpuVA vaBVH, XboxBvh4NodePtr nodePtr)
{
    // nodePtr[31:3] -> offset from BVH start in 64-byte steps
    return vaBVH + (GpuVA)(nodePtr >> 3) * (GpuVA)64;
}

inline uint32_t TypeOfNodePointer(XboxBvh4NodePtr nodePtr)
{
    // nodePtr[2:0] -> type
    return nodePtr & NODE_TYPE_MASK;
}

void Swap(inout float a, inout float b)
{
    float t = a; a = b; b = t;
}

void Swap(inout uint a, inout uint b)
{
    uint t = a; a = b; b = t;
}

float RayBoxTest(
    float ray_extent,
    float3 ray_origin,
    float3 ray_dir,
    float3 ray_inv_dir,
    BoundingBox32 box)
{
    // When an element of Direction is 0, its inverse needs to be NAN rather than INF.
    // IEEE float math won't propogate NANs, so this element won't affect min_t or max_t.

    float3 t_plane_min = (box.min - ray_origin) * ray_inv_dir;
    float3 t_plane_max = (box.max - ray_origin) * ray_inv_dir;

    float3 min_interval = select(ray_inv_dir > 0.0, t_plane_min, t_plane_max);
    float3 max_interval = select(ray_inv_dir > 0.0, t_plane_max, t_plane_min);

    float min_t = max(max(min_interval.x, min_interval.y), max(min_interval.z, 0.0));
    float max_t = min(min(max_interval.x, max_interval.y), min(max_interval.z, ray_extent));

    return min_t <= max_t ? min_t : FLT_INF;
}

uint4 TestXboxBvh4Node(
    GpuVA vaOfBVH,
    uint boxNodePtr,
    float ray_extent,
    float3 ray_origin,
    float3 ray_dir,
    float3 ray_inv_dir,
    bool b16bit,
    bool sortResults)
{
    // Clamp ray_extent to one value less than FLT_INF
    // This is just to fix our emulation of the intrinsic and
    // doesn't represent actual hardware behaviour.
    ray_extent = (ray_extent == FLT_INF) ? asfloat(0x7F7FFFFF) : ray_extent;

    GpuVA vaOfBoxNode = NodePointerToVA(vaOfBVH, boxNodePtr);
    uint4 pointers = GpuVALoad4(vaOfBoxNode);
    float4 results = FLT_INF;

    // Test each valid pointer's bounding box for ray intersection
    for (uint childIdx = 0; childIdx < 4; ++childIdx)
    {
        uint childPtr = pointers[childIdx];

        // NOTE: The hardware does not test for valid pointers before intersecting boxes.  If you have
        // valid box data, the pointer--valid or invalid--will be sorted into the return list.
        BoundingBox32 childBounds;
        if (b16bit)
            childBounds = LoadBox16(vaOfBoxNode, childIdx);
        else
            childBounds = LoadBox32(vaOfBoxNode, childIdx);

        results[childIdx] = RayBoxTest(ray_extent, ray_origin, ray_dir, ray_inv_dir, childBounds);
    }

    // Invalidate pointers of any missed boxes
    pointers = select(results <= ray_extent, pointers, InvalidNodePtr);

    if (sortResults)
    {
        // Sort pointers by distance
        if (results.x > results.z)
        {
            Swap(results.x, results.z);
            Swap(pointers.x, pointers.z);
        }

        if (results.y > results.w)
        {
            Swap(results.y, results.w);
            Swap(pointers.y, pointers.w);
        }

        if (results.x > results.y)
        {
            Swap(results.x, results.y);
            Swap(pointers.x, pointers.y);
        }

        if (results.z > results.w)
        {
            Swap(results.z, results.w);
            Swap(pointers.z, pointers.w);
        }

        if (results.y > results.z)
        {
            Swap(results.y, results.z);
            Swap(pointers.y, pointers.z);
        }
    }

    return pointers;
}

#if ENABLE_SCARLETT_INTERSECTION_TEST_INTRINSICS

uint4 BvhMakeDescriptor(
    GpuVA vaBVH,        // Start address of BVH in bytes (256B-aligned)
    uint64_t size,
    bool sortBoxes,     // Sort box intersections closest to farthest
    uint triangleReturnMode = 0,
    uint growBoxes = 0)   // Amount to grow boxes (0-255 ULPs)
{
    // Extract bits [47:8] from BVH start address
    uint64_t baseAddress = (vaBVH << 16) >> 24;
    
    /*
    struct BvhDescriptor
    {
        // lo
        uint64_t    baseAddress         : 40;   // BvhStart / 256
        uint64_t    _unused0            : 15;
        uint64_t    boxGrowingAmount    :  8;   // Size to grow boxes in ULPs
        uint64_t    boxSortingEnabled   :  1;   // Sort results of box node intersections

        // hi
        uint64_t    size                : 42;   // BvhSize / 64 - 1 (for range checking)
        uint64_t    _unused1            : 14;
        uint64_t    triangleReturnMode  :  1;   // 0: triangle ID  1: barycentric coordinates
        uint64_t    _unused2            :  2;
        uint64_t    bigPages            :  1;   // Entire BVH uses 64KB pages (or larger)
        uint64_t    type                :  4;   // Must be set to '8'
    };
    */

    // Configure options
    uint boxFlags = ((growBoxes & 0xFF) | ((uint)sortBoxes << 8)) << 23;
    uint miscFlags = (0x88 + triangleReturnMode) << 24; // type=8, bigPages=1, triangleReturnMode

    // Form 16 byte descriptor
    return uint4(baseAddress, (baseAddress >> 32) | boxFlags, size, (size >> 32) | miscFlags);
}

float4 BvhIntersectRayHardware(
    GpuVA vaBVH,
    uint nodePtr,
    float ray_extent,
    float3 ray_origin,
    float3 ray_dir,
    float3 ray_inv_dir,
    uint64_t size,
    bool sortBoxTestResults = true,
    bool triangleReturnMode = 0)
{
    uint4 res = InvalidNodePtr;

    size = __XB_MakeUniform(size);
    uint4 desc = BvhMakeDescriptor(__XB_MakeUniform(vaBVH), size, sortBoxTestResults, triangleReturnMode, 0);
    res = __XB_BvhIntersectRay(desc, nodePtr, ray_extent, ray_origin, ray_dir, ray_inv_dir);

    return asfloat(res);
}

#define BvhIntersectRay BvhIntersectRayHardware

#else

//
// Ray-Triangle Intersection Testing
//

// Break ties on shared edges.  Bottom and right edges should return true ("missed").
void BreakTieOnEdge(float Ay, float By, float Cy, bool winding_order, inout bool missed)
{
    // Is horizontal?
    if (abs(By) == 0.0 && abs(Cy) == 0.0)
    {
        bool bottom_edge = Ay > 0.0;
        missed |= bottom_edge;
    }
    else
    {
        // right edge
        bool first_vertex_below_second = By < 0.0 || (abs(By) == 0.0 && Cy > 0.0);
        bool right_edge = first_vertex_below_second ^ winding_order;
        missed |= right_edge;
    }
}

float4 TestXboxBvh4Leaf(
    GpuVA vaOfBVH,
    uint nodePtr,
    float3 origin,
    float3 dir,
    uint triangleReturnMode)
{
    // Read 3 vertices
    GpuVA vaTri = NodePointerToVA(vaOfBVH, nodePtr);    
    uint triIdx = TypeOfNodePointer(nodePtr);
    
    const uint v0LUT = 0x18180C00;  // Byte offset of v0 for tri 0-3 (3=24, 2=24, 1=12, 0=0)
    const uint v1LUT = 0x3024240C;  // Byte offset of v1 for tri 0-3 (3=48, 2=36, 1=36, 0=12)
    const uint v2LUT = 0x00301818;  // Byte offset of v2 for tri 0-3 (3=0, 2=48, 1=24, 0=24)

    uint lutOffset = (triIdx * 8);
    uint v0Offset = ((v0LUT >> lutOffset) & 0xFF);
    uint v1Offset = ((v1LUT >> lutOffset) & 0xFF);
    uint v2Offset = ((v2LUT >> lutOffset) & 0xFF);

    float3 v0 = asfloat(GpuVALoad3Offset(vaTri, v0Offset));
    float3 v1 = asfloat(GpuVALoad3Offset(vaTri, v1Offset));
    float3 v2 = asfloat(GpuVALoad3Offset(vaTri, v2Offset));
    
    // The remap flags reorder the vertices for the purposes of properly assigning barycentric weights
    uint triangleIDOrRemap = GpuVALoad1Offset(vaTri, OFFSET_OF_REMAP);
    uint remap = triangleIDOrRemap >> (triIdx * 8);

    // Reorder vector elements to make Z correspond with the dominant direction of the ray
    float3 mag = abs(dir);
    float max_mag = max(mag.x, max(mag.y, mag.z));

    if (mag.x == max_mag)
    {
        dir = dir.yzx;
        origin = origin.yzx;
        v0 = v0.yzx;
        v1 = v1.yzx;
        v2 = v2.yzx;
    }
    else if (mag.y == max_mag)
    {
        dir = dir.zxy;
        origin = origin.zxy;
        v0 = v0.zxy;
        v1 = v1.zxy;
        v2 = v2.zxy;
    }

    // Translate vertices to make them relative to the origin
    v0 -= origin;
    v1 -= origin;
    v2 -= origin;

    // Transform vertices to ray space.  The ray direction is now [0, 0, dir.z]
    float3 A = float3(v0.xy * dir.z - dir.xy * v0.z, v0.z);
    float3 B = float3(v1.xy * dir.z - dir.xy * v1.z, v1.z);
    float3 C = float3(v2.xy * dir.z - dir.xy * v2.z, v2.z);

    precise float3 UVW = float3(cross(C, B).z, cross(A, C).z, cross(B, A).z);

    const float t_num = dot(UVW, float3(A.z, B.z, C.z));
    const float t_denom = dot(UVW, dir.z);

    // Indicates a full miss
    bool triangle_missed = any(UVW < -0.0) && any(UVW > 0.0);

    // Indicates a zero area triangle.  (Either ray is coincident or degenerate point/line.)
    triangle_missed |= abs(t_denom) == 0.0;

    // Triangle is behind the ray
    triangle_missed |= asfloat(asuint(t_num) ^ asuint(t_denom) ^ asuint(abs(t_denom))) < 0.0;

    // Not enough precision
    triangle_missed |= isnan(t_num);

    // Handle special edge cases to guarantee only one triangle is hit on the edge
    const bool winding_order = t_denom > 0.0;
    if (abs(UVW.x) == 0.0)
        BreakTieOnEdge(A.y, B.y, C.y, winding_order, triangle_missed);
    if (abs(UVW.y) == 0.0)
        BreakTieOnEdge(B.y, C.y, A.y, winding_order, triangle_missed);
    if (abs(UVW.z) == 0.0)
        BreakTieOnEdge(C.y, A.y, B.y, winding_order, triangle_missed);

    if (triangle_missed)
        return float4(FLT_INF, 1.0, 0.0, 0.0);
    else
    {
        if(triangleReturnMode == 0)
            return float4(t_num, t_denom, asfloat(triangleIDOrRemap + triIdx), asfloat(0x1));
        else
            return float4(t_num, t_denom, UVW[(remap >> 0) & 3], UVW[(remap >> 2) & 3]);
    }
}

float4 BvhIntersectRaySoftware(
    GpuVA vaOfBVH,
    uint nodePtr,
    float ray_extent,
    float3 ray_origin,
    float3 ray_dir,
    float3 ray_inv_dir,
    uint size,
    bool sortBoxTestResults = true,
    bool triangleReturnMode = 0)
{
    switch (TypeOfNodePointer(nodePtr))
    {
    case NODE_TYPE_TRI0:
    case NODE_TYPE_TRI1:
    case NODE_TYPE_TRI2:
    case NODE_TYPE_TRI3:
        return TestXboxBvh4Leaf(vaOfBVH, nodePtr, ray_origin, ray_dir, triangleReturnMode);
    case NODE_TYPE_BOX16:
        return asfloat(TestXboxBvh4Node(vaOfBVH, nodePtr, ray_extent, ray_origin, ray_dir, ray_inv_dir, true, sortBoxTestResults));
    case NODE_TYPE_BOX32:
        return asfloat(TestXboxBvh4Node(vaOfBVH, nodePtr, ray_extent, ray_origin, ray_dir, ray_inv_dir, false, sortBoxTestResults));
    default:
        return 0;
    }
}

#define BvhIntersectRay BvhIntersectRaySoftware

#endif


bool AABBIntersect(float3 rayPos, float3 invRayDir, float3 bbMin, float3 bbMax, out uint sideEntered, out float minT)
{
    float3 l1 = (bbMin - rayPos) * invRayDir;
    float3 l2 = (bbMax - rayPos) * invRayDir;

    float3 minL = min(l1, l2);
    float3 maxL = max(l1, l2);

    minT = max(max(minL.x, minL.y), minL.z);
    float maxT = min(min(maxL.x, maxL.y), maxL.z);

    bool enteredY = (minL.y == minT);
    bool enteredZ = (minL.z == minT);
    sideEntered = (invRayDir.x > 0.0f) ? 1 : 0;   // Assume it entered via the X axis unless it didn't.
    sideEntered = enteredY ? (invRayDir.y > 0.0f ? 3 : 2) : sideEntered;
    sideEntered = enteredZ ? (invRayDir.z > 0.0f ? 5 : 4) : sideEntered;
    
    return (maxT >= 0 && maxT >= minT);
}

float3 UintToColor(uint i)
{
    float3 color;
    color.r = (i & 1) ? 1 : 0;
    color.g = (i & 2) ? 1 : 0;
    color.b = (i & 4) ? 1 : 0;
    return color;
}

void RecordRayStats(bool hit, uint bufferSlot)
{
#if defined(__XBOX_ENABLE_WAVE32)
    uint hitBallot = __XB_Ballot64(hit).x;
    uint numHits = countbits(hitBallot);
#else
    uint2 hitBallot = __XB_Ballot64(hit);
    uint numHits = countbits(hitBallot.x) + countbits(hitBallot.y);
#endif

    if (__XB_GetLaneID() == 0)
    {
        InterlockedAdd(statsBuffer[bufferSlot], numHits);
        InterlockedAdd(statsBuffer[bufferSlot+1], WAVE_SIZE - numHits);
    }
}
