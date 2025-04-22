
#define ComputeRS \
        "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
        "DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL), "\
        "DescriptorTable(UAV(u0, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL), "\
        "DescriptorTable(UAV(u1, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)"

Texture2D InputParticles : register(t0);
RWTexture2D<float4> Scene : register(u0);
RWTexture2D<float> reactive : register(u1); 

cbuffer cb : register(b0)
{
    float factor; 
};


[RootSignature(ComputeRS)]
[numthreads(8, 8, 1)]
void main(uint2 did : SV_DispatchThreadID)
{
    float4 particleInput = InputParticles.Load(uint3(did, 0));
 
    float3 scene = Scene.Load(did).rgb;

    float3 blend = (scene * (1.0f - particleInput.a)) + particleInput.rgb;

    Scene[did.xy] = float4(blend, 1); 
    reactive[did.xy] = min (1.0f, length(particleInput)*50.0f); //dialate into a reactive mask for FSR2
}

