#include "PBRCommon.hlsli"

Texture2D<float4> AlbedoTexture : register(t0);
Texture2D<float2> NormalTexture : register(t1);
Texture2D<float4> MetallicRoughnessTexture : register(t2);
Texture2D<float> ShadowMap : register(t3);
TextureCube<float4> IrradianceTexture : register(t4);
TextureCube<float4> RadianceTexture : register(t5);
Texture2D<float2> BrdfLutTexture : register(t6);

SamplerState MaterialSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);
SamplerState EnvironmentSampler : register(s2);

static const float Pi = 3.14159265f;

float3 FresnelSchlick(float cosTheta, float3 f0)
{
    return f0 + (1.0f - f0) * pow(1.0f - cosTheta, 5.0f);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 f0, float roughness)
{
    return f0 + (max((1.0f - roughness).xxx, f0) - f0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float3 normal, float3 halfway, float roughness)
{
    const float alpha = roughness * roughness;
    const float alphaSquared = alpha * alpha;
    const float normalDotHalfway = saturate(dot(normal, halfway));
    const float denominator = normalDotHalfway * normalDotHalfway * (alphaSquared - 1.0f) + 1.0f;
    return alphaSquared / max(Pi * denominator * denominator, 0.000001f);
}

float GeometrySchlickGGX(float normalDotDirection, float roughness)
{
    const float k = ((roughness + 1.0f) * (roughness + 1.0f)) / 8.0f;
    return normalDotDirection / max(normalDotDirection * (1.0f - k) + k, 0.000001f);
}

float GeometrySmith(float3 normal, float3 viewDirection, float3 lightDirection, float roughness)
{
    return GeometrySchlickGGX(saturate(dot(normal, viewDirection)), roughness)
        * GeometrySchlickGGX(saturate(dot(normal, lightDirection)), roughness);
}

float3x3 CalculateTangentFrame(float3 position, float3 normal, float2 texCoord)
{
    const float3 positionDx = ddx(position);
    const float3 positionDy = ddy(position);
    const float2 texCoordDx = ddx(texCoord);
    const float2 texCoordDy = ddy(texCoord);
    const float3x3 frame = float3x3(
        positionDx,
        positionDy,
        cross(positionDx, positionDy));
    const float2x3 inverseFrame = float2x3(
        cross(frame[1], frame[2]),
        cross(frame[2], frame[0]));
    const float3 tangent = normalize(mul(float2(texCoordDx.x, texCoordDy.x), inverseFrame));
    const float3 bitangent = normalize(mul(float2(texCoordDx.y, texCoordDy.y), inverseFrame));
    return float3x3(tangent, bitangent, normal);
}

float3 CalculateNormal(VSOutput input)
{
    const float2 encodedNormal = NormalTexture.Sample(MaterialSampler, input.TexCoord).xy * 2.0f - 1.0f;
    const float3 tangentNormal = float3(encodedNormal, sqrt(saturate(1.0f - dot(encodedNormal, encodedNormal))));

    const float3 geometricNormal = normalize(input.NormalWS);
    return normalize(mul(
        tangentNormal,
        CalculateTangentFrame(
            input.PositionWS,
            geometricNormal,
            input.TexCoord)));
}

float3 ToneMap(float3 color)
{
    const float3 numerator = color * (2.51f * color + 0.03f);
    const float3 denominator = color * (2.43f * color + 0.59f) + 0.14f;
    return pow(saturate(numerator / denominator), 1.0f / 2.2f);
}

float CalculateShadow(float4 shadowPosition)
{
    const float3 projected = shadowPosition.xyz / shadowPosition.w;
    const float2 uv = float2(projected.x * 0.5f + 0.5f, 0.5f - projected.y * 0.5f);

    if (projected.z <= 0.0f || projected.z >= 1.0f || any(uv < 0.0f) || any(uv > 1.0f))
    {
        return 1.0f;
    }

    uint width;
    uint height;
    ShadowMap.GetDimensions(width, height);
    
    const float2 texelSize = 1.0f / float2(width, height);
    const float comparisonDepth = projected.z - 0.0008f;
    float visibility = 0.0f;

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            visibility += ShadowMap.SampleCmpLevelZero(
                ShadowSampler,
                uv + float2(x, y) * texelSize,
                comparisonDepth);
        }
    }

    return visibility / 9.0f;
}

float3 RotateEnvironmentDirection(float3 direction)
{
    float sine;
    float cosine;
    sincos(EnvironmentParameters.z, sine, cosine);
    return float3(
        direction.x * cosine - direction.z * sine,
        direction.y,
        direction.x * sine + direction.z * cosine);
}

[RootSignature(PBRRootSignature)]
float4 main(VSOutput input) : SV_Target0
{
    const float4 sampledAlbedo = AlbedoTexture.Sample(MaterialSampler, input.TexCoord);
    const float4 baseColor = sampledAlbedo * BaseColorFactor;

    const float3 metallicRoughness = MetallicRoughnessTexture.Sample(MaterialSampler, input.TexCoord).rgb;
    const float metallic = saturate(metallicRoughness.b * MaterialFactors.x);
    const float roughness = clamp(metallicRoughness.g * MaterialFactors.y, 0.045f, 1.0f);

    const float3 normal = CalculateNormal(input);
    const float3 viewDirection = normalize(EyePosition.xyz - input.PositionWS);
    const float3 toLight = normalize(-LightDirection.xyz);
    const float3 halfway = normalize(viewDirection + toLight);
    const float normalDotLight = saturate(dot(normal, toLight));
    const float normalDotView = saturate(dot(normal, viewDirection));

    const float3 f0 = lerp(0.04f.xxx, baseColor.rgb, metallic);
    const float3 fresnel = FresnelSchlick(saturate(dot(halfway, viewDirection)), f0);
    const float distribution = DistributionGGX(normal, halfway, roughness);
    const float geometry = GeometrySmith(normal, viewDirection, toLight, roughness);
    const float3 specular = distribution * geometry * fresnel / max(4.0f * normalDotView * normalDotLight, 0.0001f);

    const float3 diffuseWeight = (1.0f - fresnel) * (1.0f - metallic);
    const float3 diffuse = diffuseWeight * baseColor.rgb / Pi;
    const float shadow = CalculateShadow(input.ShadowPosition);

    const float3 environmentFresnel = FresnelSchlickRoughness(normalDotView, f0, roughness);
    const float3 environmentDiffuseWeight = (1.0f - environmentFresnel) * (1.0f - metallic);
    const float3 irradiance = IrradianceTexture.Sample(EnvironmentSampler, RotateEnvironmentDirection(normal)).rgb;
    const float3 diffuseIbl = irradiance * baseColor.rgb;

    const float3 reflectionDirection = RotateEnvironmentDirection(reflect(-viewDirection, normal));
    const float3 radiance = RadianceTexture.SampleLevel(EnvironmentSampler, reflectionDirection, roughness * EnvironmentParameters.y).rgb;
    const float2 brdf = BrdfLutTexture.Sample(EnvironmentSampler, float2(normalDotView, roughness));
    const float3 specularIbl = radiance * (f0 * brdf.x + brdf.y);

    const float3 ibl = EnvironmentParameters.x * (environmentDiffuseWeight * diffuseIbl + specularIbl);

    const float3 color = ToneMap((diffuse + specular) * LightColor.rgb * normalDotLight * shadow + ibl);

    return float4(color, baseColor.a);
}
