#define SkyboxRootSignature \
    "RootFlags(DENY_HULL_SHADER_ROOT_ACCESS | " \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
    "CBV(b0)," \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s0," \
    "filter = FILTER_MIN_MAG_MIP_LINEAR," \
    "addressU = TEXTURE_ADDRESS_CLAMP," \
    "addressV = TEXTURE_ADDRESS_CLAMP," \
    "addressW = TEXTURE_ADDRESS_CLAMP," \
    "visibility = SHADER_VISIBILITY_PIXEL)"

cbuffer SkyboxConstants : register(b0)
{
    float4x4 InverseViewProjection;
    float4 EyePosition;
    float4 EnvironmentParameters;
}

struct SkyboxOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};
