#include "Shared.hlsli"

[RootSignature(RootSig)]
Interpolants main(uint vertexIndex : SV_VertexID)
{
    Interpolants rv;
    rv.uv = float2(((vertexIndex + 1) >> 1) & 1, (vertexIndex >> 1) & 1);
    rv.pos.xy = (rv.uv * float2(2.0f, -2.0f) + float2(-1.0f,1.0f)) * g_quadScale;
    rv.pos.zw = float2(0, 1);
    return rv;
}
