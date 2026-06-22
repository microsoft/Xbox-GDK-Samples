//--------------------------------------------------------------------------------------
//
// SignedDistanceFractals.hlsli
//
// Set of signed distance fractal tests.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "RaytracingShaderHelper.hlsli"
#include "SignedDistancePrimitives.hlsli"

//------------------------------------------------------------------

// Returns a signed distance to a recursive pyramid fractal.
// h = { sin a, cos a, height of a pyramid}.
// a = pyramid's inner angle between its side plane and a ground plane.
// Pyramid position - sitting on a ground plane.
// Pyramid span: {<-a,0,-a>, <a,h.z,a>}, where a = width of base = h.z * h.y / h.x.
// More info here http://blog.hvidtfeldts.net/index.php/2011/08/distance-estimated-3d-fractals-iii-folding-space/
float sdFractalPyramid(in float3 position, float3 h, in float Scale = 2.0f)
{
    // Set pyramid vertices to AABB's extremities.
    float a = h.z * h.y / h.x;
    float3 v1 = float3(0.0f, h.z, 0.0f);
    float3 v2 = float3(-a, 0.0f, a);
    float3 v3 = float3(a, 0.0f, -a);
    float3 v4 = float3(a, 0.0f, a);
    float3 v5 = float3(-a, 0.0f, -a);

    int n = 0;
    for (n = 0; n < N_FRACTAL_ITERATIONS; n++)
    {
        // Find the closest vertex.
        float dist, d;
        float3 v;
        v = v1; dist = length_toPow2(position - v1); 
        d = length_toPow2(position - v2); if (d < dist) { v = v2; dist = d; }
        d = length_toPow2(position - v3); if (d < dist) { v = v3; dist = d; }
        d = length_toPow2(position - v4); if (d < dist) { v = v4; dist = d; }
        d = length_toPow2(position - v5); if (d < dist) { v = v5; dist = d; }

        // Update to a relative position in the current fractal iteration.
        position = Scale * position - v * (Scale - 1.0);
    }
    float distance = sdPyramid(position, h);

    // Convert the distance from within a fractal iteration to the object space.
    return distance * pow(Scale, float(-n));
}
