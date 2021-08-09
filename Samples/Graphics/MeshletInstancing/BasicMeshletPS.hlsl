//--------------------------------------------------------------------------------------
// BasicMeshletPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"


//--------------------------------------------------------------------
// Main

[RootSignature(ROOT_SIG)]
float4 main(VertexOut input) : SV_TARGET
{
    float ambientIntensity = 0.1;
    float3 lightColor = float3(1, 1, 1);
    float3 lightDir = -normalize(float3(1, -1, 1));

    float3 color;
    float shininess;

    if (Globals.RenderMode == 0)
    {
        color = 0.8;
        shininess = 64.0;
    }
    else // Globals.RenderMode == 1)
    {
        uint meshletIndex = input.MeshletIndex;

        color = float3(
            float(meshletIndex & 1),
            float(meshletIndex & 3) / 4,
            float(meshletIndex & 7) / 8);
        shininess = 16.0;
    }

    float3 normal = normalize(input.Normal);

    // Do some fancy Blinn-Phong shading!
    float cosAngle = saturate(dot(normal, lightDir));
    float3 viewDir = -normalize(input.PositionVS);
    float3 halfAngle = normalize(lightDir + viewDir);

    float blinnTerm = saturate(dot(normal, halfAngle));
    blinnTerm = cosAngle != 0.0 ? blinnTerm : 0.0;
    blinnTerm = pow(blinnTerm, shininess);

    float3 finalColor = (cosAngle + blinnTerm + ambientIntensity) * color;

    return float4(finalColor, 1);
}
