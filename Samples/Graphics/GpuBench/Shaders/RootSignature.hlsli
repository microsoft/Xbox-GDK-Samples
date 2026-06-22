// TODO: Should have an immediate descriptor for SMEM test

// Keep these in sync with the root signature below!
#define ROOT_PARAMETER_INDEX_CBV        0
#define ROOT_PARAMETER_INDEX_SAMPLER    1
#define ROOT_PARAMETER_INDEX_SRV        2
#define ROOT_PARAMETER_INDEX_UAV        3

#define ROOT_SIGNATURE\
    RootSignature\
    (\
        "\
            DescriptorTable(CBV(b0, numDescriptors = unbounded)),\
            DescriptorTable(Sampler(s0, numDescriptors = unbounded)),\
            DescriptorTable(SRV(t0, numDescriptors = unbounded)),\
            DescriptorTable(UAV(u0, numDescriptors = unbounded)),\
        "\
    )\

