//--------------------------------------------------------------------------------------
// OpaqueVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define USE_HLSL
#include "SharedDefinitions.h"
#include "HLSLCommon.hlsli"

ConstantBuffer<SceneConstants> sceneCts     : register(b0);
ConstantBuffer<ObjectConstants> objCts      : register(b1);

[RootSignature(MAIN_RS)]
Interpolators main( VertexInputs IN )
{
    Interpolators OUT = (Interpolators)0;

    OUT.position = mul(float4(IN.position, 1.0f), objCts.worldMatrix);
    float4 viewSpace = mul(OUT.position, sceneCts.viewMatrix);

    // Pass view space depth to pixel shader.
    OUT.viewDepth = viewSpace.z;

    OUT.position = mul(viewSpace, sceneCts.projMatrix);

    OUT.texcoords = IN.texcoords;

    return OUT;
}
