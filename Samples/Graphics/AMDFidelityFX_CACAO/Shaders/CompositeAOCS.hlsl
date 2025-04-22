
#define ComputeRS \
        "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
        "DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL), "\
        "DescriptorTable(SRV(t1, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL), "\
        "DescriptorTable(UAV(u0, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)"

Texture2D InputScene : register(t0);
Texture2D InputAO : register(t1);
RWTexture2D<float4> Output : register(u0);

cbuffer cb : register(b0)
{
    float AOFactor;
    int bufferView;
};


[RootSignature(ComputeRS)]
[numthreads(8, 8, 1)]
void main(uint2 did : SV_DispatchThreadID)
{
    float ao = InputAO.Load(uint3(did, 0)).r;
    if (bufferView != 0)
    {
        Output[did.xy] = float4(ao, ao, ao, 1);
    }
    else
    {
        float3 scene = InputScene.Load(uint3(did, 0)).rgb;
        scene = scene * clamp(sqrt(ao)*AOFactor, 0.0, 1.0);

        Output[did.xy] = float4(scene, 1);
    }
}

