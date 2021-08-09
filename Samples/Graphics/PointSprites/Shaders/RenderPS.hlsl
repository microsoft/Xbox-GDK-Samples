//--------------------------------------------------------------------------------------
// RenderPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

Texture2D    t1 : register(t1);
SamplerState s0 : register(s0);

[RootSignature(CommonRS)]
float4 main(VSOut vin) : SV_Target
{
    float4 c = vin.clr * t1.Sample(s0, vin.uv);

    clip(dot(c.xyz, 1) - 16.f / 255.f);

    return c;
}
