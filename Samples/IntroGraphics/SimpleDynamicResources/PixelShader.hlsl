//------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Simple shader to render a textured quad
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSig.hlsli"

struct Interpolants
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct Pixel
{
    float4 color    : SV_Target;
};

[RootSignature(MainRS)]
Pixel main( Interpolants In )
{
    // ResourceDescriptorHeap[] is a new keyword in HLSL SM 6.6 allowing dynamic access to resources through the descriptor heap.
    Texture2D txDiffuse = ResourceDescriptorHeap[0];

    // SamplerDescriptorHeap[] is a new keyword in HLSL SM 6.6 allowing dynamic access to samplers through the descriptor heap.
    SamplerState samLinear = SamplerDescriptorHeap[0];

    Pixel Out;
    Out.color = txDiffuse.Sample(samLinear, In.texcoord);
    return Out;
}
