//--------------------------------------------------------------------------------------
// shared.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef CPP_SHARED
#define cbuffer struct
typedef DirectX::SimpleMath::Vector4    float4;
typedef DirectX::SimpleMath::Matrix     float4x4;
typedef DirectX::SimpleMath::Vector3    float3;
typedef DirectX::SimpleMath::Vector2    float2;
typedef uint32_t                        uint;
#endif

// For histograms, this should be used as base for both shaders and cpp
#define NUM_BINS_PER_HISTOGRAM     8

// We are getting 4 histograms, one per component (r,g,b,a)
#define NUMBER_OF_HISTOGRAMS        4

// Offsets for the buffer that contains each histogram
#define HISTO_OFFSET_0        0
#define HISTO_OFFSET_1        NUM_BINS_PER_HISTOGRAM
#define HISTO_OFFSET_2        2 * NUM_BINS_PER_HISTOGRAM
#define HISTO_OFFSET_3        3 * NUM_BINS_PER_HISTOGRAM

// Root signature for GPass
// Root 0       - Diffuse SRV                                                   ( t0 )
// Root 1       - GBuffer SRV                                                   ( t1 - t3)
// Root 2,3,4   - Constant buffers (scene constants, per object data, etc)      ( b0 - b2)
// Root 5       - Root Constants (only used by ambient rn)                      ( b3 )
#define ROOT_SIGNATURE   "\
    RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT \
               | DENY_DOMAIN_SHADER_ROOT_ACCESS \
               | DENY_HULL_SHADER_ROOT_ACCESS), \
    DescriptorTable (SRV(t0, numDescriptors  = 1), visibility=SHADER_VISIBILITY_PIXEL), \
    DescriptorTable (SRV(t1, numDescriptors  = 3), visibility=SHADER_VISIBILITY_PIXEL), \
    CBV(b0, space = 0), \
    CBV(b1, space = 0), \
    CBV(b2, space = 0), \
    StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s1, visibility=SHADER_VISIBILITY_PIXEL)"

// Root signature for AmbientPass
// Root 0       - GBuffer SRV       ( t0 - t2)
// Root 1       - Root Constants    ( b3 )
#define ROOT_SIGNATURE_AMBIENT   "\
    RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT \
               | DENY_DOMAIN_SHADER_ROOT_ACCESS \
               | DENY_HULL_SHADER_ROOT_ACCESS), \
    DescriptorTable (SRV(t0, numDescriptors  = 3), visibility=SHADER_VISIBILITY_PIXEL), \
    RootConstants(num32BitConstants=4, b0), \
    StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s1, visibility=SHADER_VISIBILITY_PIXEL)" 

// Root Signature for tiled Compute shaders
// 0 - TABLE - Texture where we write the results of the operations
// 1 - CBV   - The constant buffer that holds the light positions
// 2 - TABLE - The (3) G-Buffer textures 
// 3 - TABLE - The depth texture
// 4 - CBV   - Scene constants (light count, matrices)
#define ComputeRS "\
    DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
    CBV(b0, space = 0),\
    DescriptorTable(SRV(t1, numDescriptors=3), visibility=SHADER_VISIBILITY_ALL),\
    DescriptorTable(SRV(t4, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
    CBV(b1, space = 0),\
    RootConstants(num32BitConstants = 4, b2)"

// root 0 - CBV
// root 1 - TABLE SRV (Input framebuffer)
// root 2 - TABLE UAV out buffer)
#define HISTO_ROOT_SIGNATURE "\
        CBV(b0, visibility=SHADER_VISIBILITY_ALL),\
        DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
        DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
        StaticSampler(\
            s0,\
            addressU = TEXTURE_ADDRESS_WRAP,\
            addressV = TEXTURE_ADDRESS_WRAP,\
            addressW = TEXTURE_ADDRESS_WRAP,\
            comparisonFunc = COMPARISON_ALWAYS,\
            borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,\
            filter = FILTER_MIN_MAG_MIP_LINEAR,\
            visibility = SHADER_VISIBILITY_ALL)"

// IMPORTANT - this needs to match with the declaration in utils.h
#define NUM_INSTANCES           64

//--------------------------------------------------------------------------------------
// Name: Colors
// Desc: ObjectToWorld3x4 assign to the particles
//--------------------------------------------------------------------------------------
#define COLOR_COUNT     8 //1
static float4 CACHED_COLORS[COLOR_COUNT] = {
    float4(0.5f, 0, 1, 1),
    float4(1, 0.5f, 0, 1),
    float4(1, 0, 1, 1),
    float4(0.5f, 0, 0, 1),
    float4(0, 1, 1, 1),
    float4(0, 1, 0, 1),
    float4(0, 0.5f, 1, 1),
    float4(1, 0, 0.5f, 1)
    //float4(1, 0, 0, 1)
};

#define AMBIENT_INTENSITY       0.5f    // For ambient pass.
#define MAX_LIGHTS_PER_TILE     512     // Per Cluster. Playing around with this value allows to reach better occupancy (decreases LDS load).
#define CLUSTERS_Z_COUNT        16      // Number of subdivisions on the else axis for clustered algorithm.
#define GROUP_WIDTH             8       // Width and Height (in pixels) for both Tiles and Clusters. Threadgroup size will be GROUP_WIDTH^2.

/// //--------------------------------------------------------------------------------------
/// // Name: PerComponent
/// // Desc: constant buffer containing per-component data.
/// //--------------------------------------------------------------------------------------
struct PerObjectCB
{
    float4x4    matWorld;
    float4x4    matWorldRotation;
};

struct zClusterRangesCB
{
    struct ZClusterRanges
    {
        float4 range; // x=min, y=max
    } zClusterRanges[CLUSTERS_Z_COUNT];
};

struct SceneConstants
{
    float4x4 viewProj;
    float4x4 view;
    float4x4 invProj;
    float3 cameraPos;
    uint lightCount;
    float lightRadius;
    uint tileCorrectScreenWidth;
    uint tileCorrectScreenHeight;
};

struct LightPositions
{
    float4 lightPositions[NUM_INSTANCES];
};

//--------------------------------------------------------------------------------------
// Name: Lighting
// Desc: Constant buffer containing directional light information.
//--------------------------------------------------------------------------------------

struct LightingCB
{
    float4 Position;
    float4 Color;
    float4 CameraPos;
    float4 Range;
};
