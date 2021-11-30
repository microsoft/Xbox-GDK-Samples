//--------------------------------------------------------------------------------------
// SkyboxEffect_PS.hlsl
//
// A sky box effect for DirectX 12.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "SkyboxEffect_Common.hlsli"

TextureCube<float4> CubeMap : register(t0);
SamplerState Sampler        : register(s0);

[RootSignature(SkyboxRS)]
float4 main(float3 texCoord : TEXCOORD0) : SV_TARGET0
{
    return CubeMap.Sample(Sampler, normalize(texCoord));
}
