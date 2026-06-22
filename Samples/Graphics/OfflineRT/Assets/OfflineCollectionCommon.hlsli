//--------------------------------------------------------------------------------------
// OfflineCollectionCommon.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "OfflineRT/Common.hlsli"

RaytracingShaderConfig  OfflineCollection_ShaderConfig =
{
    16,                             // Max payload size (sizeof(RayPayload))
    8                               // Max attribute size (sizeof(BuiltInTriangleIntersectionAttributes))
};

RaytracingPipelineConfig OfflineCollection_PipelineConfig =
{
    1                               // Max trace recursion depth
};

struct RayPayload
{
    float4 color;
};
