//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Simple vertex shader for rendering a  moving screen space triangle
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE "RootConstants(b0, num32bitconstants=1)"

static const float4 Positions[3] =
{
    float4(0.0, 0.5, 0.5, 1.0),
    float4(0.5, -0.5, 0.5, 1.0),
    float4(-0.5, -0.5, 0.5, 1.0),
};

static const float4 Colors[3] =
{
    float4(1.0, 0.0, 0.0, 1.0),
    float4(0.0, 1.0, 0.0, 1.0),
    float4(0.0, 0.0, 1.0, 1.0),
};

struct Interpolants
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
};

cbuffer Constants : register(b0)
{
    float Time;
};

[RootSignature(ROOT_SIGNATURE)]
Interpolants main(uint VertexId : SV_VertexId)
{
    // Rotate & translate screen-space triangle using simple sinusoidal functions over time
    const float freq = 0.125; // Slow down the period

    float t = (sin(Time) + 1) * 0.5f + Time; // Reparameterize time to make movement more interesting

    float v = 2 * 3.1415926 * freq * t;
    float c = cos(v);
    float s = sin(v);

    // Rotation around Z-axis + circular translation
    float4x4 xfm = float4x4(
        c, -s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        s * 0.5, c * 0.5, 0, 1);

    Interpolants Out;
    Out.Position = mul(Positions[VertexId], xfm);
    Out.Color    = Colors[VertexId];
    return Out;
}
