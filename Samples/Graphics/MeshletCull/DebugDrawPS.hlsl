//--------------------------------------------------------------------------------------
// DebugDrawPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

[RootSignature("CBV(b0), CBV(b1), SRV(t0)")]
float4 main(DebugVertex vin) : SV_TARGET
{
    return vin.Color;
}
