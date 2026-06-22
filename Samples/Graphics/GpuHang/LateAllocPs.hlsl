//------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Force low occupancy, so that room for more waves is hard to come by
#ifndef __XBOX_SCARLETT
#define __XBOX_LIMIT_OCCUPANCY_WITH_HARD_LIMIT
#define __XBOX_REGALLOC_FORCE_VGPR_LIMIT 128
#endif

// Force high parameter cache footprint by adding lots of interpolants (don't need to be used in current D3D12.X)
struct Interpolants
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 extra[30] : extra;
};

struct Pixel
{
    float4 color    : SV_Target;
};

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
    "StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL)"

[RootSignature(MainRS)]
Pixel main( Interpolants In )
{
    Pixel Out;
    Out.color = txDiffuse.Sample(samLinear, In.texcoord);
    return Out;
}
