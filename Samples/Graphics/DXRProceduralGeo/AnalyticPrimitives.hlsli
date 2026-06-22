//--------------------------------------------------------------------------------------
//
// AnalyticPrimitives.hlsli
//
// Set of ray vs analytic primitive intersection tests.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "RaytracingShaderHelper.hlsli"

// Solve a quadratic equation.
// Ref: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
bool SolveQuadraticEqn(float a, float b, float c, out float x0, out float x1)
{
    float discr = b * b - 4.0f * a * c;
    if (discr < 0.0f)
    {
        x0 = x1 = -1.0f;
        return false;
    }
    else if (discr == 0.0f)
    {
        x0 = x1 = -0.5f * b / a;
    }
    else
    {
        float q = (b > 0.0f) ? -0.5f * (b + sqrt(discr)) : -0.5f * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }

    if (x0 > x1)
    {
        swap(x0, x1);
    }

    return true;
}

// Calculate a normal for a hit point on a sphere.
float3 CalculateNormalForARaySphereHit(in Ray ray, in float thit, float3 center)
{
    float3 hitPosition = ray.origin + thit * ray.direction;
    return normalize(hitPosition - center);
}

// Analytic solution of an unbounded ray sphere intersection points.
// Ref: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
bool SolveRaySphereIntersectionEquation(in Ray ray, out float tmin, out float tmax, in float3 center, in float radius)
{
    float3 L = ray.origin - center;
    float a = dot(ray.direction, ray.direction);
    float b = 2 * dot(ray.direction, L);
    float c = dot(L, L) - radius * radius;
    return SolveQuadraticEqn(a, b, c, tmin, tmax);
}

// Test if a ray with RayFlags and segment <RayTMin, RayTCurrent> intersects a hollow sphere.
bool RaySphereIntersectionTest(in Ray ray, out float thit, out float tmax, in uint rayFlags, in float rayTMin, in float rayTCurrent, out ProceduralPrimitiveAttributes attr, in float3 center = float3(0, 0, 0), in float radius = 1)
{
    // solutions for t if the ray intersects 
    float t0, t1;

    if (!SolveRaySphereIntersectionEquation(ray, t0, t1, center, radius))
    {
        thit = tmax = -1.0f;
        return false;
    }

    tmax = t1;

    if (t0 < rayTMin)
    {
        // t0 is before RayTMin, let's use t1 instead
        if (t1 < rayTMin)
        {
            thit = tmax = -1.0f;
            return false; // both t0 and t1 are before RayTMin
        }

        attr.normal = CalculateNormalForARaySphereHit(ray, t1, center);
        if (IsAValidHit(ray, t1, attr.normal, rayFlags, rayTMin, rayTCurrent))
        {
            thit = t1;
            return true;
        }
    }
    else
    {
        attr.normal = CalculateNormalForARaySphereHit(ray, t0, center);
        if (IsAValidHit(ray, t0, attr.normal, rayFlags, rayTMin, rayTCurrent))
        {
            thit = t0;
            return true;
        }

        attr.normal = CalculateNormalForARaySphereHit(ray, t1, center);
        if (IsAValidHit(ray, t1, attr.normal, rayFlags, rayTMin, rayTCurrent))
        {
            thit = t1;
            return true;
        }
    }

    thit = tmax = -1.0f;
    return false;
}

// Test if a ray segment <RayTMin, RayTCurrent> intersects a solid sphere.
// Limitation: this test does not take RayFlags into consideration and does not calculate a surface normal.
bool RaySolidSphereIntersectionTest(in Ray ray, out float thit, out float tmax, in float rayTMin, in float rayTCurrent, in float3 center = float3(0.0f, 0.0f, 0.0f), in float radius = 1.0f)
{
    // solutions for t if the ray intersects 
    float t0, t1;

    if (!SolveRaySphereIntersectionEquation(ray, t0, t1, center, radius))
    {
        thit = tmax = -1.0f;
        return false;
    }

    // Since it's a solid sphere, clip intersection points to ray extents.
    thit = max(t0, rayTMin);
    tmax = min(t1, rayTCurrent);

    return true;
}

// Test if a ray with RayFlags and segment <RayTMin, RayTCurrent> intersects multiple hollow spheres.
bool RaySpheresIntersectionTest(in Ray ray, out float thit, in uint rayFlags, in float rayTMin, in float rayTCurrent, out ProceduralPrimitiveAttributes attr)
{
    const int N = 3;
    float3 centers[N] =
    {
        float3(-0.3f, -0.3f, -0.3f),
        float3(0.1f, 0.1f, 0.4f),
        float3(0.35f,0.35f, 0.0f)
    };
    float  radii[N] = { 0.6f, 0.3f, 0.15f };
    bool hitFound = false;

    // Test for intersection against all spheres and take the closest hit.
    thit = rayTCurrent;

    // test against all spheres
    for (int i = 0; i < N; i++)
    {
        float _thit;
        float _tmax;
        ProceduralPrimitiveAttributes _attr = (ProceduralPrimitiveAttributes)0;
        if (RaySphereIntersectionTest(ray, _thit, _tmax, rayFlags, rayTMin, rayTCurrent, _attr, centers[i], radii[i]))
        {
            if (_thit < thit)
            {
                thit = _thit;
                attr = _attr;
                hitFound = true;
            }
        }
    }
    return hitFound;
}

// Test if a ray segment <RayTMin, RayTCurrent> intersects an AABB.
// Limitation: this test does not take RayFlags into consideration and does not calculate a surface normal.
// Ref: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool RayAABBIntersectionTest(Ray ray, float3 aabb[2], in uint rayFlags, in float rayTMin, in float rayTCurrent, out float tmin, out float tmax)
{
    float3 tmin3, tmax3;
    int3 sign3 = ray.direction > 0;

    // Handle rays parallel to any x|y|z slabs of the AABB. If a ray is within the parallel slabs, 
    // the tmin, tmax will get set to -inf and +inf which will get ignored on tmin/tmax = max/min.
    // If a ray is outside the parallel slabs, -inf/+inf will make tmax > tmin fail (i.e. no intersection).
    // TODO: handle cases where ray origin is within a slab that a ray direction is parallel to.
    //       In that case 0 * INF => NaN
    const float FLT_INFINITY = 1.#INF;

    float3 invRayDirection = select(ray.direction != 0.0f, 1.0f / ray.direction, select(ray.direction > 0.0f, FLT_INFINITY, -FLT_INFINITY));

    tmin3.x = (aabb[1 - sign3.x].x - ray.origin.x) * invRayDirection.x;
    tmax3.x = (aabb[sign3.x].x - ray.origin.x) * invRayDirection.x;

    tmin3.y = (aabb[1 - sign3.y].y - ray.origin.y) * invRayDirection.y;
    tmax3.y = (aabb[sign3.y].y - ray.origin.y) * invRayDirection.y;
    
    tmin3.z = (aabb[1 - sign3.z].z - ray.origin.z) * invRayDirection.z;
    tmax3.z = (aabb[sign3.z].z - ray.origin.z) * invRayDirection.z;
    
    tmin = max(max(tmin3.x, tmin3.y), tmin3.z);
    tmax = min(min(tmax3.x, tmax3.y), tmax3.z);
    
    return tmax > tmin && tmax >= rayTMin && tmin <= rayTCurrent;
}

// Test if a ray with RayFlags and segment <RayTMin, RayTCurrent> intersects a hollow AABB.
bool RayAABBIntersectionTest(Ray ray, float3 aabb[2], out float thit, in uint rayFlags, in float rayTMin, in float rayTCurrent, out ProceduralPrimitiveAttributes attr)
{
    float tmin, tmax;
    if (RayAABBIntersectionTest(ray, aabb, rayFlags, rayTMin, rayTCurrent, tmin, tmax))
    {
        // Only consider intersections crossing the surface from the outside.
        if (tmin < rayTMin || tmin > rayTCurrent)
        {
            thit = tmax = -1.0f;
            return false;
        }

        thit = tmin;

        // Set a normal to the normal of a face the hit point lays on.
        float3 hitPosition = ray.origin + thit * ray.direction;
        float3 distanceToBounds[2] = { abs(aabb[0] - hitPosition), abs(aabb[1] - hitPosition) };
        const float eps = 0.0001f;

        if (distanceToBounds[0].x < eps) attr.normal = float3(-1.0f, 0.0f, 0.0f);
        else if (distanceToBounds[0].y < eps) attr.normal = float3(0.0f, -1.0f, 0.0f);
        else if (distanceToBounds[0].z < eps) attr.normal = float3(0.0f, 0.0f, -1.0f);
        else if (distanceToBounds[1].x < eps) attr.normal = float3(1.0f, 0.0f, 0.0f);
        else if (distanceToBounds[1].y < eps) attr.normal = float3(0.0f, 1.0f, 0.0f);
        else if (distanceToBounds[1].z < eps) attr.normal = float3(0.0f, 0.0f, 1.0f);

        return IsAValidHit(ray, thit, attr.normal, rayFlags, rayTMin, rayTCurrent);
    }

    thit = tmax = -1.0f;
    return false;
}
