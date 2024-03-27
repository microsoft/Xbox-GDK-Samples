//--------------------------------------------------------------------------------------
// Utils.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef HLSL_INCLUDE
typedef float4x4    Matrix;
typedef float4      Vector4;
typedef float3      Vector3;
typedef float2      Vector2;
typedef uint        uint32_t;
#else
using namespace DirectX::SimpleMath;
#endif

#define TEMPORAL_RESOLVE_THREAD_X   8
#define TEMPORAL_RESOLVE_THREAD_Y   8

#define VARIANCE_NEIGHBOUR_COUNT    9

enum GPU_TIMER_PASSES
{
    TIMER_FRAME_TIME = 0,
    TIMER_GEOMETRY_PASS,
    TIMER_FULL_SCREEN_SHADING_PASS,
    TIMER_TAA_RESOLVE_PASS,
    TIMER_TAA_COPY_TO_BACKBUFFER_PASS,
    TIMER_COUNT
};

enum TAA_FLAGS
{
    TAA_FLAG_NONE = 0u,
    TAA_FLAG_ENABLE_TAA = 1u,
    TAA_FLAG_USE_DILATION_NONE = 2u,
    TAA_FLAG_USE_DILATION_CLOSEST_DEPTH = 4u,
    TAA_FLAG_USE_DILATION_GREATEST_VELOCITY = 8u,
    TAA_FLAG_USE_BILINEAR_FILTER = 16u,
    TAA_FLAG_USE_BICUBIC_FILTER_9TAPS = 32u,
    TAA_FLAG_USE_BICUBIC_FILTER_5TAPS = 64u,
    TAA_FLAG_USE_RGB_CLAMP = 128u,
    TAA_FLAG_USE_RGB_CLIPPING = 256u,
    TAA_FLAG_USE_VARIANCE_CLIPPING = 512u
};

enum SHADER_VISIBLE_HEAP_INDICES
{
    SRV_COLOR_BUFFER = 0,
    SRV_CURRENT_DEPTH,
    SRV_PREVIOUS_ACCUMULATION_0,
    SRV_PREVIOUS_ACCUMULATION_1,
    UAV_CURRENT_ACCUMULATION_0,
    UAV_CURRENT_ACCUMULATION_1,
    SRV_GBUFFER_ALBEDO,
    SRV_GBUFFER_NORMALS,
    SRV_GBUFFER_MOTION_VECTORS,
    UAV_FSQ_MOTION_VECTORS,
    UAV_INTERMEDIATE_RT,
    SRV_INTERMEDIATE_RT,
    SHADER_VISIBLE_DESC_HEAP_INDICES_COUNT
};

enum RTV_HEAP_INDICES
{
    RTV_COLOR_BUFFER = 0,
    RTV_GBUFFER_MOTION,
    RTV_GBUFFER_NORMALS,
    RTV_GBUFFER_ALBEDO,
    RTV_HEAP_INDICES_COUNT
};

struct TemporalResolveCB
{
    Vector4 resolution;
    uint32_t flags;
    float varianceNeighbourCountRCP;
};

struct FullscreenPassCB
{
    Matrix prevFrameView;
    Matrix prevFrameProj;
    Matrix invView;
    Matrix invProj;
    Vector4 resolution;
    Vector3 cameraPosition;
};

struct ZoomPassCB
{
    Vector2 minUV;
    Vector2 maxUV;
    Vector3 borderColor;
    float width;
    float height;
    float margin;
};

struct SceneCB
{
    Matrix viewProj;
    Vector4 jitterNDC; // current Jitter in xy. Previous in zw.
    Vector2 halfRes;
    float samplingBias;
};

struct PerObjectCB
{
    Matrix world;
    Matrix normalTransform;
    Matrix previousWorld;
    Vector3 diffuseColor;
};

struct MagnifyingGlassPassCB
{
    Vector2 vertexMinCoordsNDC; 
    Vector2 vertexMaxCoordsNDC; 
    Vector2 texMinUV;
    Vector2 texMaxUV;
};

struct SharpenPassCB
{
    float SharpeningAmount;
    bool skipSharpen;
};

#ifndef HLSL_INCLUDE
enum JitterQRSequence
{
    JITTER_QR_HALTON23_8 = 0,
    JITTER_QR_HALTON23_16,
    JITTER_QR_BLUENOISE_16,
    JITTER_QR_SEQUENCE_COUNT
};

static wchar_t const* s_jitterQRSequenceName[JITTER_QR_SEQUENCE_COUNT] =
{
    L"Halton23_8",
    L"Halton23_16",
    L"BlueNoise_16"
};

enum DilationMode
{
    DILATION_MODE_NONE = 0,
    DILATION_MODE_CLOSEST_DEPTH,
    DILATION_MODE_GREATEST_VELOCITY,
    DILATION_MODE_COUNT
};

static wchar_t const* s_dilationModeNames[DILATION_MODE_COUNT] =
{
    L"None",
    L"Closest Depth",
    L"Greatest Velocity"
};

enum FilterMode
{
    FILTER_MODE_BILINEAR = 0,
    FILTER_MODE_BICUBIC_9TAPS,
    FILTER_MODE_BICUBIC_5TAPS,
    FILTER_MODE_COUNT
};

static wchar_t const* s_filterModeNames[FILTER_MODE_COUNT] =
{
    L"Bilinear",
    L"Bicubic 9 taps",
    L"Bicubic 5 taps"
};

enum ClipMode
{
    CLIP_MODE_RGB_CLAMP,
    CLIP_MODE_RGB_CLIPPING,
    CLIP_MODE_VARIANCE_CLIPPING,
    CLIP_MODE_COUNT
};

static wchar_t const* s_clipModeNames[CLIP_MODE_COUNT] =
{
    L"RGB Clamp",
    L"RGB Clipping",
    L"Variance Clipping"
};
#endif

