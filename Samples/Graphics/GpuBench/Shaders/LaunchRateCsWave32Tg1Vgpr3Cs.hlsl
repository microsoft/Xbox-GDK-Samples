//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef __XBOX_SCARLETT
#define __XBOX_ENABLE_WAVE32 1
#endif

#define THREADGROUP_X 8
#define THREADGROUP_Y 4
#define THREADGROUP_Z 1

#define VGPR_LOAD 3

#include "LaunchRateCsCs.hlsli"
