#include "SimpleInstancing.hlsli"

//--------------------------------------------------------------------------------------
// Desc: Vertex Shader main entrypoint.
//--------------------------------------------------------------------------------------
[RootSignature(rootSig)]
Interpolants main(InstancedVertex In)
{
    Interpolants Out = (Interpolants)0;
    // Scale.
    float3 position = In.Position * In.InstPosScale.w;

    // Rotate vertex position and normal based on instance quaternion...
    position = RotateVectorByQuaternion(In.InstRotation, position);
    float3 normal = RotateVectorByQuaternion(In.InstRotation, In.Normal);

    // Move to world space.
    position += In.InstPosScale.xyz;

    // ...and clip.
    Out.Position = mul(float4(position, 1), Clip);

    // World space transform
    Out.WorldPos = position;

    // Finally, output  normal and color
    Out.Normal = normal;
    Out.Color = In.InstColor;

    return Out;
}