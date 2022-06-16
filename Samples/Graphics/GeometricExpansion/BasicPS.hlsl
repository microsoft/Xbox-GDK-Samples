//--------------------------------------------------------------------------------------
// BasicPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

[RootSignature(ROOT_SIG)]
float4 main(VertexOut input) : SV_TARGET
{
    return float4(input.Color.xyz * input.Color.a, input.Color.a);
}
