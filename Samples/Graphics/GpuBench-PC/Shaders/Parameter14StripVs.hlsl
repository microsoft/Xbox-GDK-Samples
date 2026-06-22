//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define NUM_EXPORTS 14

#if defined(__XBOX_SCARLETT) && (__DXC_VERSION_RELEASE >= 2404)
#define __XBOX_ENABLE_UNUSED_PARAMETER_REMOVAL 1
#endif

#ifndef __XBOX_SCARLETT
#undef DEST_16_BIT
#endif

#include "ParameterVs.hlsli"
