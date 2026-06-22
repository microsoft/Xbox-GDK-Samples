//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifndef STORE_PATTERN
#define STORE_PATTERN STORE_PATTERN_COHERENT
#endif

// Keep these in sync with TcCompatDccUavTest::m_threadGroup<X|Y|Z> in TcCompatBenchmark.cpp
#if STORE_PATTERN == STORE_PATTERN_COHERENT
#define THREADGROUP_X 8
#define THREADGROUP_Y 8
#define THREADGROUP_Z 1
#elif STORE_PATTERN == STORE_PATTERN_SCATTERED
#define THREADGROUP_X 1
#define THREADGROUP_Y 64
#define THREADGROUP_Z 1
#endif

RWTexture2D<float> tex                     : register(u0);

// Create a draw which is bandwidth bound, so that TC compat has a chance of improving performance
[numthreads(THREADGROUP_X, THREADGROUP_Y, THREADGROUP_Z)]
[ROOT_SIGNATURE]
void main(uint3 idDispatchThread : SV_DispatchThreadID)
{
    uint2 texcoord = idDispatchThread.xy;

    tex[texcoord.xy] = 0;
}
