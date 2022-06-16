//--------------------------------------------------------------------------------------
// BasicMeshletPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "CommonMS.hlsli"

float3 Luminosity(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722)).xxx;
}

[RootSignature(ROOT_SIG)]
float4 main(VertexOut input) : SV_TARGET
{
    float ambientIntensity = 0.1;
    float3 lightColor = float3(1, 1, 1);
    float3 lightDir   = -normalize(float3(1, -1, 1));

    float3 color;
    float shininess = 16;

    // Choose diffuse color/shininess by render mode
    if (Constants.RenderMode == 0) // Flat-white shading
    {
        color = 0.8;
    }
    else  // Per-meshlet color shading
    {
        color = float3(
            float(input.MeshletIndex & 1),
            float(input.MeshletIndex & 3) / 4,
            float(input.MeshletIndex & 7) / 8
        );

        // When the user has cursored over a meshlet or has selected one, desaturate every other meshlet.
        if (Constants.HighlightedIndex != -1 ||
            Constants.SelectedIndex != -1)
        {
            // A meshlet is highlighted or selected

            if (input.MeshletIndex != Constants.SelectedIndex &&
                input.MeshletIndex != Constants.HighlightedIndex)
            {
                // This meshlet is not highlighted - desaturate its color.
                color = lerp(color, Luminosity(color) + 0.2, 0.8);
            }
        }
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
