#include "SimpleInstancing.hlsli"

//--------------------------------------------------------------------------------------
// Desc: Pixel Shader main entrypoint.
//--------------------------------------------------------------------------------------
[RootSignature(rootSig)]
float4 main(Interpolants In) : SV_Target
{
    float4 colorOut = 0;

    // Directional component:
    colorOut = saturate(dot(In.Normal, Directional.xyz)) * In.Color * 0.5;

    for (uint i = 0; i < c_pointLightCount; ++i)
    {
        float3 pointDirection = PointPositions[i].xyz - In.WorldPos;
        float d = length(pointDirection);
        float attenuation = max(0, 1.0f - (dot(pointDirection, pointDirection) / 500));
        pointDirection = normalize(pointDirection);
        colorOut += saturate(dot(In.Normal, pointDirection)) * In.Color * PointColors[i] * attenuation;
    }

    return colorOut + ((sign(In.Color.a) * In.Color));
}