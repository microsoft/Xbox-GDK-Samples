//--------------------------------------------------------------------------------------
// Raytracing.hlsl
//
// Shader library with DXR shaders entry points.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef __XBOX_SCARLETT
float XDXR_RayTCurrent();
#endif

#define HLSL
#include "RaytracingHlslCompat.h"
#include "ProceduralPrimitivesLibrary.hlsli"
#include "RaytracingShaderHelper.hlsli"

//***************************************************************************
//*****------ Shader resources bound via root signatures -------*************
//***************************************************************************

// g_* - bound via a global root signature.
// l_* - bound via a local root signature.

// Scene wide resources.
RaytracingAccelerationStructure g_scene :                                       register(t0);
RWTexture2D<float4> g_renderTarget :                                            register(u0);
ConstantBuffer<SceneConstantBuffer> g_sceneCB :                                 register(b0);

// Triangle resources
ByteAddressBuffer g_indices :                                                   register(t1);
StructuredBuffer<Vertex> g_vertices :                                           register(t2);

// Procedural geometry resources
StructuredBuffer<PrimitiveInstancePerFrameBuffer> g_AABBPrimitiveAttributes :   register(t3);
ConstantBuffer<PrimitiveConstantBuffer> l_materialCB :                          register(b1);
ConstantBuffer<PrimitiveInstanceConstantBuffer> l_aabbCB:                       register(b2);


// Phong lighting model = ambient + diffuse + specular components.
// TODO - This method also exists in InlineRaytracing.hlsl. It will need to exist duplicated while waiting for bug 48051262.
inline float4 CalculatePhongLighting(in float4 albedo, in float3 hitWorldPosition, in float3 worldRayDirection, in float3 normal, in bool isInShadow,
    in const float4 lightPosition, in const float4 lightDiffuseColor, in const float4 lightAmbient,
    in float diffuseCoef = 1.0f, in float specularCoef = 1.0f, in float specularPower = 50.0f)
{
    float shadowFactor = isInShadow ? InShadowRadiance : 1.0f;
    float3 incidentLightRay = normalize(hitWorldPosition - lightPosition.xyz);

    // Diffuse component.
    float Kd = CalculateDiffuseCoefficient(hitWorldPosition, incidentLightRay, normal);
    float4 diffuseColor = shadowFactor * diffuseCoef * Kd * lightDiffuseColor * albedo;

    // Specular component.
    float4 specularColor = 0.0f;
    if (!isInShadow)
    {
        float4 lightSpecularColor = 1.0f;
        float4 Ks = CalculateSpecularCoefficient(hitWorldPosition, worldRayDirection, incidentLightRay, normal, specularPower);
        specularColor = specularCoef * Ks * lightSpecularColor;
    }

    // Ambient component.
    // Fake AO: Darken faces with normal facing downwards/away from the sky a little bit.
    float4 ambientColorMin = lightAmbient - 0.1f;
    float4 ambientColorMax = lightAmbient;
    float a = 1.0f - saturate(dot(normal, float3(0.0f, -1.0f, 0.0f)));
    float4 ambientColor = albedo * lerp(ambientColorMin, ambientColorMax, a);

    return ambientColor + diffuseColor + specularColor;
}
 
//***************************************************************************
//*****------ TraceRay wrappers for radiance and shadow rays. -------********
//***************************************************************************

// Trace a radiance ray into the scene and returns a shaded color.
float4 TraceRadianceRay(in Ray ray, in uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return 0.0f;
    }

    // Set the ray's extents.
    // Set TMin to a zero value to avoid aliasing artifacts along contact areas.
    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    rayDesc.TMin = 0.0f;
    rayDesc.TMax = 10000.0f;

    // Note: make sure to enable face culling so as to avoid surface face fighting.
    RayPayload rayPayload = { float4(0.0f, 0.0f, 0.0f, 0.0f), currentRayRecursionDepth + 1u };
    TraceRay(g_scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        ~0u,    // Everything is visible.
        0,      // Ray contribution offset - RadianceRay
        2,      // Geometry multiplier (2 entries per geometry, since there are two ray types)
        0,      // MissShader Offset for RadianceRay
        rayDesc, rayPayload);

    return rayPayload.color;
}

// Trace a shadow ray and return true if it hits any geometry.
bool TraceShadowRayAndReportIfHit(in Ray ray, in uint currentRayRecursionDepth)
{
#ifndef __XBOX_SCARLETT
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return false;
    }
#endif

    float tmax = 10000.0f;

    // Set the ray's extents.
    // Set TMin to a zero value to avoid aliasing artifcats along contact areas.
    // Note: make sure to enable back-face culling so as to avoid surface face fighting.
    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    rayDesc.TMin = 0.0f;
    rayDesc.TMax = tmax;

    // Initialize shadow ray payload.
    // Set the initial value to true since closest and any hit shaders are skipped. 
    // Shadow miss shader, if called, will set it to false.
    ShadowRayPayload shadowPayload = { true };

    TraceRay(g_scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES
#ifdef __XBOX_SCARLETT
        | XBOX_RAY_FLAG_COHERENT_RAYS_HINT
#endif
        | RAY_FLAG_FORCE_OPAQUE             // ~skip any hit shaders
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER  // ~skip closest hit shaders,
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
        ~0u,    // InstanceMask - Everything is visible.
        1,      // Ray contribution Offset (ShadowRay).
        2,      // Geometry multiplier (2 entries per geometry, since there are 2 ray types)
        1,      // MissShader Offset for ShadowRay
        rayDesc, shadowPayload);

#ifdef __XBOX_SCARLETT
#ifdef BUG_47048430_IS_FIXED
    // If current T is less than the max, it means there was a collision
    // against an opaque object, so this pixel is in shadow.
    return XDXR_RayTCurrent() < tmax;
#else
    return shadowPayload.hit;
#endif
#else
    return shadowPayload.hit;
#endif // __XBOX_SCARLETT
}

//***************************************************************************
//********************------ Ray gen shader.. -------************************
//***************************************************************************

[shader("raygeneration")]
void MyRaygenShader()
{
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    Ray ray = GenerateCameraRay(DispatchRaysIndex().xy, DispatchRaysDimensions().xy, g_sceneCB.cameraPosition.xyz, g_sceneCB.projectionToWorld);
 
    // Cast a ray into the scene and retrieve a shaded color.
    uint currentRecursionDepth = 0;
    float4 color = TraceRadianceRay(ray, currentRecursionDepth);

    // Write the raytraced color to the output texture.
    g_renderTarget[DispatchRaysIndex().xy] = color;
}

//***************************************************************************
//******************------ Closest hit shaders -------***********************
//***************************************************************************

[shader("closesthit")]
void MyClosestHitShader_Triangle(inout RayPayload rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
    // Get the base index of the triangle's first 16 bit index.
    uint indexSizeInBytes = 2u;
    uint indicesPerTriangle = 3u;
    uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load up three 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(baseIndex, g_indices);

    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 triangleNormal = g_vertices[indices[0]].normal;

    // PERFORMANCE TIP: it is recommended to avoid values carry over across TraceRay() calls. 
    // Therefore, in cases like retrieving HitWorldPosition(), it is recomputed every time.

    // Shadow component.
    // Trace a shadow ray.
    float3 hitPosition = HitWorldPosition();
    Ray shadowRay = { hitPosition, normalize(g_sceneCB.lightPosition.xyz - hitPosition) };
    bool shadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, rayPayload.recursionDepth);

    float checkers = AnalyticalCheckersTexture(HitWorldPosition(), DispatchRaysIndex().xy, DispatchRaysDimensions().xy, triangleNormal, g_sceneCB.cameraPosition.xyz, g_sceneCB.projectionToWorld);

    // Reflected component.
    float4 reflectedColor = 0.0f;
    if (l_materialCB.reflectanceCoef > 0.001f )
    {
        // Trace a reflection ray.
        Ray reflectionRay = { HitWorldPosition(), reflect(WorldRayDirection(), triangleNormal) };
        float4 reflectionColor = TraceRadianceRay(reflectionRay, rayPayload.recursionDepth);

        float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), triangleNormal, l_materialCB.albedo.xyz);
        reflectedColor = l_materialCB.reflectanceCoef * float4(fresnelR, 1.0f) * reflectionColor;
    }

    // Calculate final color.
    float4 phongColor = CalculatePhongLighting(l_materialCB.albedo, HitWorldPosition(), WorldRayDirection(), triangleNormal, shadowRayHit,
        g_sceneCB.lightPosition, g_sceneCB.lightDiffuseColor, g_sceneCB.lightAmbientColor,
        l_materialCB.diffuseCoef, l_materialCB.specularCoef, l_materialCB.specularPower);
    float4 color = checkers * (phongColor + reflectedColor);

    // Apply visibility falloff.
    float t = RayTCurrent();
    color = lerp(color, BackgroundColor, 1.0f - exp(-0.000002f * t * t * t));

    rayPayload.color = color;
}

[shader("closesthit")]
void MyClosestHitShader_AABB(inout RayPayload rayPayload, in ProceduralPrimitiveAttributes attr)
{
    // PERFORMANCE TIP: it is recommended to minimize values carry over across TraceRay() calls. 
    // Therefore, in cases like retrieving HitWorldPosition(), it is recomputed every time.

    // Shadow component.
    // Trace a shadow ray.
    float3 hitPosition = HitWorldPosition();
    Ray shadowRay = { hitPosition, normalize(g_sceneCB.lightPosition.xyz - hitPosition) };
    bool shadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, rayPayload.recursionDepth);

    // Reflected component.
    float4 reflectedColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (l_materialCB.reflectanceCoef > 0.001f)
    {
        // Trace a reflection ray.
        Ray reflectionRay = { HitWorldPosition(), reflect(WorldRayDirection(), attr.normal) };
        float4 reflectionColor = TraceRadianceRay(reflectionRay, rayPayload.recursionDepth);

        float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), attr.normal, l_materialCB.albedo.xyz);
        reflectedColor = l_materialCB.reflectanceCoef * float4(fresnelR, 1.0f) * reflectionColor;
    }

    // Calculate final color.
    float4 phongColor = CalculatePhongLighting(l_materialCB.albedo, HitWorldPosition(), WorldRayDirection(), attr.normal, shadowRayHit,
        g_sceneCB.lightPosition, g_sceneCB.lightDiffuseColor, g_sceneCB.lightAmbientColor,
        l_materialCB.diffuseCoef, l_materialCB.specularCoef, l_materialCB.specularPower);
    float4 color = phongColor + reflectedColor;

    // Apply visibility falloff.
    float t = RayTCurrent();
    color = lerp(color, BackgroundColor, 1.0f - exp(-0.000002f * t * t * t));

    rayPayload.color = color;
}

//***************************************************************************
//**********************------ Miss shaders -------**************************
//***************************************************************************

[shader("miss")]
void MyMissShader(inout RayPayload rayPayload)
{
    float4 backgroundColor = float4(BackgroundColor);
    rayPayload.color = backgroundColor;
}

[shader("miss")]
void MyMissShader_ShadowRay(inout ShadowRayPayload rayPayload)
{
    rayPayload.hit = false;
}

//***************************************************************************
//*****************------ Intersection shaders-------************************
//***************************************************************************

// Get ray in AABB's local space.
Ray GetRayInAABBPrimitiveLocalSpace()
{
    PrimitiveInstancePerFrameBuffer attr = g_AABBPrimitiveAttributes[l_aabbCB.instanceIndex];

    // Retrieve a ray origin position and direction in bottom level AS space 
    // and transform them into the AABB primitive's local space.
    Ray ray;
    ray.origin = mul(float4(ObjectRayOrigin(), 1), attr.BLASToLocalSpace).xyz;
    ray.direction = mul(ObjectRayDirection(), (float3x3) attr.BLASToLocalSpace);
    return ray;
}

[shader("intersection")]
void MyIntersectionShader_AnalyticPrimitive()
{
    Ray localRay = GetRayInAABBPrimitiveLocalSpace();
    AnalyticPrimitive::Enum primitiveType = (AnalyticPrimitive::Enum) l_aabbCB.primitiveType;

    float thit;
    ProceduralPrimitiveAttributes attr = (ProceduralPrimitiveAttributes)0;
    if (RayAnalyticGeometryIntersectionTest(localRay, RayFlags(), RayTMin(), RayTCurrent(), primitiveType, thit, attr))
    {
        PrimitiveInstancePerFrameBuffer aabbAttribute = g_AABBPrimitiveAttributes[l_aabbCB.instanceIndex];
        attr.normal = mul(attr.normal, (float3x3) aabbAttribute.localSpaceToBLAS);
        attr.normal = normalize(mul((float3x3) ObjectToWorld3x4(), attr.normal));

        ReportHit(thit, /*hitKind*/ 0, attr);
    }
}

[shader("intersection")]
void MyIntersectionShader_VolumetricPrimitive()
{
    Ray localRay = GetRayInAABBPrimitiveLocalSpace();
    VolumetricPrimitive::Enum primitiveType = (VolumetricPrimitive::Enum) l_aabbCB.primitiveType;
    
    float thit;
    ProceduralPrimitiveAttributes attr = (ProceduralPrimitiveAttributes)0;

    if (RayVolumetricGeometryIntersectionTest(localRay, RayFlags(), RayTMin(), RayTCurrent(), primitiveType, thit, attr, g_sceneCB.elapsedTime))
    {
        PrimitiveInstancePerFrameBuffer aabbAttribute = g_AABBPrimitiveAttributes[l_aabbCB.instanceIndex];
        attr.normal = mul(attr.normal, (float3x3) aabbAttribute.localSpaceToBLAS);
        attr.normal = normalize(mul((float3x3) ObjectToWorld3x4(), attr.normal));

        ReportHit(thit, /*hitKind*/ 0, attr);
    }
}

[shader("intersection")]
void MyIntersectionShader_SignedDistancePrimitive()
{
    Ray localRay = GetRayInAABBPrimitiveLocalSpace();
    SignedDistancePrimitive::Enum primitiveType = (SignedDistancePrimitive::Enum) l_aabbCB.primitiveType;

    float thit;
    ProceduralPrimitiveAttributes attr = (ProceduralPrimitiveAttributes)0;
    if (RaySignedDistancePrimitiveTest(localRay, RayFlags(), RayTMin(), RayTCurrent(), primitiveType, thit, attr, l_materialCB.stepScale))
    {
        PrimitiveInstancePerFrameBuffer aabbAttribute = g_AABBPrimitiveAttributes[l_aabbCB.instanceIndex];
        attr.normal = mul(attr.normal, (float3x3) aabbAttribute.localSpaceToBLAS);
        attr.normal = normalize(mul((float3x3) ObjectToWorld3x4(), attr.normal));
        
        ReportHit(thit, /*hitKind*/ 0, attr);
    }
}
