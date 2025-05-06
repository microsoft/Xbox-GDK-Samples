#define RootSig \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_AMPLIFICATION_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS | DENY_HULL_SHADER_ROOT_ACCESS | DENY_MESH_SHADER_ROOT_ACCESS )," \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL ), "\
    "RootConstants(num32BitConstants = 4, b0), " \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, visibility = SHADER_VISIBILITY_PIXEL)"

cbuffer RootConstants : register(b0)
{
    float2 offset;
    float2 scale;
};

struct Interpolants
{
    float4 pos : SV_Position;
    float2 uv  : UV0;
};
