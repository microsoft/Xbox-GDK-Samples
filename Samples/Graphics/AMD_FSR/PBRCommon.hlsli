#define PBRRootSignature \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
    "DENY_HULL_SHADER_ROOT_ACCESS | " \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
    "CBV(b0)," \
    "CBV(b1)," \
    "CBV(b2)," \
    "CBV(b3)," \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t1), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t2), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t3), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t4), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t5), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t6), visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s0," \
    "filter = FILTER_ANISOTROPIC," \
    "addressU = TEXTURE_ADDRESS_WRAP," \
    "addressV = TEXTURE_ADDRESS_WRAP," \
    "addressW = TEXTURE_ADDRESS_WRAP," \
    "maxAnisotropy = 8," \
    "visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s1," \
    "filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT," \
    "addressU = TEXTURE_ADDRESS_BORDER," \
    "addressV = TEXTURE_ADDRESS_BORDER," \
    "addressW = TEXTURE_ADDRESS_BORDER," \
    "borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE," \
    "comparisonFunc = COMPARISON_LESS_EQUAL," \
    "visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s2," \
    "filter = FILTER_MIN_MAG_MIP_LINEAR," \
    "addressU = TEXTURE_ADDRESS_CLAMP," \
    "addressV = TEXTURE_ADDRESS_CLAMP," \
    "addressW = TEXTURE_ADDRESS_CLAMP," \
    "visibility = SHADER_VISIBILITY_PIXEL)"

cbuffer SceneConstants : register(b0)
{
    float4x4 ViewProjection;
    float4x4 LightViewProjection;
    float4 EyePosition;
    float4 LightDirection;
    float4 LightColor;
    float4 EnvironmentParameters;
    float4x4 PreviousViewProjection;
}

cbuffer MeshConstants : register(b1)
{
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4 BaseColorFactor;
    float4 MaterialFactors;
    float4x4 PreviousWorld;
}

struct VSInput
{
    float3 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 PositionWS : TEXCOORD0;
    float3 NormalWS : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    float4 ShadowPosition : TEXCOORD3;
};

// Output of the motion-vectors vertex shaders: the same position is carried through twice, once
// transformed with this frame's world/view/projection and once with last frame's, so the pixel
// shader can derive a per-pixel screen-space displacement.
struct MotionVSOutput
{
    float4 Position : SV_Position;
    float4 CurrentClipPosition : TEXCOORD0;
    float4 PreviousClipPosition : TEXCOORD1;
};
