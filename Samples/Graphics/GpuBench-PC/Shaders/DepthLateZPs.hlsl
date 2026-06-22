//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#if defined(__XBOX_SCARLETT) || defined(__XBOX_ONE)
// This only takes effect if we also call SetPixelShaderDepthForceZOrder(TRUE)
#define __XBOX_FORCE_PS_ZORDER_LATE_Z 1
#else
#define PREFER_LATE_Z 1
#endif

#include "DepthPs.hlsli"
