//--------------------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------------------

#include "pch.h"

#include "Radix.h"

#include "CompiledShaders\PrefixSum64.csh"
#include "CompiledShaders\PrefixSum128.csh"
#include "CompiledShaders\PrefixSum256.csh"
#include "CompiledShaders\PrefixSum512.csh"
#include "CompiledShaders\PrefixSum1024.csh"
#include "CompiledShaders\PrefixSum2048.csh"

#include "CompiledShaders\RadixCount16.csh"
#include "CompiledShaders\RadixAlloc16.csh"

#define PrefixSumPsoList \
    PsoEntry(PrefixSum64)   \
    PsoEntry(PrefixSum128)  \
    PsoEntry(PrefixSum256)  \
    PsoEntry(PrefixSum512)  \
    PsoEntry(PrefixSum1024) \
    PsoEntry(PrefixSum2048) \

#define RadixCountAndAllocPsoList   \
    PsoEntry(RadixCount16)          \
    PsoEntry(RadixAlloc16)          \

#define RadixPsoList            \
    RadixCountAndAllocPsoList   \
    PrefixSumPsoList            \

typedef enum RadixPsoId
{
#define PsoEntry(name) kRadixPsoId_##name,
    RadixPsoList
#undef PsoEntry
} RadixPsoId;

static D3D12_SHADER_BYTECODE GRadixShaderBytecode [] =
{
#define PsoEntry(name) { g_##name, sizeof(g_##name) },
    RadixPsoList
#undef PsoEntry
};

typedef struct RadixPsoAndRs
{
    ID3D12RootSignature *m_rsCountAndAlloc;
    ID3D12RootSignature *m_rsPrefixSumL1L2;

#define PsoEntry(name) ID3D12PipelineState *m_pso##name;
    RadixPsoList
#undef PsoEntry
} RadixPsoAndRs;

static struct RadixPsoAndRs GRadixPsoAndRs;

typedef enum PrefixSumPassLimits
{
    kPrefixSumPass_LanesPerWave         = 64,   /**< Can be changed for Scarlett */
    kPrefixSumPass_MaxUintsPerLane      = 8,    /**< No more than eight 4-byte elements per lane to fit into 2 buffer_load_dwordx4*/

    kPrefixSumPass_MinUintsPerWave      = kPrefixSumPass_LanesPerWave,
    kPrefixSumPass_MaxUintsPerWave      = kPrefixSumPass_LanesPerWave * kPrefixSumPass_MaxUintsPerLane,

    kPrefixSumPass_MaxUintsPerTG        = 4 * 64 * kPrefixSumPass_MaxUintsPerLane,
    kPrefixSumPass_WavesPerTG           = kPrefixSumPass_MaxUintsPerTG / kPrefixSumPass_MaxUintsPerWave,
} PrefixSumPassLimits;

static inline void checkHRESULT(HRESULT hr)
{
    if (hr != S_OK)
    {
        __debugbreak();
    }
}

#define RADIX_ASSERT_MSG(cond, msg)                     \
    do                                                  \
    {                                                   \
        if (!(cond))                                    \
        {                                               \
            OutputDebugStringA("RadixSort :: " msg);    \
            __debugbreak();                             \
        }                                               \
    }                                                   \
    while (0);

#define RADIX_ASSERT(cond) RADIX_ASSERT_MSG(cond, #cond)

static inline ID3D12RootSignature * createRS(ID3D12Device *dev, D3D12_SHADER_BYTECODE const *bcode)
{
    ID3D12RootSignature *rs = 0;
    HRESULT hr = dev->CreateRootSignature(0, bcode->pShaderBytecode, bcode->BytecodeLength, IID_ID3D12RootSignature, (void **)&rs);
    checkHRESULT(hr);
    return rs;
}

static inline ID3D12PipelineState * createPSO(ID3D12Device *dev, D3D12_COMPUTE_PIPELINE_STATE_DESC const *psoDesc)
{
    ID3D12PipelineState *pso = 0;
    HRESULT hr = dev->CreateComputePipelineState(psoDesc, IID_ID3D12PipelineState, (void **)&pso);
    checkHRESULT(hr);
    return pso;
}

static inline ID3D12Resource * createUavRes(ID3D12Device *dev, D3D12_HEAP_PROPERTIES const *heapProps, D3D12_RESOURCE_DESC const *resDesc)
{
    ID3D12Resource *res = 0;
    HRESULT hr = dev->CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, resDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, IID_ID3D12Resource, (void **)&res);
    checkHRESULT(hr);
    return res;
}

static inline uint32_t minU32(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}
static inline uint32_t maxU32(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

static inline bool IsPow2(uint32_t x)
{
    return (x & (x - 1)) == 0;
}

static uint32_t nextPow2(uint32_t v)
{
    uint32_t x = v;
    RADIX_ASSERT_MSG(x != 0, "nextPow2 doesn't handle input 0");
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ++x;
    RADIX_ASSERT_MSG(IsPow2(x), "nextPow2 returns non-power-of-2");
    RADIX_ASSERT_MSG((IsPow2(v) && (v == x)) || (!IsPow2(v) && x > v), "nextPow2 shouldn't return non-modified input pow-of-2");
    return x;
}

static uint8_t countTrailingZeros(uint32_t x)
{
    uint8_t sum = 0, shift = 0;
    uint32_t v = x;

    shift = (x & 0xffff) == 0 ? 16u : 0;
    x  >>= shift;
    sum += shift;

    shift = (x & 0x00ff) == 0 ?  8u : 0;
    x  >>= shift;
    sum += shift;

    shift = (x & 0x000f) == 0 ?  4u : 0;
    x  >>= shift;
    sum += shift;

    shift = (x & 0x0003) == 0 ?  2u : 0;
    x  >>= shift;
    sum += shift;

    shift = (x & 0x0001) == 0 ?  1u : 0;
    x  >>= shift;
    sum += shift;

    RADIX_ASSERT_MSG(( (1u << sum) & v) != 0, "ctz overestimates");
    RADIX_ASSERT_MSG((~(1u << sum) & v) == 0, "ctz overestimates");
    return sum;
}


static inline uint32_t shiftRightAndRoundUp(uint32_t x, uint8_t shift)
{
    RADIX_ASSERT_MSG(shift < 32, "shiftRightAndRoundUp receives too big 'shift' parameter");
    return (x + ((1u << shift) - 1u)) >> shift;
}

static void computePrefixPassConfig(uint32_t *outPrefixL1WidthLog2, uint32_t *outPrefixL2WidthLog2, uint32_t totalReservedCounters)
{
    RADIX_ASSERT_MSG(IsPow2(totalReservedCounters), "computePrefixPassConfig expects that 'totalReservedCounters' is pow of 2");

    /** The smallest kernel computes prefix sum of a packet of 4-byte counters by a single wave
     *  where each lane process one 4-byte counter */
    const uint32_t minPrefixKernelSizeLog2 = countTrailingZeros(kPrefixSumPass_LanesPerWave);

    /** The widest kernel computes prefix sum of a packet of 4-byte counters by multiple waves
     *  where each lane process up to eight 4-byte counters */
    const uint32_t maxPrefixKernelSizeLog2 = countTrailingZeros(kPrefixSumPass_MaxUintsPerTG);

    // compute the log2 of the size of the prefix sum of all counters
    const uint32_t totalPrefixSumSizeLog2 = countTrailingZeros(totalReservedCounters);

    /** By default, assume that the global prefix sum can by computed in two passes with the smallest kernels */
    *outPrefixL1WidthLog2 = minPrefixKernelSizeLog2;
    *outPrefixL2WidthLog2 = minPrefixKernelSizeLog2;

    /** Check if prefix sum can be computed in one pass by the widest supported kernel */
    if (totalPrefixSumSizeLog2 <= maxPrefixKernelSizeLog2)
    {
        *outPrefixL1WidthLog2 = maxU32(minPrefixKernelSizeLog2, totalPrefixSumSizeLog2);
    }
    /** Adjust sizes of kernels for two consecutive passes */
    else if (totalPrefixSumSizeLog2 > minPrefixKernelSizeLog2 * 2)
    {
        /** Compute the difference between the size of the prefix that should be computed and
         *  the size of the prefix that could be computed by using just two smallest kernel */
        const uint32_t log2SizeRequiredIncrement = totalPrefixSumSizeLog2 - (minPrefixKernelSizeLog2 << 1u);

        /** Select by how much the size of the prefix kernel for the second level should be incremented
         *  The widest possible kernel is chosen because this kernel finishes quickly as it's launched as one thread group  */
        const uint32_t log2PrefixL2SizeIncrement =  minU32(log2SizeRequiredIncrement, maxPrefixKernelSizeLog2 - minPrefixKernelSizeLog2);

        /** The size of the prefix kernel of the first level is incremented by the remaining part */
        const uint32_t log2PrefixL1SizeIncrement = log2SizeRequiredIncrement - log2PrefixL2SizeIncrement;

        *outPrefixL1WidthLog2 += log2PrefixL1SizeIncrement;
        *outPrefixL2WidthLog2 += log2PrefixL2SizeIncrement;
    }
}

static void initCommonUavBufferDesc(D3D12_RESOURCE_DESC *outDesc, D3D12_RESOURCE_FLAGS AdditionalFlags)
{
    *outDesc = {
        /*.Dimension          = */ D3D12_RESOURCE_DIMENSION_BUFFER,
        /*.Alignment          = */ 0,
        /*.Width              = */ 0,
        /*.Height             = */ 1,
        /*.DepthOrArraySize   = */ 1,
        /*.MipLevels          = */ 1,
        /*.Format             = */ DXGI_FORMAT_UNKNOWN,
        /*.SampleDesc.Count   = */ 1,
        /*.SampleDesc.Quality = */ 0,
        /*.Layout             = */ D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        /*.Flags              = */ D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | AdditionalFlags
    };
}

static inline void resByteBufferSetNumElements(D3D12_RESOURCE_DESC *resDesc, uint32_t numElements)
{
    resDesc->Width = numElements * sizeof(uint32_t);
}

void radixInit(struct ID3D12Device *device)
{
    GRadixPsoAndRs.m_rsCountAndAlloc = createRS(device, &GRadixShaderBytecode[kRadixPsoId_RadixCount16]);
    GRadixPsoAndRs.m_rsPrefixSumL1L2 = createRS(device, &GRadixShaderBytecode[kRadixPsoId_PrefixSum64]);

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc;
    psoDesc.NodeMask    = 0;
    psoDesc.CachedPSO   = { 0, 0 };
    psoDesc.Flags       = D3D12_PIPELINE_STATE_FLAG_NONE;

#define PsoEntry(name) \
    psoDesc.CS = { g_##name, sizeof(g_##name) }; \
    GRadixPsoAndRs.m_pso##name = createPSO(device, &psoDesc);

    psoDesc.pRootSignature = GRadixPsoAndRs.m_rsCountAndAlloc;
    RadixCountAndAllocPsoList

    psoDesc.pRootSignature = GRadixPsoAndRs.m_rsPrefixSumL1L2;
    PrefixSumPsoList
#undef PsoEntry

}

void radixTerm(void)
{
#define PsoEntry(name) GRadixPsoAndRs.m_pso##name->Release();
    RadixPsoList
#undef PsoEntry
    GRadixPsoAndRs.m_rsCountAndAlloc->Release();
    GRadixPsoAndRs.m_rsPrefixSumL1L2->Release();
}

static const uint32_t subKeyWidth = 8;

static void radixUpdateRequiredCounters(RadixConfig *inoutConfig, uint32_t numU32Counters)
{
    uint32_t log2PrefixSizeL1 = 0;
    uint32_t log2PrefixSizeL2 = 0;

    computePrefixPassConfig(&log2PrefixSizeL1, &log2PrefixSizeL2, nextPow2(numU32Counters));

    RADIX_ASSERT((1 << log2PrefixSizeL1) >= kPrefixSumPass_LanesPerWave && (1 << log2PrefixSizeL1) <= kPrefixSumPass_MaxUintsPerTG);
    RADIX_ASSERT((1 << log2PrefixSizeL2) >= kPrefixSumPass_LanesPerWave && (1 << log2PrefixSizeL2) <= kPrefixSumPass_MaxUintsPerTG);

    uint32_t dispatchSizeL1 = shiftRightAndRoundUp(numU32Counters, (uint8_t)log2PrefixSizeL1);
    uint32_t dispatchSizeL2 = shiftRightAndRoundUp(dispatchSizeL1, (uint8_t)log2PrefixSizeL2);

    RADIX_ASSERT(dispatchSizeL1 <= kPrefixSumPass_MaxUintsPerTG);
    RADIX_ASSERT(dispatchSizeL2 == 1);

    inoutConfig->m_numU32CountersRequired = dispatchSizeL1 << log2PrefixSizeL1;

    inoutConfig->m_log2PrefixSizeL1 = (uint8_t)log2PrefixSizeL1;
    inoutConfig->m_log2PrefixSizeL2 = (uint8_t)log2PrefixSizeL2;

    inoutConfig->m_dispatchSizeL1 = (uint16_t)dispatchSizeL1;
}

void radixInitConfig(RadixConfig *outConfig, uint32_t numElements)
{
    outConfig->m_numElements    = numElements;
    outConfig->m_numSubkeyBits  = subKeyWidth;
    outConfig->m_log2KernelSize = 11;
    outConfig->m_numKernels     = static_cast<uint16_t>(shiftRightAndRoundUp(numElements, outConfig->m_log2KernelSize));

    /** Prefix sum could be computed for at least 64 uint32 elements/counters.
     *  This limit is defined by the size of compute kernel.
     *  By computing the number of 256-byte blocks that are processed by prefix sum kernels it's possible
     *  to compute how many 4-byte counters are touched per group, per single subkey.
     */
    const uint32_t num256ByteBlocks = shiftRightAndRoundUp(outConfig->m_numKernels, 6);

    /** Knowing how many 256-byte blocks storing 4-byte counters are necessary per subkey, it's possible to compute
     *  total number of 4-byte counters */
    radixUpdateRequiredCounters(outConfig, num256ByteBlocks << (outConfig->m_numSubkeyBits + 6));
}

/** Helper macro list of resource initialization commands. In-place macro definition is required per command */
#define RadixResourceCreationOpSequence(info)           \
    ByteBufferInitOp(D3D12_RESOURCE_FLAG_NONE)          \
    ByteBufferSetNumElementsOp(info.numU32Keys)         \
    ResourceCreationOp(kRadixResId_Subkeys)             \
    ByteBufferSetNumElementsOp(info.numU32Keys)         \
    ResourceCreationOp(kRadixResId_Payload)             \
    ByteBufferSetNumElementsOp(info.numU32PrefixL1)     \
    ResourceCreationOp(kRadixResId_PrefixL1)            \
    ByteBufferSetNumElementsOp(info.numU32PrefixL2)     \
    ResourceCreationOp(kRadixResId_PrefixL2)            \

typedef struct RadixResourceInfo
{
    uint32_t numU32Keys;
    uint32_t numU32PrefixL1;
    uint32_t numU32PrefixL2;
} RadixResourceInfo;

static inline void radixInitResourceInfo(RadixResourceInfo *outInfo, RadixConfig const *config)
{
    outInfo->numU32Keys         = config->m_numElements;
    outInfo->numU32PrefixL1     = config->m_numU32CountersRequired;
    outInfo->numU32PrefixL2     = 1u << config->m_log2PrefixSizeL2;
}

static inline void radixCreateCommittedRes(RadixResources *outRes, uint32_t mask, RadixResId resId, struct ID3D12Device *device, D3D12_RESOURCE_DESC const *resDesc, D3D12_HEAP_PROPERTIES const *heapProps)
{
    if (mask & (1u << resId))
    {
        ID3D12Resource *res = createUavRes(device, heapProps, resDesc);
        outRes->m_d3dPtrs[resId] = res;
        outRes->m_gpuAddr[resId] = res->GetGPUVirtualAddress();
    }
}

void radixCreateCommittedResources(RadixResources *outRes, uint32_t mask, struct ID3D12Device *device, RadixConfig const *config)
{
    RadixResourceInfo info;
    radixInitResourceInfo(&info, config);

    D3D12_HEAP_PROPERTIES heapProps = {
        /*.Type                 =*/ D3D12_HEAP_TYPE_DEFAULT,
        /*.CPUPageProperty      =*/ D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        /*.MemoryPoolPreference =*/ D3D12_MEMORY_POOL_UNKNOWN,
        /*.CreationNodeMask     =*/ 0x1,
        /*.VisibleNodeMask      =*/ 0x1
    };

    D3D12_RESOURCE_DESC resDesc;
#define ResourceCreationOp(id)                  radixCreateCommittedRes(outRes, mask, id, device, &resDesc, &heapProps);
#define ByteBufferInitOp(flags)                 initCommonUavBufferDesc(&resDesc, flags);
#define ByteBufferSetNumElementsOp(numElems)    resByteBufferSetNumElements(&resDesc, numElems);

    RadixResourceCreationOpSequence(info)

#undef ByteBufferSetNumElementsOp
#undef ByteBufferInitOp
#undef ResourceCreationOp
}

static void radixSubmitSubkeySortingPass(struct ID3D12GraphicsCommandList *cmdList, RadixConfig const *config, RadixResources const *resources, uint32_t ShiftSize)
{
    RadixResourceInfo info;
    radixInitResourceInfo(&info, config);

    const D3D12XBOX_FLUSH flushMask = D3D12XBOX_FLUSH_BOP_CS_PARTIAL | D3D12XBOX_FLUSH_BOP_TEXTURE_L1_INVALIDATE;

    const uint64_t *gpuAddr = resources->m_gpuAddr;
    {
        PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, "Clear prefix buffers");
        cmdList->FillMemoryWith32BitValueX(gpuAddr[kRadixResId_PrefixL1], info.numU32PrefixL1 * 4, 0, D3D12XBOX_COPY_FLAG_NONE);
        cmdList->FillMemoryWith32BitValueX(gpuAddr[kRadixResId_PrefixL2], info.numU32PrefixL2 * 4, 0, D3D12XBOX_COPY_FLAG_NONE);
        PIXEndEvent(cmdList);
    }
    cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
    {
        cmdList->SetComputeRootSignature(GRadixPsoAndRs.m_rsCountAndAlloc);
        cmdList->SetPipelineState(GRadixPsoAndRs.m_psoRadixCount16);

        cmdList->SetComputeRootShaderResourceView(0, gpuAddr[kRadixResId_Subkeys]);
        cmdList->SetComputeRootShaderResourceView(1, 0);
        cmdList->SetComputeRootShaderResourceView(2, 0);
        cmdList->SetComputeRootUnorderedAccessView(3, gpuAddr[kRadixResId_PrefixL1]);
        cmdList->SetComputeRoot32BitConstant(4, config->m_numU32CountersRequired >> config->m_numSubkeyBits, 0);
        cmdList->SetComputeRoot32BitConstant(4, ShiftSize, 1);
        cmdList->SetComputeRoot32BitConstant(4, config->m_log2PrefixSizeL1, 2);

        PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, "Count # of [0..%u] subkeys among %u keys", (1u << config->m_numSubkeyBits) - 1u, 1u << config->m_log2KernelSize);
        cmdList->Dispatch(config->m_numKernels, 1, 1);
        PIXEndEvent(cmdList);
    }
    cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
    {
        /** Select PSOs for prefix sum passes */
        ID3D12PipelineState *prefixSumPsos[] =
        {
            GRadixPsoAndRs.m_psoPrefixSum64,
            GRadixPsoAndRs.m_psoPrefixSum128,
            GRadixPsoAndRs.m_psoPrefixSum256,
            GRadixPsoAndRs.m_psoPrefixSum512,
            GRadixPsoAndRs.m_psoPrefixSum1024,
            GRadixPsoAndRs.m_psoPrefixSum2048
        };

        uint32_t psoIndexL1 = config->m_log2PrefixSizeL1 - 6u;
        uint32_t psoIndexL2 = config->m_log2PrefixSizeL2 - 6u;

        cmdList->SetComputeRootSignature(GRadixPsoAndRs.m_rsPrefixSumL1L2);
        if (config->m_dispatchSizeL1 == 1u)
        {
            cmdList->SetPipelineState(prefixSumPsos[psoIndexL1]);

            cmdList->SetComputeRootUnorderedAccessView(0, gpuAddr[kRadixResId_PrefixL1]);
            cmdList->SetComputeRootUnorderedAccessView(1, 0);

            cmdList->SetComputeRoot32BitConstant(2, 0, 0);

            cmdList->Dispatch(1, 1, 1);
        }
        else
        {
            cmdList->SetPipelineState(prefixSumPsos[psoIndexL1]);

            cmdList->SetComputeRootUnorderedAccessView(0, gpuAddr[kRadixResId_PrefixL1]);
            cmdList->SetComputeRootUnorderedAccessView(1, gpuAddr[kRadixResId_PrefixL2]);

            cmdList->SetComputeRoot32BitConstant(2, 1, 0);

            PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, "Prefix sum of %u counters per kernel (Lvl1)", 1u << config->m_log2PrefixSizeL1);
            cmdList->Dispatch(config->m_dispatchSizeL1, 1, 1);
            PIXEndEvent(cmdList);

            cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);

            if (psoIndexL1 != psoIndexL2)
                cmdList->SetPipelineState(prefixSumPsos[psoIndexL2]);

            cmdList->SetComputeRootUnorderedAccessView(0, gpuAddr[kRadixResId_PrefixL2]);
            cmdList->SetComputeRootUnorderedAccessView(1, 0);

            cmdList->SetComputeRoot32BitConstant(2, 0, 0);

            PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, "Prefix sum of %u counters per kernel (Lvl2)", 1u << config->m_log2PrefixSizeL2);
            cmdList->Dispatch(1, 1, 1);
            PIXEndEvent(cmdList);
        }
    }
    cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
    {
        cmdList->SetComputeRootSignature(GRadixPsoAndRs.m_rsCountAndAlloc);
        cmdList->SetPipelineState(GRadixPsoAndRs.m_psoRadixAlloc16);

        cmdList->SetComputeRootShaderResourceView(0, gpuAddr[kRadixResId_Subkeys]);
        cmdList->SetComputeRootShaderResourceView(1, gpuAddr[kRadixResId_PrefixL1]);
        cmdList->SetComputeRootShaderResourceView(2, gpuAddr[kRadixResId_PrefixL2]);
        cmdList->SetComputeRootUnorderedAccessView(3, gpuAddr[kRadixResId_Payload]);
        cmdList->SetComputeRoot32BitConstant(4, config->m_numU32CountersRequired >> config->m_numSubkeyBits, 0);
        cmdList->SetComputeRoot32BitConstant(4, ShiftSize, 1);
        cmdList->SetComputeRoot32BitConstant(4, config->m_log2PrefixSizeL1, 2);

        PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, "Shuffle %u keys", 1u << config->m_log2KernelSize);
        cmdList->Dispatch(config->m_numKernels, 1, 1);
        PIXEndEvent(cmdList);
    }
    cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
}

void radixSubmit(struct ID3D12GraphicsCommandList *cmdList, RadixConfig const *config, RadixResources *resources)
{
    const uint32_t numPasses = 32 / subKeyWidth;
    for (uint32_t i = 0; i < numPasses; ++i)
    {
        PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, "Subpass %u, subkey[%u:%u]", i, i * subKeyWidth, (i + 1) * subKeyWidth - 1);
        radixSubmitSubkeySortingPass(cmdList, config, resources, subKeyWidth * i);
        PIXEndEvent(cmdList);

        uint64_t tmpSubkeys = resources->m_gpuAddr[kRadixResId_Subkeys];
        uint64_t tmpPayload = resources->m_gpuAddr[kRadixResId_Payload];

        resources->m_gpuAddr[kRadixResId_Subkeys] = tmpPayload;
        resources->m_gpuAddr[kRadixResId_Payload] = tmpSubkeys;
    }
}
