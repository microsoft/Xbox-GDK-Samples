//--------------------------------------------------------------------------------------
// Common.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Constants for the sample - used by cpp and hlsl files
//--------------------------------------------------------------------------------------

#define NUM_MODELS 3
#define MAX_LIGHTS 3 // Change the packoffset values in Common.hlsli in case you change this
#define NUM_LIGHTS 2
#define NUM_PSO 3
#ifdef _DEBUG
#define NUM_INSTANCES 64 * 64
#else
#define NUM_INSTANCES 64 * 256
#endif

// The discrepancy in size is due to PSO's being larger on Scarlett
#if defined(__XBOX_SCARLETT) || defined(_GAMING_XBOX_SCARLETT)
#define SIZEOF_GPU_APPEND_BUFFER_STRUCT 108
#else
#define SIZEOF_GPU_APPEND_BUFFER_STRUCT 104
#endif
