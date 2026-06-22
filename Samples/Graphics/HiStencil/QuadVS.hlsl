//--------------------------------------------------------------------------------------
// QuadVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Simple shader to render a quad
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct VSInput
{
    float4 position     : SV_Position;
    float2 texCoord     : TEXCOORD0;
};

struct VSOutput
{
    float4 position     : SV_Position;
    float2 texCoord     : TEXCOORD0;
};

[ROOT_SIGNATURE_MAIN]
VSOutput main(VSInput vsIn)
{
    return (VSOutput) vsIn;
}
