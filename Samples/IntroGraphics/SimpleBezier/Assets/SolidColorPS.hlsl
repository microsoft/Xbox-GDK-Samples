#include "SimpleBezier.hlsli"

//--------------------------------------------------------------------------------------
// Solid color shading pixel shader (used for wireframe overlay)
//--------------------------------------------------------------------------------------
[RootSignature(rootSig)]
float4 main(DS_OUTPUT Input) : SV_TARGET
{
    // Return a dark green color
    return float4(0, 0.25, 0, 1);
}
