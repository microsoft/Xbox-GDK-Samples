//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Simple shader to render a triangle
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct Interpolants
{
    float4 position : SV_Position;
    float4 color    : COLOR0;
};

struct Pixel
{
    float4 color    : SV_TARGET0;
};

uint GetTimeStamp()
{
    uint2 memtime = __XB_MemTime();
    return ((uint) memtime.y << 32) | memtime.x;
}

// This explicitly makes the triangle's pixel shader expensive enough
// to benefit from using coarser shading rates through VRS.
float ExpensiveShaderFunction()
{
    static const uint clockRate = 1825;
    static const uint g_desiredFrameTime = 500 * clockRate;
    uint startTime = GetTimeStamp();
    uint currentTime = GetTimeStamp();
    uint duration = 0;

    do
    {
        currentTime = GetTimeStamp();
        duration = currentTime - startTime;
    } while (duration < g_desiredFrameTime);

    float dummyDontOptimizeMeOut = float(duration);
    return dummyDontOptimizeMeOut;
}

[RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)")]
Pixel main(Interpolants In)
{
    Pixel Out;
    Out.color = In.color;
    Out.color.a = ExpensiveShaderFunction();
    return Out;
}
