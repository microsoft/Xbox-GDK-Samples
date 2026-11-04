#include "PBRCommon.hlsli"

[RootSignature(PBRRootSignature)]
float4 main(VSInput input) : SV_Position
{
    const float4 positionWS = mul(float4(input.Position, 1.0f), World);
    return mul(positionWS, LightViewProjection);
}
