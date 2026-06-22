//--------------------------------------------------------------------------------------
//
// ProceduralPrimitivesLibrary.hlsli
//
// An interface to call per geometry intersection tests based on as primitive type.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "RaytracingShaderHelper.hlsli"

#include "AnalyticPrimitives.hlsli"
#include "VolumetricPrimitives.hlsli"
#include "SignedDistancePrimitives.hlsli"
#include "SignedDistanceFractals.hlsli"

// Analytic geometry intersection test.
// AABB local space dimensions: <-1,1>.
bool RayAnalyticGeometryIntersectionTest(in Ray ray, in uint rayFlags, in float rayTMin, in float rayTCurrent, in AnalyticPrimitive::Enum analyticPrimitive, out float thit, out ProceduralPrimitiveAttributes attr)
{
    float3 aabb[2] = {
        float3(-1.0f, -1.0f, -1.0f),
        float3(1.0f, 1.0f, 1.0f)
    };

    float tmax;

    switch (analyticPrimitive)
    {
    case AnalyticPrimitive::AABB: return RayAABBIntersectionTest(ray, aabb, thit, rayFlags, rayTMin, rayTCurrent, attr);
    case AnalyticPrimitive::Spheres: return RaySpheresIntersectionTest(ray, thit, rayFlags, rayTMin, rayTCurrent, attr);
    default:
        thit = -1.0f;
        return false;
    }
}

// Analytic geometry intersection test.
// AABB local space dimensions: <-1,1>.
bool RayVolumetricGeometryIntersectionTest(in Ray ray, in uint rayFlags, in float rayTMin, in float rayTCurrent, in VolumetricPrimitive::Enum volumetricPrimitive, out float thit, out ProceduralPrimitiveAttributes attr, in float elapsedTime)
{
    switch (volumetricPrimitive)
    {
    case VolumetricPrimitive::Metaballs:
        return RayMetaballsIntersectionTest(ray, thit, rayFlags, rayTMin, rayTCurrent, attr, elapsedTime);
    default:
        thit = -1.0f;
        return false;
    }
}

// Signed distance functions use a shared ray signed distance test.
// The test, instead, calls into this function to retrieve a distance for a primitive.
// AABB local space dimensions: <-1,1>.
// Ref: http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float GetDistanceFromSignedDistancePrimitive(in float3 position, in SignedDistancePrimitive::Enum signedDistancePrimitive)
{
    switch (signedDistancePrimitive)
    {
    case SignedDistancePrimitive::MiniSpheres:
        return opI(sdSphere(opRep(position + 1.0f, (float3) 2.0f / 4.0f), 0.65f / 4.0f), sdBox(position, (float3)1.0f));

    case SignedDistancePrimitive::IntersectedRoundCube:
        return opS(opS(udRoundBox(position, (float3) 0.75f, 0.2f), sdSphere(position, 1.20f)), -sdSphere(position, 1.32f));

    case SignedDistancePrimitive::SquareTorus: 
        return sdTorus82(position, float2(0.75f, 0.15f));
    
    case SignedDistancePrimitive::TwistedTorus: 
        return sdTorus(opTwist(position), float2(0.6f, 0.2f));
        
    case SignedDistancePrimitive::Cog:
        return opS( sdTorus82(position, float2(0.60f, 0.3f)),
                    sdCylinder(opRep(float3(atan2(position.z, position.x) / 6.2831f, 
                                            1.0f,
                                            0.015f + 0.25f * length(position)) + 1.0f,
                                     float3(0.05f, 1.0f, 0.075f)),
                               float2(0.02f, 0.8f)));
    
    case SignedDistancePrimitive::Cylinder: 
        return opI(sdCylinder(opRep(position + float3(1.0f, 1.0f, 1.0f), float3(1.0f, 2.0f, 1.0f)), float2(0.3f, 2.0f)),
                   sdBox(position + float3(1.0f, 1.0f, 1.0f), float3(2.0f, 2.0f, 2.0f)));
    
    case SignedDistancePrimitive::FractalPyramid: 
         // Let pyramid have a base at y == -1 of AABB => position + float3(0,1,0) 
         // Pyramid: 63.435 degrees at base, height 2
         return sdFractalPyramid(position + float3(0.0f, 1.0f, 0.0f), float3(0.894f, 0.447f, 2.0f), 2.0f);
    
    default: return 0.0f;
    }
}
