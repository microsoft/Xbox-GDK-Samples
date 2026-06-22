// TODO: Should have an immediate descriptor for SMEM test

// Keep these in sync with the root signature below!
#define ROOT_PARAMETER_INDEX_CBV        0
#define ROOT_PARAMETER_INDEX_SAMPLER    1
#define ROOT_PARAMETER_INDEX_SRV        2
#define ROOT_PARAMETER_INDEX_UAV        3

#if defined(__XBOX_SCARLETT) || defined(__XBOX_ONE)

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

#else
//  on PC we specify the number of descriptors to be constant to supress debug layer's
//
//  [COMPILATION MESSAGE #1277: CREATE_ROOT_SIGNATURE_UNBOUNDED_STATIC_DESCRIPTORS]
//      MESSAGE: ID3D12Device::CreateRootSignature : The specified root signature contains
//      at least one descriptor range which was declared both unbounded(NumDescriptors == UINT_MAX) and
//      static (missing D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE or
//      containing D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS).
//      This combination is ignored and treated as DESCRIPTORS_VOLATILE. To enable static descriptor driver
//      optimizations or debug validation, specify a bounded descriptor table size.
//
//  We also initialize them to 1 to make sure the following ERROR doesn't trigger when don't initialize all descriptors in the heap
//
// [EXECUTION ERROR #646: INVALID_DESCRIPTOR_HANDLE]
//      D3D12 ERROR : CGraphicsCommandList::SetGraphicsRootDescriptorTable :
//      Specified GPU Descriptor Handle(ptr = 'addr' at 'i' offsetInDescriptorsFromDescriptorHeapStart),
//      for Root Signature('addr':'name')'s Descriptor Table (at Parameter Index ['i'])'s Descriptor Range(at Range Index['k']
//      of type D3D12_DESCRIPTOR_RANGE_TYPE_N) has not been initialized. All descriptors of descriptor ranges declared STATIC
//      (not - DESCRIPTORS_VOLATILE) in a root signature must be initialized prior to being set on the command list.

#define ROOT_SIGNATURE\
    RootSignature\
    (\
        "\
            DescriptorTable(CBV(b0, numDescriptors = 1)),\
            DescriptorTable(Sampler(s0, numDescriptors = 1)),\
            DescriptorTable(SRV(t0, numDescriptors = 1)),\
            DescriptorTable(UAV(u0, numDescriptors = 1)),\
        "\
    )\

#endif
