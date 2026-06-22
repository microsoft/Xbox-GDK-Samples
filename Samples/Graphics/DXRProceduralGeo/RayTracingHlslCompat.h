//--------------------------------------------------------------------------------------
// RaytracingHLSLCompat.h
//
// A header with shared definitions for C++ and HLSL source files. 
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef RAYTRACINGHLSLCOMPAT_H
#define RAYTRACINGHLSLCOMPAT_H

#ifdef HLSL
typedef float2          Vector2;
typedef float3          Vector3;
typedef float4          Vector4;
typedef float4x4        Matrix;
typedef uint            uint32_t;
#else
using namespace DirectX::SimpleMath;
#endif

// Number of metaballs to use within an AABB.
#define N_METABALLS             5         

#define N_FRACTAL_ITERATIONS    4 // = <1,...>

// PERFORMANCE TIP: Set max recursion depth as low as needed
// as drivers may apply optimization strategies for low recursion depths.
#define MAX_RAY_RECURSION_DEPTH 3

struct ProceduralPrimitiveAttributes
{
    Vector3 normal;
};

struct RayPayload
{
    Vector4 color;
    uint32_t recursionDepth;
};

struct ShadowRayPayload
{
    bool hit;
};

// Constant buffer with scene data. Bound via global root signature.
struct SceneConstantBuffer
{
    Matrix      projectionToWorld;
    Vector4     cameraPosition;
    Vector4     lightPosition;
    Vector4     lightAmbientColor;
    Vector4     lightDiffuseColor;
    float       reflectance;
    float       elapsedTime; // Elapsed application time.
    uint32_t    width;
    uint32_t    height;

    // Padding is necessary since Constant Buffers need to be aligned to the D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT.
    // This define is different for PC (256) and Xbox (16), which is why the #ifdef is necessary. If the alignment is not
    // enforced, wrong results can happen when updating the constant buffer values from the cpu (for instance, stepping on
    // the previous frame memory).
#ifdef _GAMING_DESKTOP
    uint32_t padding[28];
#endif

#ifndef HLSL
    SceneConstantBuffer()
        : reflectance(0.0f)
        , elapsedTime(0.0f)
        , width(0)
        , height(0)
#ifdef _GAMING_DESKTOP
        , padding{}
#endif
    {}
#endif
};

// Attributes per primitive type. Bound via local root signature.
struct PrimitiveConstantBuffer
{
    Vector4 albedo;
    float   reflectanceCoef;
    float   diffuseCoef;
    float   specularCoef;
    float   specularPower;
    float   stepScale;  // Step scale for ray marching of signed distance primitives. 
                        // - Some object transformations don't preserve the distances and 
                        //   thus require shorter steps.

#ifndef HLSL
    PrimitiveConstantBuffer()
        : reflectanceCoef(0.0f)
        , diffuseCoef(0.0f)
        , specularCoef(0.0f)
        , specularPower(0.0f)
        , stepScale(0.0f)
    {}
#endif
};

// Attributes per primitive instance. Bound via local root signature.
struct PrimitiveInstanceConstantBuffer
{
    uint32_t instanceIndex;
    uint32_t primitiveType; // Procedural primitive type
};

// This needs to be aligned to 16.
// Dynamic attributes per primitive instance.
struct PrimitiveInstancePerFrameBuffer
{
    Matrix localSpaceToBLAS;   // Matrix from local primitive space to bottom-level object space.
    Matrix BLASToLocalSpace;   // Matrix from bottom-level object space to local primitive space.
};

struct Vertex
{
    Vector3 position;
    Vector3 normal;
};

// Struct representing the memory layout of a shader record. Needed so the compute shader knows
// the layout and can access the correct data inside the struct
struct CustomShaderRecordPadded
{
    uint32_t shaderRecord[8]; // 32 bytes for the shader record
    PrimitiveConstantBuffer materialCb; // Local root bindings
    PrimitiveInstanceConstantBuffer aabbCB; // Local root bindings
    uint32_t pad[13]; // Padding, since shader records are being aligned to 64 bytes
};

// Ray types traced in this sample.
namespace RayType
{
    enum Enum
    {
        RadianceRay = 0,   // ~ Primary, reflected camera/view rays calculating color for each hit.
        ShadowRay,         // ~ Shadow/visibility rays, only testing for occlusion
        Count
    };
}

namespace TraceRayParameters
{
    static const uint32_t InstanceMask = ~0u;   // Everything is visible.

    namespace HitGroup
    {
        static const uint32_t Offset[RayType::Count] =
        {
            0, // Radiance ray
            1  // Shadow ray
        };
        static const uint32_t GeometryStride = RayType::Count;
    }

    namespace MissShader
    {
        static const uint32_t Offset[RayType::Count] =
        {
            0, // Radiance ray
            1  // Shadow ray
        };
    }
}

// From: http://blog.selfshadow.com/publications/s2015-shading-course/hoffman/s2015_pbs_physics_math_slides.pdf
static const Vector4 ChromiumReflectance = Vector4(0.549f, 0.556f, 0.554f, 1.0f);

static const Vector4 BackgroundColor = Vector4(0.8f, 0.9f, 1.0f, 1.0f);
static const float InShadowRadiance = 0.35f;

namespace AnalyticPrimitive
{
    enum Enum
    {
        AABB = 0,
        Spheres,
        Count
    };
}

namespace VolumetricPrimitive
{
    enum Enum
    {
        Metaballs = 0,
        Count
    };
}

namespace SignedDistancePrimitive
{
    enum Enum
    {
        MiniSpheres = 0,
        IntersectedRoundCube,
        SquareTorus,
        TwistedTorus,
        Cog,
        Cylinder,
        FractalPyramid,
        Count
    };
}

#endif // RAYTRACINGHLSLCOMPAT_H
