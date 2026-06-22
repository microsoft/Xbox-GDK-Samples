//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define SAMPLE_TYPE Sample
#define SAMPLE_ARGS , 0.0f, status
#define SAMPLER_STATE_TYPE SamplerState

#if defined(__XBOX_SCARLETT) || defined(__XBOX_ONE)
// Use Texture3D here to save VALU, otherwise we will be at least partly VALU bound,
// due to the calculation of the cubemap texcoords. We are not interested in VALU cost here.
//
// The same image_ op will be generated for Texture3D as for TextureCube.
// Because the descriptor is actually a TextureCube, the hardware will perform a 
// cubemap fetch
#define TEXTURE_TYPE Texture3D
#else
#define TEXTURE_TYPE TextureCube
#endif
#define TEXCOORD_CHANNELS xyz

#include "SamplePs.hlsli"
