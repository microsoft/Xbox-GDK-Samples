//--------------------------------------------------------------------------------------
// SimpleInstancing.hlsli
//
// Simple shaders demonstrating how to perform instanced drawing.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.h"

#define rootSig "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
	"| DENY_DOMAIN_SHADER_ROOT_ACCESS " \
	"| DENY_GEOMETRY_SHADER_ROOT_ACCESS " \
	"| DENY_HULL_SHADER_ROOT_ACCESS), " \
    "CBV(b0, space = 0, visibility=SHADER_VISIBILITY_VERTEX), " \
    "CBV(b0, space = 0, visibility=SHADER_VISIBILITY_PIXEL)"

//--------------------------------------------------------------------------------------
// Name: InstancingConstants
// Desc: Constant buffer containing clip-space transform.
//--------------------------------------------------------------------------------------
cbuffer InstancingConstants
{
    float4x4 Clip;      // Clip transform
};

//--------------------------------------------------------------------------------------
// Name: Lights
// Desc: Constant buffer containing lighting information.
//--------------------------------------------------------------------------------------
cbuffer Lights
{
    float4 Directional;
    float4 PointPositions[ c_pointLightCount ];
    float4 PointColors[ c_pointLightCount ];
};

//--------------------------------------------------------------------------------------
// Name: InstancedVertex
// Desc: Structure containing vertex definition for instanced drawing.
//       The incoming vertices arrive from 3 streams - one of which contains 
//       per-primitive information, the other two per-instance.
//--------------------------------------------------------------------------------------
struct InstancedVertex
{
    float3 Position     : POSITION;     // Vertex position (per primitive)
    float3 Normal       : NORMAL;       // Vertex normal (per primitive)
    float4 InstRotation : I_ROTATION;   // Orientation quaternion (per instance)
    float4 InstPosScale : I_POSSCALE;   // Position and scale (per instance)
    float4 InstColor    : I_COLOR;      // Color (per instance)
};

//--------------------------------------------------------------------------------------
// Name: Interpolants
// Desc: Interpolated values passed to the pixel shader.
//--------------------------------------------------------------------------------------
struct Interpolants
{
    float4 Position     : SV_POSITION;
    float3 Normal       : TEXCOORD0;
    float4 Color        : TEXCOORD1;
    float3 WorldPos     : TEXCOORD2;
};

//--------------------------------------------------------------------------------------
// Name: RotateVectorByQuaternion
// Desc: Rotate a vector using a quaternion.
//--------------------------------------------------------------------------------------
float3 RotateVectorByQuaternion( float4 Q, float3 V )
{
    return V + 2.0f * cross( Q.xyz, cross( Q.xyz, V ) + Q.w * V );
}


