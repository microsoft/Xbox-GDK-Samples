//------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Simple shader to render a textured quad
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

[RootSignature(MainRS)]
Pixel main(Interpolants In)
{
    Pixel Out;
    MeshInfo currentMeshInfo = g_meshInfo[g_meshCB.meshInfoIndex];
    Texture2D materialTex = ResourceDescriptorHeap[currentMeshInfo.texIndex];
    SamplerState texSamp = SamplerDescriptorHeap[currentMeshInfo.samplerIndex];
    float4 albedoColor = materialTex.Sample(texSamp, In.texcoord);
    
    float3 lightDir = normalize(g_sceneCB.lightPosition - In.worldPos);
    float nDotL = max(0.0f, dot(lightDir, normalize(In.normal)));
    float4 diffuseColor = g_sceneCB.lightDiffuseColor * nDotL;
    
    Out.color = (g_sceneCB.lightAmbientColor + diffuseColor) * albedoColor;
    return Out;
}
