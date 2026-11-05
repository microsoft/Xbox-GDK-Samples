#include "ParticleCommon.hlsli"

float Hash(uint value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return float(value & 0x00ffffffu) / 16777216.0f;
}

float3 Hash3(uint value)
{
    return float3(
        Hash(value),
        Hash(value ^ 0x68bc21ebu),
        Hash(value ^ 0x02e5be93u));
}

[RootSignature(ParticleRootSignature)]
ParticleOutput main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    static const float2 corners[6] =
    {
        float2(-1.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(-1.0f, -1.0f),
        float2(-1.0f, -1.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, -1.0f)
    };

    const float time = CameraUpAndTime.w;
    const float lifetime = VolumeCenterAndLifetime.w;
    const float initialPhase = Hash(instanceId ^ 0x9e3779b9u);
    const float lifePosition = time / lifetime + initialPhase;
    const float age = frac(lifePosition);
    const uint generation = uint(floor(lifePosition));
    const uint seed = instanceId * 747796405u + generation * 2891336453u;
    const float3 random = Hash3(seed);
    const float speed = lerp(
        SpeedRange.x,
        SpeedRange.y,
        Hash(seed ^ 0xb5297a4du));
    const float particleTime = time * speed;
    const float3 center = VolumeCenterAndLifetime.xyz;
    const float3 extents = VolumeExtents.xyz;

    const float3 spawnPosition = center + (random * 2.0f - 1.0f) * extents * 0.55f;
    const float3 flockCenter = center + extents * float3(
        sin(time * 0.23f) * 0.25f,
        sin(time * 0.31f + 1.7f) * 0.12f,
        cos(time * 0.19f) * 0.25f);
    const float phase = particleTime + random.y * 6.2831853f;
    const float3 orbit = extents * float3(
        sin(phase + random.z * 3.0f) * 0.16f,
        sin(phase * 1.37f + random.x * 4.0f) * 0.08f,
        cos(phase * 0.83f + random.y * 5.0f) * 0.16f);
    const float cohesion = 0.25f + 0.12f * sin(phase * 0.47f);
    float3 particlePosition = lerp(spawnPosition, flockCenter, cohesion) + orbit;
    particlePosition = clamp(particlePosition, center - extents, center + extents);

    const float fadeIn = smoothstep(0.0f, 0.08f, age);
    const float fadeOut = 1.0f - smoothstep(0.78f, 1.0f, age);
    const float alpha = fadeIn * fadeOut;
    const float size = CameraRightAndSize.w * lerp(0.65f, 1.35f, random.z);
    const float2 corner = corners[vertexId];
    const float3 vertexPosition = particlePosition
        + CameraRightAndSize.xyz * corner.x * size
        + CameraUpAndTime.xyz * corner.y * size;

    const float warmth = Hash(instanceId ^ 0xa511e9b3u);
    const float3 color = lerp(
        float3(0.45f, 0.75f, 1.0f),
        float3(1.0f, 0.72f, 0.25f),
        warmth);

    ParticleOutput output;
    output.Position = mul(float4(vertexPosition, 1.0f), ViewProjection);
    output.TexCoord = corner * 0.5f + 0.5f;
    output.Color = float4(color, alpha);
    return output;
}
