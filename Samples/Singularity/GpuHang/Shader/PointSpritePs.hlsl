//------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE\
    RootSignature\
    (\
        "\
            SRV(t0, visibility=SHADER_VISIBILITY_GEOMETRY),\
            DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),\
            StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL),\
        "\
    )\

struct Interpolants
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct Pixel
{
    float4 color    : SV_Target;
};

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

[ROOT_SIGNATURE]
Pixel main( Interpolants In )
{
    Pixel Out;
    Out.color = txDiffuse.Sample(samLinear, In.texcoord);
    return Out;
}