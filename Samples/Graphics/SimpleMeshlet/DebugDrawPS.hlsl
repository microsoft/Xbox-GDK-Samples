//--------------------------------------------------------------------------------------
// DebugDrawPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


struct DebugVertex
{
    float4 Color : COLOR0;
    float4 Position : SV_POSITION;
};

[RootSignature("CBV(b0)")]
float4 main(DebugVertex vin) : SV_TARGET
{
    return vin.Color;
}
