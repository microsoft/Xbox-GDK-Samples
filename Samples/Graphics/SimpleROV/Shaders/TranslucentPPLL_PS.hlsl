//--------------------------------------------------------------------------------------
// TranslucentPPLL_PS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define USE_HLSL
#include "SharedDefinitions.h"
#include "HLSLCommon.hlsli"

ConstantBuffer<ObjectConstants> objCts          : register(b1);
ConstantBuffer<BlendPassConstants> cts          : register(b2);

Texture2D<float4> diffuse                       : register(t0);

RWByteAddressBuffer perPixelLinkedListHead      : register(u0);
RWStructuredBuffer<Fragment> linkedListBuffer   : register(u1);
RWByteAddressBuffer linkedListCounter           : register(u2);

sampler texSampler                              : register(s0);

[earlydepthstencil]
[RootSignature(MAIN_RS)]
void main(Interpolators IN)
{
    float4 color = float4(objCts.diffuseColor, TRANSLUCENT_ALPHA);

    // New entry for this pixel's linked list.
    uint2 pixelCoords = uint2(IN.position.x, IN.position.y);

    uint previousCountValue;
    linkedListCounter.InterlockedAdd(0, 1, previousCountValue);

    // Assume that the UAV starts with the value -1 if no element has been added to that linked list.
    uint oldHeadIndex;

    // ByteAddressBuffers (or raw buffers) use a byte value offset from the beginning of the buffer to
    // access data. The byte value must be a multiple of four so that it is DWORD aligned. If any other
    // value is provided, behavior is undefined.
    uint address = 4 * ((uint)IN.position.x  + (uint)IN.position.y * cts.resolution.x);
    perPixelLinkedListHead.InterlockedExchange(address, previousCountValue, oldHeadIndex);

    // Create the entry for the current pixel.
    Fragment entry = (Fragment)0;
    entry.color = PackRGBA(ToRGBE(float4(color.rgb * color.w, 1.0f)));

    entry.transmission = (uint)((1.0f - color.w) * TRANSMISSION_MAX);

    // With this operation we make sure depth is between 0u and 0xffffff (24 bits).
    float normalizedDepth = IN.viewDepth / cts.cameraFar;
    entry.depth = uint(normalizedDepth * DEPTH_MAX);

    entry.next = oldHeadIndex; // Tail of list will point to (uint)-1.

    linkedListBuffer[previousCountValue] = entry;
}
