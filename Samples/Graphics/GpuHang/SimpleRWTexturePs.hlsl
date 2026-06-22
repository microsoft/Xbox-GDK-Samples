//------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct Interpolants
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct Pixel
{
    float4 color    : SV_Target;
};

RWTexture2D<float4> txDiffuse : register(u0);

#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "DescriptorTable (UAV(u0), visibility=SHADER_VISIBILITY_PIXEL),"

[RootSignature(MainRS)]
Pixel main(Interpolants In)
{
    Pixel Out;
    uint2 uv = uint2(1920, 1080) * In.texcoord;
    Out.color = txDiffuse.Load(uv);
    txDiffuse[uv] = Out.color;
    return Out;
}
