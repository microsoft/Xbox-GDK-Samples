//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#if defined(__XBOX_ENABLE_WAVE32)
    #define PER_THREAD_PREFIX_WIDTH 4
    #define WAVE_SIZE 32
#else
    #define PER_THREAD_PREFIX_WIDTH 2
    #define WAVE_SIZE 64
#endif

#include "PrefixSumShared.hlsli"
