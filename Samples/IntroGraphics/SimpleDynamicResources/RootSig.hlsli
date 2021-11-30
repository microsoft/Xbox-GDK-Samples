// Enable dynamic indexing into descriptor heap with CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED.
// Enable dynamic indexing into sampler heap with SAMPLER_HEAP_DIRECTLY_INDEXED.
// This enables use of the ResourceDescriptorHeap[] and SamplerDescriptorHeap[] keywords in HLSL SM 6.6,
// which allows "bindless" access of resources and samplers directly from the descriptor heaps,
// without needing them to be bound in the root signature.
#define MainRS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | SAMPLER_HEAP_DIRECTLY_INDEXED  | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)"
