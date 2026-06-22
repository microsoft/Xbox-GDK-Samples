//--------------------------------------------------------------------------------------
// CSResolve.hlsli
//
// Common code for resolve using a compute shader
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"
#include "FMask.hlsli"

Texture2DMS<float4> texColorMS : register(t0);
Texture2D<uint2> texFMask : register(t1);
RWTexture2D<float4> texColor : register(u0);

//-------------------------------------------------------------------------------------------------------------
// Name: CSManualResolve()
// Desc: Manually resolve the MS texture by reading FMask and Color and writing to output texture.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_COMPUTE
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float4 resolvedColor = ManualResolve(texColorMS, texFMask, id.xy);
    texColor[id.xy] = resolvedColor;
}
