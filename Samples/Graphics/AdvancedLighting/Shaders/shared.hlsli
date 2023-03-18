//--------------------------------------------------------------------------------------
// shared.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Root signature for GPass
// Root 0   - Diffuse SRV
// Root 1   - GBuffer SRV
// Root 2:4 - Constant buffers (scene constants, per object data, etc)
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
// Root 0   - SRV for albedo
#define ROOT_SIGNATURE_AMBIENT   "\
    RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT \
               | DENY_DOMAIN_SHADER_ROOT_ACCESS \
               | DENY_HULL_SHADER_ROOT_ACCESS), \
    DescriptorTable (SRV(t0, numDescriptors  = 1), visibility=SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL)" 

// Root signature for the light volume calls
// 0 - SRV descriptor table
// 1 - CBV - Per Component data
// 2 - SRV - light position data
// 3 - CBV - Lighting data? (camera pos for some reason)
#define ROOT_SIGNATURE_LIGHT_VOLUME "\
    RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT \
               | DENY_DOMAIN_SHADER_ROOT_ACCESS \
               | DENY_HULL_SHADER_ROOT_ACCESS), \
    DescriptorTable (SRV(t0, numDescriptors  = 3), visibility=SHADER_VISIBILITY_PIXEL), \
    CBV(b0, space = 0), \
    SRV(t3, space = 0)"

// Root Signature for tiled and clustered Compute shaders
// 0 - TABLE - Texture where we write the results of the operations
// 1 - SRV   - The buffer that holds all the light data
// 2 - TABLE - The (3) G-Buffer textures 
// 3 - TABLE - The depth texture
// 4 - CBV   - Scene constants (light count, matrices)
#define ComputeRS "\
    DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
    SRV(t0, space = 0),\
    DescriptorTable(SRV(t1, numDescriptors=3), visibility=SHADER_VISIBILITY_ALL),\
    DescriptorTable(SRV(t4, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),\
    CBV(b0, space = 0),\
    CBV(b1, space = 0)"

// Particles (light source representation) Root Signature
// 0 - CBV with per scene constants
// 1 - SRV with particle position data
#define ParticleRS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT \
                            | DENY_DOMAIN_SHADER_ROOT_ACCESS \
                            | DENY_HULL_SHADER_ROOT_ACCESS), \
                    CBV(b0, space = 0), \
                    SRV(t0, space = 0)"

// Compute (Particles position and culling) Root Signature
// 0 - TABLE - Table containing the two UAV's and one buffer SRV
// 1 - CBV   - Update constants (view matrix, fustrum data and some general particle data)
#define ComputeParticlesRS "\
    DescriptorTable( UAV(u0, numDescriptors = 2), SRV(t0, numDescriptors = 1)), \
    CBV(b0, space = 0)"

//--------------------------------------------------------------------------------------
// Name: Colors
// Desc: ObjectToWorld3x4 assign to the particles
//--------------------------------------------------------------------------------------
#define COLOR_COUNT     8
static float4 CachedColors[COLOR_COUNT] = {
    float4(1.0f, 1.0f, 1.0f, 1.0f),
    float4(1.0f, 1.0f, 0.0f, 1.0f),
    float4(1.0f, 0.0f, 1.0f, 1.0f),
    float4(1.0f, 0.0f, 0.0f, 1.0f),
    float4(0.0f, 1.0f, 1.0f, 1.0f),
    float4(0.0f, 1.0f, 0.0f, 1.0f),
    float4(0.0f, 0.0f, 1.0f, 1.0f),
    float4(0.0f, 0.0f, 0.0f, 1.0f)
};

#define AMBIENT_INTENSITY       0.25f   // For ambient pass.
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
    float zMinView;
    float zMaxView;
};

//--------------------------------------------------------------------------------------
// Name: Lighting
// Desc: Constant buffer containing directional light information.
//--------------------------------------------------------------------------------------

struct Light
{
    float4 lightPosition;
};

struct LightingCB
{
    float4 Position;
    float4 Color;
    float4 CameraPos;
    float4 Range;
};
