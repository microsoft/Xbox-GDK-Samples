#include "SimpleLighting.hlsli"

//--------------------------------------------------------------------------------------
// Desc: Pixel shader applying solid color
//--------------------------------------------------------------------------------------
[RootSignature(rootSig)]
float4 main(PS_INPUT input) : SV_Target
{
    return outputColor;
}