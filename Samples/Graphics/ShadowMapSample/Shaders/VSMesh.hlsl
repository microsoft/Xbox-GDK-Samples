//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"


ConstantBuffer<CBTransformStruct> cbTransform  : register(b0);
ConstantBuffer<CBSceneConstStruct> cbConstants : register(b3);


[RootSignature(ROOT_SIG)]
InterpolantsMesh main(VertexMesh In)
{
    InterpolantsMesh Out;
    
    Out.position = mul(In.position, cbTransform.g_mWorld);
    Out.position = mul(Out.position, cbConstants.g_mView);
    Out.position = mul(Out.position, cbConstants.g_mProj);

    Out.texcoord = In.texcoord;

    Out.normal = mul(In.normal, (float3x3) cbTransform.g_mWorld);
#ifdef SUPPORT_NORMAL_MAP
    Out.tangent = mul( In.tangent, (float3x3) cbTransform.g_mWorld );
    Out.binormal = mul( In.binormal, (float3x3) cbTransform.g_mWorld );
#endif
    
    Out.color = In.color;

    Out.posworld = mul(In.position.xyz, (float3x3) cbTransform.g_mWorld);

    return Out;
}
