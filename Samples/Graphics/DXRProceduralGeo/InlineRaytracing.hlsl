//--------------------------------------------------------------------------------------
// InlineRaytracing.hlsl
//
// Compute shader that runs inline raytracing for Xbox Series consoles.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// For GDKs prior to March 2024 (2403), we will continue using the InlineIS path.
// For March and onwards the sample will use the HLSL2021 based lambda system.
// This define (and its application in the sample) should be removed as soon as the
// minimum GDK for this sample becomes 2403.
#if GDK_VER >= 240300
#define _USE_HLSL2021_LAMBDA_SYSTEM
#endif

#define HLSL
#include "RaytracingHlslCompat.h"
#include "ProceduralPrimitivesLibrary.hlsli"

#ifndef _USE_HLSL2021_LAMBDA_SYSTEM
// DXR 1.1 RayQuery traversal is required to break out of the BVH traversal loop in order to give control to the user for handling AnyHit
// or Intersection-Shader logic in a while(query.Proceed()) loop. This can lead to complex flow-control and suboptimal performance.
// XDXR Standalone Traversal provides the InlineAHS and InlineIS mechanisms to improve performance when a shader uses the same AnyHit or
// Intersection Shader logic across all TraceRayInline calls. This define allows the sample to use inline intersection shaders for AABBs.
#define XDXR_USE_INLINE_IS_FUNCTION 1
#endif

// In place of the RayQuery object from DXR 1.1, we provide several versions of an XDXRStandaloneTraverse***.hlsli header file inside the GDK install directory:
// GDK_Install_Path\GDK_Version\GXDK\gameKit\Include\Scarlett. These headers are provided as source code, however it is up to the developer to ensure their header
// file version is compatible with the GDK that their BVHs are built against. Prior versions of the header will not be supported on newer GDKs, and modifications
// of the header must be merged with any changes from a newer GDK.
#include "XDXRStandaloneTraverseHT.hlsli"

// Root signature for the inline raytracing compute shader.
// -ROOT INDEX : 0 -> Acceleration Structure
// -ROOT INDEX : 1 -> Shader Table
// -ROOT INDEX : 2 -> Scene ConstantBuffer
// -ROOT INDEX : 3 -> SRV structured buffer
// -ROOT INDEX : 4 -> Index - Vertex buffer
// -ROOT INDEX : 4 -> Output UAV
#define InlineRootSignature " \
    SRV(t0, space = 0), \
    SRV(t1, space = 0), \
    CBV(b0, space = 0), \
    SRV(t2, space = 0), \
    DescriptorTable(SRV(t3, numDescriptors=2)), \
    DescriptorTable(UAV(u0, numDescriptors=1))"

// Raytracing acceleration structure.
RaytracingAccelerationStructure                     g_scene : register(t0);

// Shader table to fetch local bindings.
StructuredBuffer<CustomShaderRecordPadded>          g_shaderTable : register(t1);

// Scene constant data.
ConstantBuffer<SceneConstantBuffer>                 g_sceneCB : register(b0);

// Per primitive instance data.
StructuredBuffer<PrimitiveInstancePerFrameBuffer>   g_AABBPrimitiveAttributes : register(t2);

// Triangle resources.
ByteAddressBuffer                                   g_indices : register(t3);
StructuredBuffer<Vertex>                            g_vertices : register(t4);

// Output UAV.
RWTexture2D<float4>                                 g_colorBuffer : register(u0);

// Global Variables
static float3 g_normalAttr;
static const float g_maxT = 100000.0f;
static const uint g_rayContributionToHitGroupIndex = 0;
static const uint g_multiplierForGeometryContributionToHitGroupIndex = TraceRayParameters::HitGroup::GeometryStride;

// Structure for storing data related to a hit.
struct RayIntersectionData
{
    float3 phongColor;
    float rayTCurrent;
    float3 fresnelxReflectance;
    float checkers;
};

// Container for holding information about each ray bounce. Later used to compose it all into
// one color for hit contribution to the final color.
static RayIntersectionData g_intersectionStack[MAX_RAY_RECURSION_DEPTH + 1];

// Phong lighting model = ambient + diffuse + specular components.
// TODO - This method also exists in Raytracing.hlsl. It will need to exist duplicated while waiting for bug 48051262.
inline float4 CalculatePhongLighting(in float4 albedo, in float3 hitWorldPosition, in float3 worldRayDirection, in float3 normal, in bool isInShadow,
    in float diffuseCoef = 1.0f, in float specularCoef = 1.0f, in float specularPower = 50.0f)
{
    float4 lightPosition = g_sceneCB.lightPosition;
    float shadowFactor = isInShadow ? InShadowRadiance : 1.0f;
    float3 incidentLightRay = normalize(hitWorldPosition - lightPosition.xyz);

    // Diffuse component.
    float4 lightDiffuseColor = g_sceneCB.lightDiffuseColor;
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
    float4 lightAmbient = g_sceneCB.lightAmbientColor;
    float4 ambientColorMin = lightAmbient - 0.1f;
    float4 ambientColorMax = lightAmbient;
    float a = 1.0f - saturate(dot(normal, float3(0.0f, -1.0f, 0.0f)));
    float4 ambientColor = albedo * lerp(ambientColorMin, ambientColorMax, a);

    return ambientColor + diffuseColor + specularColor;
}

// Get Ray direction from the camera to a pixel (with pixelCoords).
float3 GetRayDirection(uint2 pixelCoords)
{
    float2 xy = pixelCoords + 0.5f; // center in the middle of the pixel.
    float2 screenPos = (xy / float2(g_sceneCB.width, g_sceneCB.height)) * 2.0f - 1.0f;
    
    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;
    
    // Unproject the pixel coordinate into a world positon.
    float4 world = mul(float4(screenPos, 0.0f, 1.0f), g_sceneCB.projectionToWorld);
    world.xyz /= world.w;

    return normalize(world.xyz - g_sceneCB.cameraPosition.xyz);
}

// Get ray in AABB's local space.
Ray GetRayInAABBPrimitiveLocalSpace(uint instanceIndex, float3 objectRayOrigin, float3 objectRayDirection)
{
    PrimitiveInstancePerFrameBuffer attr = g_AABBPrimitiveAttributes[instanceIndex];

    // Retrieve a ray origin position and direction in bottom level AS space and transform them into the AABB primitive's local space.
    Ray ray;
    ray.origin = mul(float4(objectRayOrigin, 1), attr.BLASToLocalSpace).xyz;
    ray.direction = mul(objectRayDirection, (float3x3) attr.BLASToLocalSpace);
    return ray;
}

#ifdef _USE_HLSL2021_LAMBDA_SYSTEM
// DXR 1.1 RayQuery traversal is required to break out of the BVH traversal loop in order to give control to the user for handling AnyHit
// or Intersection-Shader logic in a while(query.Proceed()) loop. This can lead to complex flow-control and suboptimal performance.
// XDXR Standalone Traversal provides the InlineAHS and InlineIS mechanisms to improve performance
struct ProceduralObjectLambda : InlineISBaseLambda
{
#endif
    // Inline method that will be called (during the XboxRayQuery's proceed calls) whenever there is a ray-AABB intersection.
    void InlineIS(inout XboxRayQueryProxy rqp)
    {
        const uint shaderTableIndex = rqp.CandidateInstanceContributionToHitGroupIndex() + g_rayContributionToHitGroupIndex +
            g_multiplierForGeometryContributionToHitGroupIndex * rqp.CandidateGeometryIndex();
        CustomShaderRecordPadded shaderRecord = g_shaderTable[shaderTableIndex];

        bool hit = false;
        float thit;
        ProceduralPrimitiveAttributes attr = (ProceduralPrimitiveAttributes)0;
        Ray localRay = GetRayInAABBPrimitiveLocalSpace(shaderRecord.aabbCB.instanceIndex, rqp.CandidateObjectRayOrigin(), rqp.CandidateObjectRayDirection());

        uint rayFlags = rqp.RayFlags();
        float rayTMin = rqp.RayTMin();
        float rayTCurrent = rqp.CommittedRayT();

        if (rqp.CandidateGeometryIndex() < 2) // AnalyticPrimitive
        {
            AnalyticPrimitive::Enum primitiveType = (AnalyticPrimitive::Enum)shaderRecord.aabbCB.primitiveType;
    
            if (RayAnalyticGeometryIntersectionTest(localRay, rayFlags, rayTMin, rayTCurrent, primitiveType, thit, attr))
            {
                PrimitiveInstancePerFrameBuffer aabbAttribute = g_AABBPrimitiveAttributes[shaderRecord.aabbCB.instanceIndex];
                attr.normal = mul(attr.normal, (float3x3) aabbAttribute.localSpaceToBLAS);
                attr.normal = normalize(mul((float3x3)rqp.CandidateObjectToWorld3x4(), attr.normal));
    
                hit = true;
            }
        }
        else if (rqp.CandidateGeometryIndex() == 2) // VolumetricPrimitive
        {
            VolumetricPrimitive::Enum primitiveType = (VolumetricPrimitive::Enum)shaderRecord.aabbCB.primitiveType;
    
            if (RayVolumetricGeometryIntersectionTest(localRay, rayFlags, rayTMin, rayTCurrent, primitiveType, thit, attr, g_sceneCB.elapsedTime))
            {
                PrimitiveInstancePerFrameBuffer aabbAttribute = g_AABBPrimitiveAttributes[shaderRecord.aabbCB.instanceIndex];
                attr.normal = mul(attr.normal, (float3x3) aabbAttribute.localSpaceToBLAS);
                attr.normal = normalize(mul((float3x3)rqp.CandidateObjectToWorld3x4(), attr.normal));
    
                hit = true;
            }
        }
        else // SignedDistancePrimitive
        {
            SignedDistancePrimitive::Enum primitiveType = (SignedDistancePrimitive::Enum)shaderRecord.aabbCB.primitiveType;
    
            if (RaySignedDistancePrimitiveTest(localRay, rayFlags, rayTMin, rayTCurrent, primitiveType, thit, attr, shaderRecord.materialCb.stepScale))
            {
                PrimitiveInstancePerFrameBuffer aabbAttribute = g_AABBPrimitiveAttributes[shaderRecord.aabbCB.instanceIndex];
                attr.normal = mul(attr.normal, (float3x3) aabbAttribute.localSpaceToBLAS);
                attr.normal = normalize(mul((float3x3)rqp.CandidateObjectToWorld3x4(), attr.normal));
    
                hit = true;
            }
        }

        if (hit)
        {
            rqp.CommitProceduralPrimitiveHit(thit);
            g_normalAttr = attr.normal;
        }
    }
#ifdef _USE_HLSL2021_LAMBDA_SYSTEM
};
#endif

// Returns true if intersectionPoint is in shade, and false if lit.
bool IsPointInShadow(in float3 intersectionPoint)
{
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    float3 rayDir = normalize(g_sceneCB.lightPosition.xyz - intersectionPoint);
    RayDesc shadowRay = { intersectionPoint, 0.0f, rayDir, g_maxT };

#ifndef _USE_HLSL2021_LAMBDA_SYSTEM
    XboxRayQuery shadowQuery = (XboxRayQuery)0;

    uint rayflags = RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_FORCE_OPAQUE             // ~skip any hit shaders
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER  // ~skip closest hit shaders,
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

    // TraceRayInlineNoAHS is used here to get inline IS support, and no AHS support (since we don't need it).
    shadowQuery.TraceRayInlineNoAHS(
        g_scene,    // Acceleration structure
        rayflags,
        ~0,         // Instance mask
        shadowRay);
#else // Hlsl2021 based lambda system

    // Pass RAY_FLAG_FORCE_OPAQUE as template flag since we do not need AHS support.
    // Pass ProceduralObjectLambda to enable InlineIS system for faster traversal.
    const RAY_FLAG RayFlags = RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_FORCE_OPAQUE
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

    XboxRayQuery<RayFlags, ProceduralObjectLambda> shadowQuery;

    // Best to use template flags, as compiler can optimize for these more thoroughly.
    uint runtimeRayflags = RAY_FLAG_NONE;

    shadowQuery.TraceRayInline(
        g_scene,    // Acceleration structure
        runtimeRayflags,
        ~0,         // Instance mask
        shadowRay);
#endif

    // Looping over this call is not required if the InlineIS mechanism is used.
    shadowQuery.Proceed();

    // If there was a hit, compare T against light distance and set boolean
    return (shadowQuery.CommittedStatus() != COMMITTED_NOTHING) && shadowQuery.CommittedRayT() < g_maxT;
}

// Entry point.
[RootSignature(InlineRootSignature)]
[numthreads(8, 4, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 currentRayOrigin = g_sceneCB.cameraPosition.xyz;
    float3 currentIncomingRay = 0.0f;
    float3 currentNormal = 0.0f;
    int lastValidBounce = -1;

    // MaxRayRecursionDepth represents the count of Primary plus Reflection ray(s) per pixel.
    [unroll]
    for (int i = 0; i < MAX_RAY_RECURSION_DEPTH; ++i)
    {
        // Trace a primary or reflection ray.
        float3 currentRayDirection = (i == 0) ? GetRayDirection(DTid.xy) : reflect(currentIncomingRay, currentNormal);
        RayDesc newRay = { currentRayOrigin , 0.0f, currentRayDirection, g_maxT };

#ifndef _USE_HLSL2021_LAMBDA_SYSTEM
        XboxRayQuery rayQuery = (XboxRayQuery)0;

        uint rayflags = RAY_FLAG_CULL_BACK_FACING_TRIANGLES;

        // TraceRayInlineNoAHS is used here to get inline IS support, and no AHS support (since we don't need it).
        rayQuery.TraceRayInlineNoAHS(
            g_scene,    // Acceleration structure
            rayflags,
            ~0,         // Instance mask
            newRay);
#else // Hlsl2021 based lambda system
        // Pass RAY_FLAG_FORCE_OPAQUE as template flag since we do not need AHS support.
        // Pass ProceduralObjectLambda to enable InlineIS system for faster traversal.
        XboxRayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ProceduralObjectLambda> rayQuery;
        
        // Best to use template flags, as compiler can optimize for these more thoroughly.
        uint runtimeRayflags = RAY_FLAG_NONE;
        
        rayQuery.TraceRayInline(
            g_scene,    // Acceleration structure
            runtimeRayflags,
            ~0,         // Instance mask
            newRay);
#endif

        // Looping over this call is not required since the InlineIS mechanism is used.
        rayQuery.Proceed();

        // Early exit if ray hit nothing.
        if (rayQuery.CommittedStatus() == COMMITTED_NOTHING)
        {
            // If we enter here for the primary ray, BackgroundColor will be assigned to the UAV.
            g_intersectionStack[i].phongColor = BackgroundColor.xyz;
            break;
        }
        lastValidBounce = i;

        g_intersectionStack[i].rayTCurrent = rayQuery.CommittedRayT();
        float3 currentIntersectionPoint = currentRayOrigin + g_intersectionStack[i].rayTCurrent * currentRayDirection;

        uint indexSizeInBytes = 2u;
        uint indicesPerTriangle = 3u;
        uint triangleIndexStride = indexSizeInBytes * indicesPerTriangle;
        uint baseIndex = rayQuery.CommittedPrimitiveIndex() * triangleIndexStride;

        // This corresponds to the shader entry for the point intersected by the ray we just traced.
        uint newShaderTableIndex = rayQuery.CommittedInstanceContributionToHitGroupIndex() + g_rayContributionToHitGroupIndex +
            g_multiplierForGeometryContributionToHitGroupIndex * rayQuery.CommittedGeometryIndex();
        CustomShaderRecordPadded shaderRecord = g_shaderTable[newShaderTableIndex];

        // If current hit's reflectance coefficient is small, break out of the loop.
        float4 currentBounceAlbedo = shaderRecord.materialCb.albedo;
        float currentBounceDiffuseCoef = shaderRecord.materialCb.diffuseCoef;
        float currentBounceSpecularCoef = shaderRecord.materialCb.specularCoef;
        float currentBounceSpecularPower = shaderRecord.materialCb.specularPower;
        float currentReflectanceCoef = shaderRecord.materialCb.reflectanceCoef;

        // Update the value of currentNormal at the intersection point.
        if (rayQuery.CommittedStatus() == COMMITTED_PROCEDURAL_PRIMITIVE_HIT)
        {
            currentNormal = g_normalAttr;
            g_intersectionStack[i].checkers = 1.0f;
        }
        else
        {
            // Load up three 16 bit indices for the triangle and retrieve corresponding vertex normals for the vertices.
            const uint3 indices = Load3x16BitIndices(baseIndex, g_indices);
            currentNormal = g_vertices[indices[0]].normal;

            uint2 dispatchRayDimensions = uint2(g_sceneCB.width, g_sceneCB.height);
            g_intersectionStack[i].checkers = AnalyticalCheckersTexture(currentIntersectionPoint, DTid.xy, dispatchRayDimensions, currentNormal,
                g_sceneCB.cameraPosition.xyz, g_sceneCB.projectionToWorld);
        }

        // Check if intersection point is in shadow.
        bool shadowRayHit = IsPointInShadow(currentIntersectionPoint);

        // Calculate phong color for the current intersection point.
        g_intersectionStack[i].phongColor = CalculatePhongLighting(currentBounceAlbedo, currentIntersectionPoint, currentRayDirection, currentNormal,
            shadowRayHit, currentBounceDiffuseCoef, currentBounceSpecularCoef, currentBounceSpecularPower).xyz;

        // Calculate Fresnel and save it together with reflectance coefficient for the current hit.
        float3 fresnelR = FresnelReflectanceSchlick(currentRayDirection, currentNormal, currentBounceAlbedo.xyz);
        float3 currentFresnelxReflectance = currentReflectanceCoef * fresnelR;
        g_intersectionStack[i].fresnelxReflectance = currentFresnelxReflectance;

        // If current hit's reflectance coefficient is small, break out of the loop.
        if (currentReflectanceCoef < 0.001f)
        {
            break;
        }

        // Update these values for the next ray to be cast.
        currentIncomingRay = currentRayDirection;
        currentRayOrigin = currentIntersectionPoint;
    }

    // Combine reflection contributions and primary ray into a final color.
    [unroll(MAX_RAY_RECURSION_DEPTH)]
    for (int i = lastValidBounce; i >= 0; --i)
    {
        float3 upperLevelColorContribution = g_intersectionStack[i].fresnelxReflectance * g_intersectionStack[i + 1].phongColor;

        // current color is combined with upper level one
        float3 composedColor = g_intersectionStack[i].checkers * (g_intersectionStack[i].phongColor + upperLevelColorContribution);

        // Apply visibility falloff.
        float t = g_intersectionStack[i].rayTCurrent;
        composedColor = lerp(composedColor, BackgroundColor.xyz, 1.0f - exp(-0.000002f * t * t * t));

        g_intersectionStack[i].phongColor = composedColor;
    }

    // Shade the UAV with the final color.
    g_colorBuffer[DTid.xy] = float4(g_intersectionStack[0].phongColor, 1.0f);
}
