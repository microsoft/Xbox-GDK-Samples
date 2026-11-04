#define SmokeParticleRootSignature \
    "RootFlags(DENY_HULL_SHADER_ROOT_ACCESS | " \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
    "CBV(b0)," \
    "SRV(t1, visibility = SHADER_VISIBILITY_VERTEX)," \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t2), visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s0," \
    "filter = FILTER_MIN_MAG_MIP_LINEAR," \
    "addressU = TEXTURE_ADDRESS_CLAMP," \
    "addressV = TEXTURE_ADDRESS_CLAMP," \
    "addressW = TEXTURE_ADDRESS_CLAMP," \
    "visibility = SHADER_VISIBILITY_PIXEL)"

cbuffer SmokeParticleConstants : register(b0)
{
    float4x4 ViewProjection;
    float4 CameraRightAndSizeScale;    // xyz = camera right, w = global size scale
    float4 CameraUpAndTime;            // xyz = camera up, w = elapsed time (seconds)
    float4 EmitterCenterAndLifetime;   // xyz = emitter plane center, w = particle lifetime (seconds)
    float4 EmitterExtentsAndDrift;     // xy = emitter plane half extents (x/z), z = horizontal drift amplitude
    float4 SettleHeightRange;          // x = min ground-layer settle height, y = max settle height (units above emitter plane), z = depth-fade distance
    float4 SizeRange;                  // x = min billboard size, y = max billboard size
    float4 CameraForwardAndNear;       // xyz = camera forward (world space), w = camera near distance
    float4 EyePositionAndFar;          // xyz = camera world position, w = camera far distance
    float4 SpriteSheetGrid;            // x = columns, y = rows, z = frame count, w = per-cell UV inset (avoids bilinear bleed across frames)
}

// Per-frame back-to-front draw order: ParticleOrder[instanceId] gives the actual particle index
// whose spawn/age/position should be computed for this instance. This lets a single instanced
// draw call still render particles in sorted order for correct alpha blending.
StructuredBuffer<uint> ParticleOrder : register(t1);

struct SmokeParticleOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
    float3 WorldPosition : TEXCOORD1;
    float3 TexCoordNextAndBlend : TEXCOORD2;   // xy = next frame's atlas UV, z = blend weight toward it
};
