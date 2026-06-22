//--------------------------------------------------------------------------------------
// SharedDefinitions.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef USE_HLSL
typedef float4x4    Matrix;
typedef float2      Vector2;
typedef float3      Vector3;
typedef float4      Vector4;
typedef uint        uint32_t;
#else
using namespace DirectX::SimpleMath;
#endif

#define PPLL_CLEAR_VALUE    0xffffffff  

#define NODE_COUNT_MLAB     4
#define NODE_COUNT_PPLL     8

#define TRANSLUCENT_DIM_X  3 
#define TRANSLUCENT_DIM_Y  3   
#define TRANSLUCENT_COUNT  (TRANSLUCENT_DIM_X * TRANSLUCENT_DIM_Y)

#define TILE_SIZE_X         8
#define TILE_SIZE_Y         8

#define TRANSLUCENT_ALPHA  0.4f

enum OpaqueObjs
{
    OPAQUE_CITY = 0,
    OPAQUE_TERRAIN,
    OPAQUE_COUNT
};

enum DescriptorHeapRTIndices
{
    RTV_INDEX_INTERMEDIATE_RT = 0,
    RTV_INDEX_COUNT
};

enum DescriptorHeapIndices
{
    UAV_PPLL_HEAD_POINTER_BUFFER = 0,
    UAV_PPLL_BUFFER,
    UAV_PPLL_BUFFER_COUNTER,
    SRV_PPLL_HEAD_POINTER_BUFFER,
    SRV_PPLL_BUFFER,
    SRV_MLAB_CLEAR_MASK,
    SRV_MLAB_NODE_LIST,
    UAV_MLAB_CLEAR_MASK,
    UAV_MLAB_NODE_LIST,
    UAV_SWAPCHAIN_01,
    UAV_SWAPCHAIN_02,
    UAV_INTERMEDIATE_RT,
    SHADER_VISIBLE_DESC_HEAP_COUNT
};

enum TIMER_PASS
{
    TIMER_CLEAR_UAV_PASS = 0,
    TIMER_BLEND_PASS,
    TIMER_COMPOSITE_PASS,
    TIMER_PASS_COUNT
};

enum BlendMode
{
    ROP_BLEND = 0,
    CUSTOM_BLEND_PPLL,
    CUSTOM_BLEND_MLAB,
    BLEND_MODE_COUNT,
};

enum TranslucentChoice
{
    MODEL_DRAGONS = 0,
    MODEL_GARGOYLES,
    MODEL_COUNT,
};

#ifndef USE_HLSL
static wchar_t const* s_blendModesNames[BLEND_MODE_COUNT] =
{
    L"HW blend (sorted geo)",
    L"Per-pixel linked list",
    L"MLAB (uses ROV)"
};
#endif

struct SceneConstants
{
    Matrix viewMatrix;
    Matrix projMatrix;
};

struct ObjectConstants
{
    Matrix worldMatrix;
    Matrix normalMatrix;
    Vector3 diffuseColor;
};

struct BlendPassConstants
{
    Vector2 resolution;
    float   cameraFar;
};

// https://interplayoflight.wordpress.com/2022/07/02/order-independent-transparency-part-2/
struct Fragment
{
    uint32_t color;
    uint32_t transmission   : 8;
    uint32_t depth          : 24;
    uint32_t next;
};

// https://interplayoflight.wordpress.com/2022/07/02/order-independent-transparency-part-2/
struct TransparentFragment
{
    uint32_t color;
    uint32_t transmission   : 8;
    uint32_t depth          : 24;
};

struct NodeFragments
{
    TransparentFragment frags[NODE_COUNT_MLAB];
};
