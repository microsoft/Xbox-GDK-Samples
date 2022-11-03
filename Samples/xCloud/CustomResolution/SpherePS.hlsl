#define ROOT_SIG    " RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
                    "            | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
                    "            | DENY_HULL_SHADER_ROOT_ACCESS)," \
                    " CBV(b0, space = 0)" \

struct VSOut
{
    float4 position : SV_Position;
    float4 normal : Normals;
};

[RootSignature(ROOT_SIG)]
float4 main(VSOut In) : SV_TARGET
{
    float3 normal = normalize(In.normal.xyz);

    // Faking a light
    float3 lightColor = float3(0.6f, 0.6f, 0.0f);
    float3 lightDir = float3(0.0f, -1.0f, 0.0f);

    // diffuse
    float3 sphereDiffuse = float3(0.5f, 0.5f, 0.6f);
    float fDiffuseIntensity = max(dot(lightDir, normal), 0.0f);

    return float4(sphereDiffuse + fDiffuseIntensity * lightColor, 1.0f);
}
