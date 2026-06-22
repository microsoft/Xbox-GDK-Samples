//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Global.hlsli"

#define RAYCAST_AO_DISTANCE                 7.5
#define RAYCAST_AO_STEP_SIZE                2.5
#define RAYCAST_AO_MIN_STEP_SIZE            0.5
#define AO_ATTENUATION                      0.5

#define SHADOW_MIN_STEP_SIZE                1.0
#define SHADOW_ATTENUATION                  0.5        // less is softer
#define MIN_SHADOW_TERM                     0.1         // stop iterating at ambient, not black, fake GI



float RayTraceTerrain(float3 origin, float3 dir, inout float3 raytraceDebug)
{
    float maxY = worldScaleY * 1.5;                         // TODO better determination of max height
    float light = 1.0;
    float3 pos = origin + dir;                              // first step is free!
    float dist = 1.0;

    while (true)
    {
        float height = LowResMaxHeightSRV.Sample(LinearBorderSampler, PosToUV(pos));

        if (0.0 == height)      // off texture
            break;      

        float delta = pos.y - height;
        if (delta < 0.0)
        {
            float fakePenumbra = 0.05 + 0.95 / sqrt(dist);

            light = min(light, 1.0 + SHADOW_ATTENUATION * delta * fakePenumbra);

            if (light <= MIN_SHADOW_TERM)
            {
                raytraceDebug.z = MIN_SHADOW_TERM;
                return MIN_SHADOW_TERM;
            }
        }
        // Less VALU to advance pos, rather than recalculate it every time from origin, this likely has more drift in it, but given how soft the shadows are that's fine
        dist += stepSize * max(delta, SHADOW_MIN_STEP_SIZE);
        pos = origin + dist * dir;

        if (pos.y > maxY)
            break;
    }
    raytraceDebug.z = light;
    return light;
}


float RayTraceTerrainMaxDist(float3 origin, float3 dir, float maxDist)
{
    // normalized in xz only
    dir /= sqrt(dot(dir.xz, dir.xz));
    
    float light = 1.0;
    float dist = 1.0;            

    while (true)
    {
        float3 pos = origin + dist * dir;
        float height = HeightMapSRV.Sample(LinearBorderSampler, PosToUV(pos));
        float delta = pos.y - height;
        if (delta < 0.0)
        {
            light += AO_ATTENUATION * delta;

            if (light <= 0.0)
            {
                return 0.0;
            }
        }
        dist += RAYCAST_AO_STEP_SIZE * max(delta, RAYCAST_AO_MIN_STEP_SIZE);
        
        if (dist > maxDist)
            break;
    }
    return light;
}


float CalculateAO(float3 pos, float3 normal, out float3 debug)
{
    // remove flicker due to LOD changes
    pos.y = HeightMapSRV.Sample(LinearClampSampler, PosToUV(pos));

    float ao = 0.0;

    int axisStepsXZ = 3;
    int axisStepsY = 3;
    int numSamples = axisStepsXZ * axisStepsXZ * axisStepsY;
    int x, y, z;

    float iStepsXZ = 1.0 / float(axisStepsXZ - 1);
    float iStepsY = 1.0 / float(axisStepsY - 1);
    float total = 0.0;

    for (z = 0; z < axisStepsXZ; ++z)
    {
        float fz = (float(z) * iStepsXZ) - 0.5;

        for (y = 0; y < axisStepsY; ++y)
        {
            float fy = (float(y) * iStepsY) + 0.1;

            for (x = 0; x < axisStepsXZ; ++x)
            {
                float fx = (float(x) * iStepsXZ) - 0.5;

                float3 dir = float3(fx, fy, fz);

                dir.xz *= (dot(normal, dir) > 0.0) ? 1.0 : -1.0;

                float localAo = RayTraceTerrainMaxDist(pos, dir, RAYCAST_AO_DISTANCE);

                float sum = dot(normal, normalize(dir));
                sum = pow(sum, 1.0 / 3.0);

                ao += localAo * sum;
                total += sum;
            }
        }
    }
    ao /= total;
    ao = pow(ao, 3.0);
    debug = ao;
    return ao;
}
