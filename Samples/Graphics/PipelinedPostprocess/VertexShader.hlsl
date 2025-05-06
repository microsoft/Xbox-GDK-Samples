//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

ConstantBuffer<SceneConstantsCB> perObjCB : register(b0);

[RootSignature(ROOT_SIGNATURE)]
Interpolants main(in VSIn In)
{
    Interpolants Out;
    Out.position = mul(float4(In.position, 1.0f), perObjCB.mvp);
    return Out;
}
