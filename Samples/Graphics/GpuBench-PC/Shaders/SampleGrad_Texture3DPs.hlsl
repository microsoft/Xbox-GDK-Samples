//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define SAMPLE_TYPE SampleGrad
#define SAMPLE_ARGS , float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f)

#define TEXTURE_TYPE Texture3D
#define TEXCOORD_CHANNELS xyz

#include "SamplePs.hlsli"
