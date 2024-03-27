//--------------------------------------------------------------------------------------
// DebugDrawPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

[RootSignature(ROOT_SIG_DEBUG)]
float4 main(DebugVertex vin) : SV_TARGET
{
    return vin.Color;
}
