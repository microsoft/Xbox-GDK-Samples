#include "UpscaleBlitCommon.hlsli"

[RootSignature(UpscaleBlitRootSignature)]
FullScreenOutput main(uint vertexId : SV_VertexID)
{
    static const float4 positions[3] =
    {
        float4(-1.0f, 1.0f, 0.0f, 1.0f),
        float4(3.0f, 1.0f, 0.0f, 1.0f),
        float4(-1.0f, -3.0f, 0.0f, 1.0f)
    };
    static const float2 texCoords[3] =
    {
        float2(0.0f, 0.0f),
        float2(2.0f, 0.0f),
        float2(0.0f, 2.0f)
    };

    FullScreenOutput output;
    output.Position = positions[vertexId];
    output.TexCoord = texCoords[vertexId];
    return output;
}
