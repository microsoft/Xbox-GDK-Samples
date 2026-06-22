//--------------------------------------------------------------------------------------
// VolumetricPrimitives.hlsli
//
// Ray marching of Metaballs (aka "Blobs").
// More info here: https://www.scratchapixel.com/lessons/advanced-rendering/rendering-distance-fields/blobbies
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "RaytracingShaderHelper.hlsli"

struct Metaball
{
    float3 center;
    float  radius;
};

// Calculate a magnitude of an influence from a Metaball charge.
// Return metaball potential range: <0,1>
// mbRadius - largest possible area of metaball contribution - AKA its bounding sphere.
float CalculateMetaballPotential(in float3 position, in Metaball blob, out float distance)
{
    distance = length(position - blob.center);
    
    if (distance <= blob.radius)
    {
        float d = distance;

        // Quintic polynomial field function.
        // The advantage of this polynomial is having smooth second derivative. Not having a smooth
        // second derivative may result in a sharp and visually unpleasant normal vector jump.
        // The field function should return 1 at distance 0 from a center, and 1 at radius distance,
        // but this one gives f(0) = 0, f(radius) = 1, so we use the distance to radius instead.
        d = blob.radius - d;

        float r = blob.radius;
        return 6.0f * (d*d*d*d*d) / (r*r*r*r*r)
            - 15.0f * (d*d*d*d) / (r*r*r*r)
            + 10.0f * (d*d*d) / (r*r*r);
    }
    return 0;
}

// Calculate field potential from all active metaballs.
float CalculateMetaballsPotential(in float3 position, in Metaball blobs[N_METABALLS], in uint nActiveMetaballs)
{
    float sumFieldPotential = 0.0f;
    for (uint j = 0; j < N_METABALLS; ++j)
    {
        float dummy;
        sumFieldPotential += CalculateMetaballPotential(position, blobs[j], dummy);
    }
    return sumFieldPotential;
}

// Calculate a normal via central differences.
float3 CalculateMetaballsNormal(in float3 position, in Metaball blobs[N_METABALLS], in uint nActiveMetaballs)
{
    float e = 0.5773f * 0.00001f;
    return normalize(float3(
        CalculateMetaballsPotential(position + float3(-e, 0.0f, 0.0f), blobs, nActiveMetaballs) -
        CalculateMetaballsPotential(position + float3(e, 0.0f, 0.0f), blobs, nActiveMetaballs),
        CalculateMetaballsPotential(position + float3(0.0f, -e, 0.0f), blobs, nActiveMetaballs) -
        CalculateMetaballsPotential(position + float3(0.0f, e, 0.0f), blobs, nActiveMetaballs),
        CalculateMetaballsPotential(position + float3(0.0f, 0.0f, -e), blobs, nActiveMetaballs) -
        CalculateMetaballsPotential(position + float3(0.0f, 0.0f, e), blobs, nActiveMetaballs)));
}

void InitializeAnimatedMetaballs(out Metaball blobs[N_METABALLS], in float elapsedTime, in float cycleDuration)
{
    // Metaball centers at t0 and t1 key frames.
    float3 keyFrameCenters[N_METABALLS][2] =
    {
        { float3(-0.7f, 0.0f, 0.0f),float3(0.7f, 0.0f, 0.0f) },
        { float3(0.7f, 0.0f, 0.0f), float3(-0.7f, 0.0f, 0.0f) },
        { float3(0.0f, -0.7f, 0.0f),float3(0.0f, 0.7f, 0.0f) },
        { float3(0.0f, 0.7f, 0.0f), float3(0.0f, -0.7f, 0.0f) },
        { float3(0.0f, 0.0f, 0.0f),   float3(0.0f, 0.0f, 0.0f) }
    };
    // Metaball field radii of max influence
    float radii[N_METABALLS] = { 0.35f, 0.35f, 0.35f, 0.35f, 0.25f };

    // Calculate animated metaball center positions.
    float  tAnimate = CalculateAnimationInterpolant(elapsedTime, cycleDuration);
    for (uint j = 0; j < N_METABALLS; ++j)
    {
        blobs[j].center = lerp(keyFrameCenters[j][0], keyFrameCenters[j][1], tAnimate);
        blobs[j].radius = radii[j];
    }
}

// Find all metaballs that ray intersects.
// The passed in array is sorted to the first nActiveMetaballs.
void FindIntersectingMetaballs(in Ray ray, in float rayTMin, in float rayTCurrent, out float tmin, out float tmax, inout Metaball blobs[N_METABALLS], out uint nActiveMetaballs)
{
    // Find the entry and exit points for all metaball bounding spheres combined.
    tmin = INFINITY;
    tmax = -INFINITY;

    nActiveMetaballs = 0;
    for (uint i = 0; i < N_METABALLS; ++i)
    {
        float _thit, _tmax;
        if (RaySolidSphereIntersectionTest(ray, _thit, _tmax, rayTMin, rayTCurrent, blobs[i].center, blobs[i].radius))
        {
            tmin = min(_thit, tmin);
            tmax = max(_tmax, tmax);
            nActiveMetaballs = N_METABALLS;
        }
    }
    tmin = max(tmin, rayTMin);
    tmax = min(tmax, rayTCurrent);
}

// Test if a ray with RayFlags and segment <RayTMin, RayTCurrent> intersects metaball field.
// The test sphere traces through the metaball field until it hits a threshold isosurface.
bool RayMetaballsIntersectionTest(in Ray ray, out float thit, in uint rayFlags, in float rayTMin, in float rayTCurrent, out ProceduralPrimitiveAttributes attr, in float elapsedTime)
{
    Metaball blobs[N_METABALLS];
    InitializeAnimatedMetaballs(blobs, elapsedTime, 12.0f);
    
    float tmin, tmax;   // Ray extents to first and last metaball intersections.
    uint nActiveMetaballs = 0;  // Number of metaballs's that the ray intersects.
    FindIntersectingMetaballs(ray, rayTMin, rayTCurrent, tmin, tmax, blobs, nActiveMetaballs);

    uint MAX_STEPS = 128;
    float t = tmin;
    float minTStep = (tmax - tmin) / (MAX_STEPS);
    uint iStep = 0;

    while (iStep++ < MAX_STEPS)
    {
        float3 position = ray.origin + t * ray.direction;
        float fieldPotentials[N_METABALLS]; // Field potentials for each metaball.
        float sumFieldPotential = 0.0f;     // Sum of all metaball field potentials.
            
        // Calculate field potentials from all metaballs.
        for (uint j = 0; j < N_METABALLS; j++)
        {
            float distance;
            fieldPotentials[j] = CalculateMetaballPotential(position, blobs[j], distance);
            sumFieldPotential += fieldPotentials[j];
         }

        // Field potential threshold defining the isosurface.
        // Threshold - valid range is (0, 1>, the larger the threshold the smaller the blob.
        const float Threshold = 0.25f;

        // Have we crossed the isosurface?
        if (sumFieldPotential >= Threshold)
        {
            float3 normal = CalculateMetaballsNormal(position, blobs, nActiveMetaballs);
            if (IsAValidHit(ray, t, normal, rayFlags, rayTMin, rayTCurrent))
            {
                thit = t;
                attr.normal = normal;
                return true;
            }
        }
        t += minTStep;
    }

    thit = -1.0f;
    return false;
}
