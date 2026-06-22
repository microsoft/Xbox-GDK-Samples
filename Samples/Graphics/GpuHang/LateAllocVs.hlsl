//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Force low occupancy, so that room for more waves is hard to come by
#define __XBOX_MIN_VGPR_COUNT 128

struct Vertex
{
    float4 position     : SV_Position;
    float2 texcoord     : TEXCOORD0;
};

// Force high parameter cache footprint by adding lots of interpolants (don't need to be used in current D3D12.X)
struct Interpolants
{
    float4 position     : SV_Position;
    float2 texcoord     : TEXCOORD0;
    float4 extra[30] : extra;
};

#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
    "StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL)"

[RootSignature(MainRS)]
Interpolants main( Vertex In )
{
    Interpolants Out;

    Out.position = In.position;
    Out.texcoord = In.texcoord;
    [unroll]
    for (uint i = 0; i < 30; ++i)
    {
        Out.extra[i] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    return Out;
}