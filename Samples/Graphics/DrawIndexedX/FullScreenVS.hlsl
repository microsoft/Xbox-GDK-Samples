
#include "Common.hlsli"

struct VSInput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

[ROOT_SIGNATURE_MESH]
VSOutput main( VSInput vsIn )
{
    return (VSOutput) vsIn;
}
