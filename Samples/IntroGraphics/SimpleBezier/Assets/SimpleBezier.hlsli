//--------------------------------------------------------------------------------------
// SimpleBezier.hlsli
//
// Shader demonstrating DirectX tessellation of a bezier surface
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


// The input patch size.  In this sample, it is 16 control points.
// This value should match the call to IASetPrimitiveTopology()
#define INPUT_PATCH_SIZE 16

// The output patch size.  In this sample, it is also 16 control points.
#define OUTPUT_PATCH_SIZE 16

// The root signature for the shaders.
#define rootSig "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), DescriptorTable(CBV(b0))"

#ifndef BEZIER_HS_PARTITION
#define BEZIER_HS_PARTITION "integer"
#endif

#ifdef __XBOX_SCARLETT
#define __XBOX_PRECOMPILE_VS_HS 1
#endif

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register(b0)
{
    row_major matrix g_mViewProjection;
    float3 g_cameraWorldPos;
    float  g_tessellationFactor;
};

//--------------------------------------------------------------------------------------
// Vertex shader section
//--------------------------------------------------------------------------------------
struct VS_CONTROL_POINT_OUTPUT
{
    float3 pos      : POSITION;
};

//--------------------------------------------------------------------------------------
// Constant data function for the BezierHS.  This is executed once per patch.
//--------------------------------------------------------------------------------------
struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4]      : SV_TessFactor;
    float Inside[2]     : SV_InsideTessFactor;
};

struct HS_OUTPUT
{
    float3 pos          : BEZIERPOS;
};

//--------------------------------------------------------------------------------------
// This constant hull shader is executed once per patch.  For the simple Mobius strip
// model, it will be executed 4 times.  In this sample, we set the tessellation factor
// via SV_TessFactor and SV_InsideTessFactor for each patch.  In a more complex scene,
// you might calculate a variable tessellation factor based on the camera's distance.
//--------------------------------------------------------------------------------------
HS_CONSTANT_DATA_OUTPUT BezierConstantHS(InputPatch< VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE > ip,
    uint PatchID : SV_PrimitiveID)
{
    HS_CONSTANT_DATA_OUTPUT output;

    float TessAmount = g_tessellationFactor;

    output.Edges[0] = output.Edges[1] = output.Edges[2] = output.Edges[3] = TessAmount;
    output.Inside[0] = output.Inside[1] = TessAmount;

    return output;
}

//--------------------------------------------------------------------------------------
// BezierHS
// The hull shader is called once per output control point, which is specified with
// outputcontrolpoints.  For this sample, we take the control points from the vertex
// shader and pass them directly off to the domain shader.  In a more complex scene,
// you might perform a basis conversion from the input control points into a Bezier
// patch, such as the SubD11 SimpleBezier.
//
// The input to the hull shader comes from the vertex shader.
//
// The output from the hull shader will go to the domain shader.
// The tessellation factor, topology, and partition mode will go to the fixed function
// tessellator stage to calculate the UVW barycentric coordinates and domain points.
//--------------------------------------------------------------------------------------
[RootSignature(rootSig)]
[domain("quad")]
[partitioning(BEZIER_HS_PARTITION)]
[outputtopology("triangle_cw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("BezierConstantHS")]
HS_OUTPUT BezierHS(InputPatch< VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE > p,
    uint i : SV_OutputControlPointID,
    uint PatchID : SV_PrimitiveID)
{
    HS_OUTPUT output;
    output.pos = p[i].pos;
    return output;
}

//--------------------------------------------------------------------------------------
// Bezier evaluation domain shader section
//--------------------------------------------------------------------------------------
struct DS_OUTPUT
{
    float4 pos        : SV_POSITION;
    float3 worldPos   : WORLDPOS;
    float3 normal     : NORMAL;
};

