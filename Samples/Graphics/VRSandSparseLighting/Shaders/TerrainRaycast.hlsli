//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"

#define RAYCAST_AO_DISTANCE                 7.5
#define RAYCAST_AO_STEP_SIZE                2.5
#define RAYCAST_AO_MIN_STEP_SIZE            0.5
#define AO_ATTENUATION                      0.5

#define SHADOW_MIN_STEP_SIZE                1.0
#define SHADOW_ATTENUATION                  0.5        // less is softer



float RayTraceTerrain(float3 origin, float3 dir, inout float3 raytraceDebug)
{
    float maxY = worldScaleY * 1.5;                         // TODO better determination of max height
    float light = 1.0;
    float3 pos = origin + dir;                              // first step is free!
    float dist = 1.0;

    while (true)
    {
        float height = LowResMaxHeightSRV.SampleLevel(LinearBorderSampler, PosToUV(pos), 0.0);

        if (0.0 == height)      // off texture
            break;      

        float delta = pos.y - height;
        if (delta < 0.0)
        {
            float fakePenumbra = 0.05 + 0.95 / sqrt(dist);

            light = min(light, 1.0 + SHADOW_ATTENUATION * delta * fakePenumbra);

            if (light <= 0.0)
            {
                raytraceDebug.z = 0.0;
                return 0.0;
            }
        }
        dist += stepSize * max(delta, SHADOW_MIN_STEP_SIZE);
        pos = origin + dist * dir;

        if (pos.y > maxY)
            break;
    }
    raytraceDebug.z = light;
    return light;
}
