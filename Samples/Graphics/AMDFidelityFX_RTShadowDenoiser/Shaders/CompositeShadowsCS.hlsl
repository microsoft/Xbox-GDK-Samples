
#define ComputeRS \
        "DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL), "\
        "DescriptorTable(SRV(t1, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL), "\
        "DescriptorTable(UAV(u0, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)"

Texture2D InputScene : register(t0);
Texture2D InputShadows : register(t1);
RWTexture2D<float4> Output : register(u0);


[RootSignature(ComputeRS)]
[numthreads(8, 8, 1)]
void main(uint2 did : SV_DispatchThreadID)
{
    float3 scene = InputScene.Load(uint3(did, 0)).rgb;
    float shadow = InputShadows.Load(uint3(did, 0)).r;
    if ( shadow < 1.0f )
    {
        scene = scene * shadow;
    }
    Output[did.xy] = float4(scene, 1);
}

