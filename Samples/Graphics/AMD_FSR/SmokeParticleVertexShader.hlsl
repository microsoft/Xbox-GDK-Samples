#include "SmokeParticleCommon.hlsli"

// Each smoke particle loops continuously: it spawns on the emitter plane, rises, drifts slightly,
// and fades its alpha to zero over its lifetime (the alpha fade-in/fade-out below). The visual
// "puff" shape, driven by an animated sprite sheet (SmokeSpriteSheet.dds: a small young wisp
// evolving into a larger, dissipated cloud), is tied to that same per-particle lifetime fraction
// (age), not to a free-running global clock: the animation plays through exactly once per
// lifetime and reaches its final (most dissipated) frame right as alpha fades to zero. Driving
// both from the same "age" also means each particle's own phase offset (reused from the
// lifetime/respawn logic) naturally desyncs the swarm, so puffs don't all show the same frame at
// once. The pixel shader cross-fades between the current and next frame (see
// TexCoordNextAndBlend below) so the discrete frame steps read as smooth, continuous motion
// instead of a slideshow of stills.

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

[RootSignature(SmokeParticleRootSignature)]
SmokeParticleOutput main(uint vertexId : SV_VertexID, uint rawInstanceId : SV_InstanceID)
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

    const uint instanceId = ParticleOrder[rawInstanceId];
    const float timeElapsed = CameraUpAndTime.w;
    const float lifetime = EmitterCenterAndLifetime.w;
    const float initialPhase = Hash(instanceId ^ 0x9e3779b9u);
    const float lifePosition = timeElapsed / lifetime + initialPhase;
    const float age = frac(lifePosition);
    const uint generation = uint(floor(lifePosition));
    const uint seed = instanceId * 747796405u + generation * 2891336453u;
    const float3 random = Hash3(seed);

    // Spawn uniformly across the emitter plane (the Sponza courtyard footprint).
    const float3 center = EmitterCenterAndLifetime.xyz;
    const float2 extents = EmitterExtentsAndDrift.xy;
    const float2 spawnXZ = center.xz + (random.yz * 2.0f - 1.0f) * extents;

    // Rise quickly to a shallow, per-particle settle height just above the floor, then hold
    // there (ground-hugging fog layer) for the remainder of the particle's life.
    const float settleHeight = lerp(SettleHeightRange.x, SettleHeightRange.y, random.x);
    const float riseFraction = 0.12f;
    const float riseProgress = smoothstep(0.0f, riseFraction, age);
    const float driftAmplitude = EmitterExtentsAndDrift.z;
    const float driftPhase = age * 6.2831853f * 1.5f + random.x * 6.2831853f;
    const float2 drift = driftAmplitude * riseProgress * float2(sin(driftPhase), cos(driftPhase * 0.77f));

    const float3 particlePosition = float3(
        spawnXZ.x + drift.x,
        center.y + settleHeight * riseProgress,
        spawnXZ.y + drift.y);

    // Placeholder "animation": quick fade-in, hold at full opacity, then fade out to zero
    // toward the end of the particle's life (stand-in for a future animated dissipation texture).
    const float fadeIn = smoothstep(0.0f, 0.06f, age);
    const float fadeOutStart = 0.72f;
    const float fadeOut = 1.0f - smoothstep(fadeOutStart, 1.0f, age);
    const float alpha = fadeIn * fadeOut;

    // Smoke billows outward slightly as it settles into the layer.
    const float sizeBase = lerp(SizeRange.x, SizeRange.y, random.z);
    const float size = sizeBase * (1.0f + riseProgress * 0.6f) * CameraRightAndSizeScale.w;

    const float2 corner = corners[vertexId];
    const float3 vertexPosition = particlePosition
        + CameraRightAndSizeScale.xyz * corner.x * size
        + CameraUpAndTime.xyz * corner.y * size;

    SmokeParticleOutput output;
    output.Position = mul(float4(vertexPosition, 1.0f), ViewProjection);

    // Select the sprite-sheet frame/strip from the particle's own lifetime fraction (age), so the
    // puff animation plays through exactly once per lifetime: frame 0 (small, fresh wisp) right
    // as the particle fades in, and the final frame (large, dissipated cloud) right as it fades
    // back out. The quad's local [0,1] UV is then mapped into that frame's cell, inset by a
    // fraction of a texel on each side so bilinear filtering never samples across into an
    // adjacent cell. The current and next frame's UVs are both output so the pixel shader can
    // cross-fade between them, turning the discrete frame steps into a continuous, soft
    // transition instead of a hard per-frame pop.
    const float2 gridSize = SpriteSheetGrid.xy;
    const float frameCount = SpriteSheetGrid.z;
    const float cellInset = SpriteSheetGrid.w;
    const float animationPosition = age * frameCount;
    const float frameBlend = frac(animationPosition);
    const uint frameIndex0 = uint(floor(animationPosition)) % uint(frameCount);
    const uint frameIndex1 = (frameIndex0 + 1u) % uint(frameCount);
    const float2 frameCell0 = float2(frameIndex0 % uint(gridSize.x), frameIndex0 / uint(gridSize.x));
    const float2 frameCell1 = float2(frameIndex1 % uint(gridSize.x), frameIndex1 / uint(gridSize.x));
    const float2 localUv = corner * 0.5f + 0.5f;
    const float2 insetLocalUv = lerp(cellInset, 1.0f - cellInset, localUv);
    output.TexCoord = (frameCell0 + insetLocalUv) / gridSize;
    output.TexCoordNextAndBlend = float3((frameCell1 + insetLocalUv) / gridSize, frameBlend);

    output.Color = float4(0.55f, 0.55f, 0.58f, alpha);
    output.WorldPosition = vertexPosition;
    return output;
}
