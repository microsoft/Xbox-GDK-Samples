#define RootSig \
    "RootFlags(DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
    "RootConstants(num32BitConstants = 3, b0)," \
    "DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(Sampler(s0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)"

cbuffer RootConstants : register(b0)
{
    float2  g_quadScale;
    uint    g_mipLevel;
};

struct Interpolants
{
    float4  pos     : SV_Position;
    float2  uv      : UV0;
};
