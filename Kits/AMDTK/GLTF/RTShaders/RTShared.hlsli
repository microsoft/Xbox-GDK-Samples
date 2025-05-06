//--------------------------------------------------------------------------------------
// RTShared.hlsli
//
// HLSL file containing shared DXR simple RT shadow implementation.
//
// Modifications Copyright (C) 2020. Advanced Micro Devices, Inc. All Rights Reserved.
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
  
RaytracingAccelerationStructure Scene : register(t0);

StructuredBuffer<uint> SobolBuffer : register(t1);
StructuredBuffer<uint> ScramblingTileBuffer : register(t2);
Texture2D DepthBuffer : register(t3);
Texture2D NormalBuffer : register(t4);

RWTexture2D<float> renderOutput : register(u0);

cbuffer RTShadowParams : register(b0)
{
    matrix CameraViewProjInverse;
    uint DispatchWidth;
    uint DispatchHeight;
    float DispatchWidthInverse;
    float DispatchHeightInverse;
    float4 LightPos;
    float LightSize;
    uint FrameNum;
}

struct ShadowRayPayload
{
    bool hitlight;
};

#define PI              3.14159265359f
#define GOLDEN_RATIO    1.61803398875f


float3 OrthoVector(in float3 n)
{
    float3 p;

    if (abs(n.z) > 0.0f)
    {
        float k = sqrt(n.y * n.y + n.z * n.z);
        p.x = 0.0f;
        p.y = -n.z / k;
        p.z = n.y / k;
    }
    else
    {
        float k = sqrt(n.x * n.x + n.y * n.y);
        p.x = n.y / k;
        p.y = -n.x / k;
        p.z = 0.0f;
    }

    return p;
}

float3 MapToCone(in float2 s, in float3 p, in float3 n, in float radius)
{
    float3 u = OrthoVector(n);
    const float3 v = cross(u, n);
    u = cross(n, v);

    const float2 offset = 2.0f * s - float2(1.0f, 1.0f);

    if (offset.x == 0.0f && offset.y == 0.0f)
    {
        return p;
    }

    float theta, r;

    if (abs(offset.x) > abs(offset.y))
    {
        r = offset.x;
        theta = PI / 4.0f * (offset.y / offset.x);
    }
    else
    {
        r = offset.y;
        theta = PI / 2.0f * (1.0f - 0.5f * (offset.x / offset.y));
    }

    const float2 uv = float2(radius * r * cos(theta), radius * r * sin(theta));

    return p + uv.x * u + uv.y * v;
}

float Sample(in uint pixel_i, in uint pixel_j, in uint sample_index, in uint sample_dimension)
{
    // Wrap arguments
    pixel_i = pixel_i & 127u;
    pixel_j = pixel_j & 127u;
    sample_index = sample_index & 255u;
    sample_dimension = sample_dimension & 255u;

    // xor index based on optimized ranking
    const uint ranked_sample_index = sample_index ^ 0; // For one sample per pixel RankingTileBuffer will always return 0.

    // Fetch value in sequence
    uint value = SobolBuffer[sample_dimension + ranked_sample_index * 256u];

    // If the dimension is optimized, xor sequence value based on optimized scrambling
    value = value ^ ScramblingTileBuffer[(sample_dimension % 8u) + (pixel_i + pixel_j * 128u) * 8u];

    // Convert to float and return
    return (value + 0.5f) / 256.0f;
}

void FillRayDesc(in uint2 did, in float3 world_position, in float3 normal, out RayDesc rayDesc)
{
    // Set the ray's extents.
    float RayPushoffAmount = 0.001f;
    rayDesc.Origin = world_position + normal * RayPushoffAmount;

    float3 light_direction = LightPos.xyz - world_position;
    const float distance_to_light = length(light_direction);

        // Use golden ratio to animate the noise over time:
        // https://blog.demofox.org/2017/10/31/animating-noise-for-integration-over-time/

    const float2 s = float2(
            fmod(Sample(did.x, did.y, 0, 0u) + (FrameNum & 0xFFu) * GOLDEN_RATIO, 1.0f),
            fmod(Sample(did.x, did.y, 0, 1u) + (FrameNum & 0xFFu) * GOLDEN_RATIO, 1.0f));

    rayDesc.Direction = normalize(MapToCone(s, LightPos.xyz, normalize(light_direction), LightSize) - world_position);

        // Set TMin to a zero value to avoid aliasing artifacts along contact areas.
        // Note: make sure to enable back-face culling so as to avoid surface face fighting.
    rayDesc.TMin = 0.0001f;
    rayDesc.TMax = distance_to_light;
}
