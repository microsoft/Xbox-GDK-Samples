#include "SimpleBezier.hlsli"

//--------------------------------------------------------------------------------------
float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4(invT * invT * invT,
        3.0f * t * invT * invT,
        3.0f * t * t * invT,
        t * t * t);
}

//--------------------------------------------------------------------------------------
float4 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4(-3 * invT * invT,
        3 * invT * invT - 6 * t * invT,
        6 * t * invT - 3 * t * t,
        3 * t * t);
}

//--------------------------------------------------------------------------------------
float3 EvaluateBezier(const OutputPatch< HS_OUTPUT, OUTPUT_PATCH_SIZE > BezPatch,
    float4 BasisU,
    float4 BasisV)
{
    float3 value = float3(0, 0, 0);
    value = BasisV.x * (BezPatch[0].pos * BasisU.x + BezPatch[1].pos * BasisU.y + BezPatch[2].pos * BasisU.z + BezPatch[3].pos * BasisU.w);
    value += BasisV.y * (BezPatch[4].pos * BasisU.x + BezPatch[5].pos * BasisU.y + BezPatch[6].pos * BasisU.z + BezPatch[7].pos * BasisU.w);
    value += BasisV.z * (BezPatch[8].pos * BasisU.x + BezPatch[9].pos * BasisU.y + BezPatch[10].pos * BasisU.z + BezPatch[11].pos * BasisU.w);
    value += BasisV.w * (BezPatch[12].pos * BasisU.x + BezPatch[13].pos * BasisU.y + BezPatch[14].pos * BasisU.z + BezPatch[15].pos * BasisU.w);

    return value;
}

//--------------------------------------------------------------------------------------
// The domain shader is run once per vertex and calculates the final vertex's position
// and attributes.  It receives the UVW from the fixed function tessellator and the
// control point outputs from the hull shader.  Since we are using the DirectX 11
// Tessellation pipeline, it is the domain shader's responsibility to calculate the
// final SV_POSITION for each vertex.  In this sample, we evaluate the vertex's
// position using a Bernstein polynomial and the normal is calculated as the cross
// product of the U and V derivatives.
//
// The input SV_DomainLocation to the domain shader comes from fixed function
// tessellator.  And the OutputPatch comes from the hull shader.  From these, you
// must calculate the final vertex position, color, texcoords, and other attributes.
//
// The output from the domain shader will be a vertex that will go to the video card's
// rasterization pipeline and get drawn to the screen.
//--------------------------------------------------------------------------------------
[RootSignature(rootSig)]
[domain("quad")]
DS_OUTPUT main(HS_CONSTANT_DATA_OUTPUT input,
    float2 UV : SV_DomainLocation,
    const OutputPatch< HS_OUTPUT, OUTPUT_PATCH_SIZE > BezPatch)
{
    float4 BasisU = BernsteinBasis(UV.x);
    float4 BasisV = BernsteinBasis(UV.y);
    float4 dBasisU = dBernsteinBasis(UV.x);
    float4 dBasisV = dBernsteinBasis(UV.y);

    float3 worldPos = EvaluateBezier(BezPatch, BasisU, BasisV);
    float3 tangent = EvaluateBezier(BezPatch, dBasisU, BasisV);
    float3 biTangent = EvaluateBezier(BezPatch, BasisU, dBasisV);
    float3 normal = normalize(cross(tangent, biTangent));

    DS_OUTPUT output;
    output.pos = mul(float4(worldPos, 1), g_mViewProjection);
    output.worldPos = worldPos;
    output.normal = normal;

    return output;
}
