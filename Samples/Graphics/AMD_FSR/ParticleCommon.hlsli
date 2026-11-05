#define ParticleRootSignature \
    "RootFlags(DENY_HULL_SHADER_ROOT_ACCESS | " \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
    "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX)"

cbuffer ParticleConstants : register(b0)
{
    float4x4 ViewProjection;
    float4 CameraRightAndSize;
    float4 CameraUpAndTime;
    float4 VolumeCenterAndLifetime;
    float4 VolumeExtents;
    float4 SpeedRange;
}

struct ParticleOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
};
