#define ROOT_SIG    " RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
                    "            | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
                    "            | DENY_HULL_SHADER_ROOT_ACCESS)," \
                    " CBV(b0, space = 0)" \

struct SphereCB
{
    float4x4 mvp;
};
ConstantBuffer<SphereCB> CB : register(b0);

struct VSIn
{
    float3 position : SV_Position;
    float3 normals : NORMAL;
    float2 texcoords : TEXCOORD;
};

struct VSOut
{
    float4 position : SV_Position;
    float4 normal : Normals;
};

[RootSignature(ROOT_SIG)]
VSOut main(VSIn In)
{
    VSOut Out = (VSOut) 0;
    Out.position = mul(float4(In.position, 1), CB.mvp);
    Out.normal = float4(In.normals, 0);
    return Out;
}
