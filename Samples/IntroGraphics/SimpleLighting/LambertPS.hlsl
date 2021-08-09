#include "SimpleLighting.hlsli"

//--------------------------------------------------------------------------------------
// Desc: Pixel shader applying Lambertian lighting from two lights
//--------------------------------------------------------------------------------------
[RootSignature(rootSig)]
float4 main(PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;

    //do NdotL lighting for 2 lights
    for (int i = 0; i < 2; i++)
    {
        finalColor += saturate(dot((float3) lightDir[i], input.Normal) * lightColor[i]);
    }
    finalColor.a = 1;
    return finalColor;
}
