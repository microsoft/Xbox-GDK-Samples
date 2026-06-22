//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "OfflineRT/ShaderShared.h"

GlobalRootSignature OfflineRT_GlobalRootSignature =
{
    "RootFlags(XBOX_RAYTRACING),"
    "DescriptorTable( UAV( u0 ), SRV( t0 ), CBV( b0 ) )"   // g_rtOutput, g_tlas, g_sceneCB
};

LocalRootSignature OfflineRT_LocalRootSignature =
{
    "DescriptorTable( SRV( t1, numDescriptors = 2 ) ),"    // g_indices, g_vertices
    "RootConstants( num32BitConstants = 4, b1 )"           // g_materialCB (inlined)
};

// Global root signature resources:
RWTexture2D<float4> g_rtOutput : register(u0);
RaytracingAccelerationStructure g_tlas : register(t0);
ConstantBuffer<SceneConstants> g_sceneCB : register(b0);

// Local root signature resources:
ByteAddressBuffer g_indices : register(t1);
StructuredBuffer<Vertex> g_vertices : register(t2);
ConstantBuffer<MaterialConstants> g_materialCB : register(b1);
