//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define SAMPLE_TYPE Sample
#define SAMPLE_ARGS , int3(0, 0, 0), 0.5f
#define SAMPLER_STATE_TYPE SamplerState

#define TEXTURE_TYPE Texture3D
#define TEXCOORD_CHANNELS xyz

#include "SamplePs.hlsli"