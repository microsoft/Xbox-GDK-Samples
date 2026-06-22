//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "PixelBinner.h"

#include "CompiledShaders\PixelCount32_NoInWave.csh"
#include "CompiledShaders\PixelCount32_ScalarInWave.csh"
#include "CompiledShaders\PixelCount32_VectorInWave.csh"
#include "CompiledShaders\PixelCount512_NoInWave.csh"
#include "CompiledShaders\PixelCount1024_NoInWave.csh"
#include "CompiledShaders\PixelCount2048_NoInWave.csh"

#include "CompiledShaders\PixelAlloc32_NoInWave.csh"
#include "CompiledShaders\PixelAlloc32_ScalarInWave.csh"
#include "CompiledShaders\PixelAlloc32_VectorInWave.csh"
#include "CompiledShaders\PixelAlloc512_NoInWave.csh"
#include "CompiledShaders\PixelAlloc1024_NoInWave.csh"
#include "CompiledShaders\PixelAlloc2048_NoInWave.csh"

#include "CompiledShaders\PrefixSum64.csh"
#include "CompiledShaders\PrefixSum128.csh"
#include "CompiledShaders\PrefixSum256.csh"
#include "CompiledShaders\PrefixSum512.csh"
#include "CompiledShaders\PrefixSum1024.csh"
#include "CompiledShaders\PrefixSum2048.csh"

#include "CompiledShaders\DebugVis_BinIdDivergence1024.csh"
#include "CompiledShaders\DebugVis_BinIdDivergence2048.csh"
#include "CompiledShaders\DebugVis_BinIdToColor.csh"
#include "CompiledShaders\DebugVis_TouchOnce.csh"
#include "CompiledShaders\DebugVis_TouchOnceTest.csh"

#include "CompiledShaders\WriteExecuteIndirectArgs.csh"

#define PixelCountPsoList \
    PsoEntry(PixelCount32_NoInWave)     \
    PsoEntry(PixelCount32_ScalarInWave) \
    PsoEntry(PixelCount32_VectorInWave) \
    PsoEntry(PixelCount512_NoInWave)    \
    PsoEntry(PixelCount1024_NoInWave)   \
    PsoEntry(PixelCount2048_NoInWave)

#define PixelAllocPsoList \
    PsoEntry(PixelAlloc32_NoInWave)     \
    PsoEntry(PixelAlloc32_ScalarInWave) \
    PsoEntry(PixelAlloc32_VectorInWave) \
    PsoEntry(PixelAlloc512_NoInWave)    \
    PsoEntry(PixelAlloc1024_NoInWave)   \
    PsoEntry(PixelAlloc2048_NoInWave)

#define PrefixSumPsoList \
    PsoEntry(PrefixSum64)   \
    PsoEntry(PrefixSum128)  \
    PsoEntry(PrefixSum256)  \
    PsoEntry(PrefixSum512)  \
    PsoEntry(PrefixSum1024) \
    PsoEntry(PrefixSum2048) \

#define DebugViewPsoList1 \
    PsoEntry(DebugVis_BinIdDivergence1024)  \
    PsoEntry(DebugVis_BinIdDivergence2048)

#define DebugViewPsoList2 \
    PsoEntry(DebugVis_TouchOnce)      \
    PsoEntry(DebugVis_TouchOnceTest)

#define PixelBinnerPsoList  \
    PixelCountPsoList       \
    PixelAllocPsoList       \
    PrefixSumPsoList        \
    DebugViewPsoList1       \
    DebugViewPsoList2       \
    PsoEntry(DebugVis_BinIdToColor) \
    PsoEntry(WriteExecuteIndirectArgs)

typedef enum PixelBinnerPsoId
{
#define PsoEntry(name) kPsoId_##name,
    PixelBinnerPsoList
#undef PsoEntry
} PixelBinnerPsoId;

static D3D12_SHADER_BYTECODE GPixelBinnerShaderBytecode [] =
{
#define PsoEntry(name) { g_##name, sizeof(g_##name) },
    PixelBinnerPsoList
#undef PsoEntry
};

static char const * GPixelBinnerPsoNames[] =
{
#define PsoEntry(name) #name,
    PixelBinnerPsoList
#undef PsoEntry
};

typedef enum PrefixSumPassLimits
{
    kPrefixSumPass_LanesPerWave         = 64,   /**< Can be changed for Scarlett */
    kPrefixSumPass_MaxUintsPerLane      = 8,    /**< No more than eight 4-byte elements per lane to fit into 2 buffer_load_dwordx4*/

    kPrefixSumPass_MinUintsPerWave      = kPrefixSumPass_LanesPerWave,
    kPrefixSumPass_MaxUintsPerWave      = kPrefixSumPass_LanesPerWave * kPrefixSumPass_MaxUintsPerLane,

    kPrefixSumPass_MaxUintsPerTG        = 4 * 64 * kPrefixSumPass_MaxUintsPerLane,
    kPrefixSumPass_WavesPerTG           = kPrefixSumPass_MaxUintsPerTG / kPrefixSumPass_MaxUintsPerWave,
} PrefixSumPassLimits;

typedef enum PixelBinnerLimits
{
    kPixelBinnerLimitsMinBins = 2,
    kPixelBinnerLimitsMaxBins = 2048,   /**< Currently limited by the pixel allocation pass which handles bin ids with up to 11 bits in width */

    kPixelBinnerLimitsMinCounters = kPrefixSumPass_MinUintsPerWave,
    kPixelBinnerLimitsMaxCounters = kPrefixSumPass_MaxUintsPerTG * kPrefixSumPass_MaxUintsPerTG
} PixelBinnerLimits;

struct PixelBinnerPsoAndRs
{
    ID3D12RootSignature *m_rsDebugVisBinIdDivergence;
    ID3D12RootSignature *m_rsDebugVisBinIdToColor;
    ID3D12RootSignature *m_rsDebugVisTouchOnceTest;
    ID3D12RootSignature *m_rsCountAndAlloc;
    ID3D12RootSignature *m_rsPrefixSumL1L2;
    ID3D12RootSignature *m_rsWriteExecuteIndirectArgs;

    ID3D12CommandSignature *m_csWriteExecuteIndirectArgs;

#define PsoEntry(name) ID3D12PipelineState *m_pso##name;
    PixelBinnerPsoList
#undef PsoEntry
} ;

static struct PixelBinnerPsoAndRs GPixelBinnerPsoAndRs;

static inline void checkHRESULT(HRESULT hr)
{
    if (hr != S_OK)
    {
        __debugbreak();
    }
}

#define PIXBIN_ASSERT_MSG(cond, msg)                    \
    do                                                  \
    {                                                   \
        if (!(cond))                                    \
        {                                               \
            OutputDebugStringA("PixelBinner :: " msg);  \
            __debugbreak();                             \
        }                                               \
    }                                                   \
    while (0);

#define PIXBIN_ASSERT(cond) PIXBIN_ASSERT_MSG(cond, #cond)

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

static inline PixelBinnerMemSizeAlign queryResMemReq(ID3D12Device *dev, D3D12_RESOURCE_DESC const *desc)
{
    D3D12_RESOURCE_ALLOCATION_INFO info = dev->GetResourceAllocationInfo(0, 1, desc);
    PIXBIN_ASSERT(info.SizeInBytes < ~0u);
    PIXBIN_ASSERT(info.Alignment < ~0u);
    PixelBinnerMemSizeAlign sizeAlign = { (uint32_t)info.SizeInBytes, (uint32_t)info.Alignment };
    return sizeAlign;
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
    PIXBIN_ASSERT_MSG(x != 0, "nextPow2 doesn't handle input 0");
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ++x;
    PIXBIN_ASSERT_MSG(IsPow2(x), "nextPow2 returns non-power-of-2");
    PIXBIN_ASSERT_MSG((IsPow2(v) && (v == x)) || (!IsPow2(v) && x > v), "nextPow2 shouldn't return non-modified input pow-of-2");
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

    PIXBIN_ASSERT_MSG(( (1u << sum) & v) != 0, "ctz overestimates");
    PIXBIN_ASSERT_MSG((~(1u << sum) & v) == 0, "ctz overestimates");
    return sum;
}

static inline uint32_t shiftRightAndRoundUp(uint32_t x, uint8_t shift)
{
    PIXBIN_ASSERT_MSG(shift < 32, "shiftRightAndRoundUp receives too big 'shift' parameter");
    return (x + ((1u << shift) - 1u)) >> shift;
}

static void computePrefixPassConfig(uint32_t *outPrefixL1WidthLog2, uint32_t *outPrefixL2WidthLog2, uint32_t totalReservedCounters)
{
    PIXBIN_ASSERT_MSG(IsPow2(totalReservedCounters), "computePrefixPassConfig expects that 'totalReservedCounters' is pow of 2");

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

static void updateRequiredCounters(PixelBinnerConfig *inoutConfig, uint32_t numU32Counters)
{
    uint32_t log2PrefixSizeL1 = 0;
    uint32_t log2PrefixSizeL2 = 0;

    computePrefixPassConfig(&log2PrefixSizeL1, &log2PrefixSizeL2, nextPow2(numU32Counters));

    PIXBIN_ASSERT((1 << log2PrefixSizeL1) >= kPrefixSumPass_LanesPerWave && (1 << log2PrefixSizeL1) <= kPrefixSumPass_MaxUintsPerTG);
    PIXBIN_ASSERT((1 << log2PrefixSizeL2) >= kPrefixSumPass_LanesPerWave && (1 << log2PrefixSizeL2) <= kPrefixSumPass_MaxUintsPerTG);

    uint32_t dispatchSizeL1 = shiftRightAndRoundUp(numU32Counters, (uint8_t)log2PrefixSizeL1);
    uint32_t dispatchSizeL2 = shiftRightAndRoundUp(dispatchSizeL1, (uint8_t)log2PrefixSizeL2);

    PIXBIN_ASSERT(dispatchSizeL1 <= kPrefixSumPass_MaxUintsPerTG);
    PIXBIN_ASSERT(dispatchSizeL2 == 1);

    inoutConfig->m_numU32CountersRequired = dispatchSizeL1 << log2PrefixSizeL1;

    inoutConfig->m_log2PrefixSizeL1 = (uint8_t)log2PrefixSizeL1;
    inoutConfig->m_log2PrefixSizeL2 = (uint8_t)log2PrefixSizeL2;

    inoutConfig->m_dispatchSizeL1 = (uint16_t)dispatchSizeL1;
}

static inline uint32_t computeCountersFromMacroTiles(PixelBinnerConfig const *c)
{
    return (uint32_t)c->m_numMacroTilesX * c->m_numMacroTilesY * c->m_numBinsRequired;
}

typedef struct PixelBinnerCountAndAllocCbData
{
    uint32_t resW;
    uint32_t resH;
    uint32_t binIdBitWidth;

    uint32_t microTilesPerMacroTileXLog2;
    uint32_t microTilesPerMacroTileYLog2;

    uint32_t macroTilesPerImageRow;
    uint32_t macroTilesPerImage;

    uint32_t numBitsPerL1;
    uint32_t numBitsPerL2;
    uint32_t packingType;
} PixelBinnerCountAndAllocCbData;

typedef struct PixelBinnerRsAndPsoData
{
    ID3D12RootSignature *m_rsCountAndAlloc;
    ID3D12RootSignature *m_rsPrefixSumL1L2;

    ID3D12PipelineState *m_psoPrefixSumL1;
    ID3D12PipelineState *m_psoPrefixSumL2;

    ID3D12PipelineState *m_psoCount;
    ID3D12PipelineState *m_psoAlloc;

    char const *m_prefixSumL1Name;
    char const *m_prefixSumL2Name;
} PixelBinnerRsAndPsoData;

static void prepareConstantBuffer(PixelBinnerCountAndAllocCbData *outCbData, PixelBinnerConfig const *pbConfig)
{
    /** Prepare data for constant buffer for counting and allocation passes */
    outCbData->resW = pbConfig->m_resX;
    outCbData->resH = pbConfig->m_resY;
    outCbData->binIdBitWidth = countTrailingZeros(pbConfig->m_numBinsRequired);

    outCbData->microTilesPerMacroTileXLog2 = (uint32_t)(pbConfig->m_log2MacroTileSizeX - pbConfig->m_log2MicroTileSizeX);
    outCbData->microTilesPerMacroTileYLog2 = (uint32_t)(pbConfig->m_log2MacroTileSizeY - pbConfig->m_log2MicroTileSizeY);
    outCbData->macroTilesPerImageRow = pbConfig->m_numMacroTilesX;
    outCbData->macroTilesPerImage =  (uint32_t)(pbConfig->m_numMacroTilesX * pbConfig->m_numMacroTilesY);

    outCbData->numBitsPerL1 = pbConfig->m_log2PrefixSizeL1;
    outCbData->numBitsPerL2 = pbConfig->m_log2PrefixSizeL2;
    outCbData->packingType  = pbConfig->m_packingType;
    if (pbConfig->m_dispatchSizeL1 == 1u)
    {
        /** Disables reading second level prefix */
        outCbData->numBitsPerL2 = 0;
    }
}

static void prepareRootSignaturesAndPipelineStates(PixelBinnerRsAndPsoData *outRes, PixelBinnerConfig const *config)
{
    /** Select root signatures */
    outRes->m_rsCountAndAlloc = GPixelBinnerPsoAndRs.m_rsCountAndAlloc;
    outRes->m_rsPrefixSumL1L2 = GPixelBinnerPsoAndRs.m_rsPrefixSumL1L2;

    /** Select PSOs for prefix sum passes */
    ID3D12PipelineState *prefixSumPsos[] =
    {
        GPixelBinnerPsoAndRs.m_psoPrefixSum64,
        GPixelBinnerPsoAndRs.m_psoPrefixSum128,
        GPixelBinnerPsoAndRs.m_psoPrefixSum256,
        GPixelBinnerPsoAndRs.m_psoPrefixSum512,
        GPixelBinnerPsoAndRs.m_psoPrefixSum1024,
        GPixelBinnerPsoAndRs.m_psoPrefixSum2048
    };

    uint32_t psoIndexL1 = config->m_log2PrefixSizeL1 - 6u;
    uint32_t psoIndexL2 = config->m_log2PrefixSizeL2 - 6u;

    outRes->m_psoPrefixSumL1 = prefixSumPsos[psoIndexL1];
    outRes->m_psoPrefixSumL2 = prefixSumPsos[psoIndexL2];

    outRes->m_prefixSumL1Name = GPixelBinnerPsoNames[kPsoId_PrefixSum64 + psoIndexL1];
    outRes->m_prefixSumL2Name = GPixelBinnerPsoNames[kPsoId_PrefixSum64 + psoIndexL2];

    /** Set defaults Scarlett */
    uint32_t binsLimitWhenToUseNonAtomicCrossLaneCounting = 32;
    uint32_t defaultCountingTypeBelowTheLimit = 1;

    XSystemDeviceType deviceType = XSystemGetDeviceType();
    if (deviceType == XSystemDeviceType::XboxOneS)
    {
        binsLimitWhenToUseNonAtomicCrossLaneCounting = 8;
    }
    else if (deviceType == XSystemDeviceType::XboxOneX || deviceType == XSystemDeviceType::XboxOneXDevkit)
    {
        binsLimitWhenToUseNonAtomicCrossLaneCounting = 16;
    }

    /** Select PSOs for pixel counting and allocation passes */
    if (config->m_numBinsRequired > 1024)
    {
        outRes->m_psoCount = GPixelBinnerPsoAndRs.m_psoPixelCount2048_NoInWave;
        outRes->m_psoAlloc = GPixelBinnerPsoAndRs.m_psoPixelAlloc2048_NoInWave;
    }
    else if (config->m_numBinsRequired > 512)
    {
        outRes->m_psoCount = GPixelBinnerPsoAndRs.m_psoPixelCount1024_NoInWave;
        outRes->m_psoAlloc = GPixelBinnerPsoAndRs.m_psoPixelAlloc1024_NoInWave;
    }
    else if (config->m_numBinsRequired > binsLimitWhenToUseNonAtomicCrossLaneCounting)
    {
        outRes->m_psoCount = GPixelBinnerPsoAndRs.m_psoPixelCount512_NoInWave;
        outRes->m_psoAlloc = GPixelBinnerPsoAndRs.m_psoPixelAlloc512_NoInWave;
    }
    else
    {
        ID3D12PipelineState *countPsos[] =
        {
            GPixelBinnerPsoAndRs.m_psoPixelCount32_NoInWave,
            GPixelBinnerPsoAndRs.m_psoPixelCount32_ScalarInWave,
            GPixelBinnerPsoAndRs.m_psoPixelCount32_VectorInWave
        };

        ID3D12PipelineState *allocPsos[] =
        {
            GPixelBinnerPsoAndRs.m_psoPixelAlloc32_NoInWave,
            GPixelBinnerPsoAndRs.m_psoPixelAlloc32_ScalarInWave,
            GPixelBinnerPsoAndRs.m_psoPixelAlloc32_VectorInWave
        };
        /** The best option so far for the case when it's necessary to support up to 32 bins --
         *  use shader variant with in-wave counting based on scalar ALUs */
        uint32_t psoIndex = config->m_overridePSOIndex ? config->m_countingPSOIndex : defaultCountingTypeBelowTheLimit;
        outRes->m_psoCount = countPsos[psoIndex];
        outRes->m_psoAlloc = allocPsos[psoIndex];
    }
}

void pixelBinnerInit(struct ID3D12Device *device)
{
    GPixelBinnerPsoAndRs.m_rsDebugVisBinIdDivergence = createRS(device, &GPixelBinnerShaderBytecode[kPsoId_DebugVis_BinIdDivergence1024]);
    GPixelBinnerPsoAndRs.m_rsDebugVisBinIdToColor = createRS(device, &GPixelBinnerShaderBytecode[kPsoId_DebugVis_BinIdToColor]);
    GPixelBinnerPsoAndRs.m_rsDebugVisTouchOnceTest = createRS(device, &GPixelBinnerShaderBytecode[kPsoId_DebugVis_TouchOnce]);
    GPixelBinnerPsoAndRs.m_rsCountAndAlloc = createRS(device, &GPixelBinnerShaderBytecode[kPsoId_PixelCount512_NoInWave]);
    GPixelBinnerPsoAndRs.m_rsPrefixSumL1L2 = createRS(device, &GPixelBinnerShaderBytecode[kPsoId_PrefixSum64]);
    GPixelBinnerPsoAndRs.m_rsWriteExecuteIndirectArgs = createRS(device, &GPixelBinnerShaderBytecode[kPsoId_WriteExecuteIndirectArgs]);

    D3D12_INDIRECT_ARGUMENT_DESC argDescs[2];
    argDescs[0].Type                                = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
    argDescs[0].Constant.RootParameterIndex         = 3;
    argDescs[0].Constant.DestOffsetIn32BitValues    = 0;
    argDescs[0].Constant.Num32BitValuesToSet        = 2;
    argDescs[1].Type                                = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

    D3D12_COMMAND_SIGNATURE_DESC csDesc =
    {
        /*.ByteStride       = */    sizeof(uint32_t) * 5,
        /*.NumArgumentDescs = */    sizeof(argDescs) / sizeof(argDescs[0]),
        /*.pArgumentDescs   = */    argDescs,
        /*.NodeMask         = */    0
    };

    device->CreateCommandSignature
    (
        &csDesc,
        GPixelBinnerPsoAndRs.m_rsDebugVisTouchOnceTest,
        __uuidof(ID3D12CommandSignature),
        (void **)&GPixelBinnerPsoAndRs.m_csWriteExecuteIndirectArgs
    );

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc;
    psoDesc.NodeMask    = 0;
    psoDesc.CachedPSO   = { 0, 0 };
    psoDesc.Flags       = D3D12_PIPELINE_STATE_FLAG_NONE;

#define PsoEntry(name) \
    psoDesc.CS = { g_##name, sizeof(g_##name) }; \
    GPixelBinnerPsoAndRs.m_pso##name = createPSO(device, &psoDesc);

    psoDesc.pRootSignature = GPixelBinnerPsoAndRs.m_rsCountAndAlloc;
    PixelCountPsoList
    PixelAllocPsoList

    psoDesc.pRootSignature = GPixelBinnerPsoAndRs.m_rsPrefixSumL1L2;
    PrefixSumPsoList

    psoDesc.pRootSignature = GPixelBinnerPsoAndRs.m_rsDebugVisBinIdDivergence;
    DebugViewPsoList1

    psoDesc.pRootSignature = GPixelBinnerPsoAndRs.m_rsDebugVisTouchOnceTest;
    DebugViewPsoList2

    psoDesc.pRootSignature = GPixelBinnerPsoAndRs.m_rsDebugVisBinIdToColor;
    PsoEntry(DebugVis_BinIdToColor)

    psoDesc.pRootSignature = GPixelBinnerPsoAndRs.m_rsWriteExecuteIndirectArgs;
    PsoEntry(WriteExecuteIndirectArgs)
#undef PsoEntry

}

void pixelBinnerTerm(void)
{
#define PsoEntry(name) GPixelBinnerPsoAndRs.m_pso##name->Release();
    PixelBinnerPsoList
#undef PsoEntry
    GPixelBinnerPsoAndRs.m_csWriteExecuteIndirectArgs->Release();
    GPixelBinnerPsoAndRs.m_rsWriteExecuteIndirectArgs->Release();
    GPixelBinnerPsoAndRs.m_rsDebugVisBinIdDivergence->Release();
    GPixelBinnerPsoAndRs.m_rsDebugVisBinIdToColor->Release();
    GPixelBinnerPsoAndRs.m_rsDebugVisTouchOnceTest->Release();
    GPixelBinnerPsoAndRs.m_rsCountAndAlloc->Release();
    GPixelBinnerPsoAndRs.m_rsPrefixSumL1L2->Release();
}

void pixelBinnerGetLimits(uint16_t *outMaxBinsSupported, uint32_t *outMaxCountersSupported)
{
    if (outMaxBinsSupported)
        *outMaxBinsSupported = kPixelBinnerLimitsMaxBins;

    if (outMaxCountersSupported)
        *outMaxCountersSupported = kPixelBinnerLimitsMaxCounters;
}

void pixelBinnerApplyLimits(uint16_t *inoutNumBinsUsed, uint32_t *inoutNumCountersUsed)
{
    if (inoutNumBinsUsed)
    {
        uint32_t userNumBins = *inoutNumBinsUsed;

        userNumBins = maxU32(kPixelBinnerLimitsMinBins, userNumBins);
        userNumBins = minU32(kPixelBinnerLimitsMaxBins, userNumBins);

        *inoutNumBinsUsed = (uint16_t)nextPow2(userNumBins);
    }

    if (inoutNumCountersUsed)
    {
        uint32_t userNumCounters = *inoutNumCountersUsed;

        uint32_t recomputedMinNumberOfCounters = kPixelBinnerLimitsMinCounters;

        /** in case if inoutNumBinsUsed is non-null pointer, the minimal number of counters is equal to the number of bins
         *  It is so, because in the worst case only one counter per bin is required for classification process as in basic counting sort */
        if (inoutNumBinsUsed)
        {
            recomputedMinNumberOfCounters = maxU32(recomputedMinNumberOfCounters, *inoutNumBinsUsed);
        }

        userNumCounters = maxU32(recomputedMinNumberOfCounters, userNumCounters);
        userNumCounters = minU32(kPixelBinnerLimitsMaxCounters, userNumCounters);

        *inoutNumCountersUsed = /*nextPow2*/(userNumCounters);
    }
}

void pixelBinnerInitConfig(PixelBinnerConfig *outConfig, uint16_t resX, uint16_t resY, uint16_t numBinsRequired, PixelBinnerPackingType packing)
{
    PIXBIN_ASSERT(resX < 4096);
    PIXBIN_ASSERT(resY < 4096);

    pixelBinnerApplyLimits(&numBinsRequired, 0);

    outConfig->m_resX = resX;
    outConfig->m_resY = resY;

    /** Default size of micro tile is hidden from the user because it may vary in future depending on
     *  sizes of compute kernels of counting & allocation passes, and number of bins */
    outConfig->m_log2MacroTileSizeX = outConfig->m_log2MicroTileSizeX = 6u;
    outConfig->m_log2MacroTileSizeY = outConfig->m_log2MicroTileSizeY = numBinsRequired <= 1024u ? 5u : 6u;

    /** Compute number of micro tiles covering the image and set initial number of macro tiles */
    outConfig->m_numMacroTilesX = outConfig->m_numMicroTilesX = (uint16_t)shiftRightAndRoundUp(resX, outConfig->m_log2MicroTileSizeX);
    outConfig->m_numMacroTilesY = outConfig->m_numMicroTilesY = (uint16_t)shiftRightAndRoundUp(resY, outConfig->m_log2MicroTileSizeY);

    outConfig->m_numBinsRequired = numBinsRequired;
    outConfig->m_numU32CountersReserved = 0;

    /** Sets m_numU32CountersRequired internally */
    updateRequiredCounters(outConfig, computeCountersFromMacroTiles(outConfig));

    outConfig->m_packingType = (uint8_t)packing;

    /** disable overriding pso index for counting and allocation passes. */
    outConfig->m_overridePSOIndex = 0;
    outConfig->m_countingPSOIndex = 2;
}

void pixelBinnerConstrainNumCounters(PixelBinnerConfig *inoutConfig, uint32_t numU32CountersReserved)
{
    uint32_t numU32CountersReservedWithLimit = numU32CountersReserved;
    pixelBinnerApplyLimits(&inoutConfig->m_numBinsRequired, &numU32CountersReservedWithLimit);
    PIXBIN_ASSERT(numU32CountersReservedWithLimit == numU32CountersReserved);

    uint8_t log2MacroTileSizeX = inoutConfig->m_log2MicroTileSizeX;
    uint8_t log2MacroTileSizeY = inoutConfig->m_log2MicroTileSizeY;

    /** Set initial value of the number of macro tiles to maximal possible */
    uint32_t numMacroTilesX = inoutConfig->m_numMicroTilesX;
    uint32_t numMacroTilesY = inoutConfig->m_numMicroTilesY;

    uint32_t numU32CountersRequired = (numMacroTilesX * numMacroTilesY * inoutConfig->m_numBinsRequired);

    /** Estimate the size of macro tile limited by the maximal number of allowed macro tiles,
     *  which is determined by the number of 4-byte counters available for all macro tiles. */
    while (numU32CountersRequired > numU32CountersReserved)
    {
        const uint32_t numMacroTilesXNext = shiftRightAndRoundUp(inoutConfig->m_resX, log2MacroTileSizeX + 1u);
        const uint32_t numMacroTilesYNext = shiftRightAndRoundUp(inoutConfig->m_resY, log2MacroTileSizeY + 1u);
#if 1
        // prioritize path which gives minimal total number of micro tiles
        if (numMacroTilesXNext * numMacroTilesY <= numMacroTilesYNext * numMacroTilesX)
#else
        // prioritize growth in X direction first, but keep ~squared shape
        if (log2MacroTileSizeX <= log2MacroTileSizeY)
#endif
        {
            numMacroTilesX = numMacroTilesXNext;
            log2MacroTileSizeX += 1;
        }
        else
        {
            numMacroTilesY = numMacroTilesYNext;
            log2MacroTileSizeY += 1;
        }
        numU32CountersRequired = (numMacroTilesX * numMacroTilesY * inoutConfig->m_numBinsRequired);
    }
    inoutConfig->m_log2MacroTileSizeX = log2MacroTileSizeX;
    inoutConfig->m_log2MacroTileSizeY = log2MacroTileSizeY;

    inoutConfig->m_numMacroTilesX = (uint16_t)numMacroTilesX;
    inoutConfig->m_numMacroTilesY = (uint16_t)numMacroTilesY;

    inoutConfig->m_numU32CountersReserved = numU32CountersReserved;
    updateRequiredCounters(inoutConfig, numU32CountersRequired);
    //assert(m_numU32CountersRequired < m_numU32CountersReserved);
}

void pixelBinnerConstrainMacroTileSize(PixelBinnerConfig *inoutConfig, uint32_t numMicroTilesPerMacroTileXLog2, uint32_t numMicroTilesPerMacroTileYLog2)
{
    PIXBIN_ASSERT(numMicroTilesPerMacroTileXLog2 + inoutConfig->m_log2MicroTileSizeX < 12u);
    PIXBIN_ASSERT(numMicroTilesPerMacroTileYLog2 + inoutConfig->m_log2MicroTileSizeY < 12u);

    numMicroTilesPerMacroTileXLog2 = minU32(12u - inoutConfig->m_log2MicroTileSizeX, numMicroTilesPerMacroTileXLog2);
    numMicroTilesPerMacroTileYLog2 = minU32(12u - inoutConfig->m_log2MicroTileSizeY, numMicroTilesPerMacroTileYLog2);

    inoutConfig->m_log2MacroTileSizeX = (uint8_t)(inoutConfig->m_log2MicroTileSizeX + numMicroTilesPerMacroTileXLog2);
    inoutConfig->m_log2MacroTileSizeY = (uint8_t)(inoutConfig->m_log2MicroTileSizeY + numMicroTilesPerMacroTileYLog2);

    inoutConfig->m_numMacroTilesX = (uint16_t)shiftRightAndRoundUp(inoutConfig->m_resX, inoutConfig->m_log2MacroTileSizeX);
    inoutConfig->m_numMacroTilesY = (uint16_t)shiftRightAndRoundUp(inoutConfig->m_resY, inoutConfig->m_log2MacroTileSizeY);

    updateRequiredCounters(inoutConfig, computeCountersFromMacroTiles(inoutConfig));
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

static void initCommonUavTex2dDesc(D3D12_RESOURCE_DESC *outDesc, uint16_t w, uint16_t h)
{
    *outDesc = {
        /*.Dimension          = */ D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        /*.Alignment          = */ 0,
        /*.Width              = */ w,
        /*.Height             = */ h,
        /*.DepthOrArraySize   = */ 1,
        /*.MipLevels          = */ 1,
        /*.Format             = */ DXGI_FORMAT_UNKNOWN,
        /*.SampleDesc.Count   = */ 1,
        /*.SampleDesc.Quality = */ 0,
        /*.Layout             = */ D3D12_TEXTURE_LAYOUT_UNKNOWN,
        /*.Flags              = */ D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    };
}
typedef enum PixelBinnerTex2dFormat
{
    kPixelBinnerTex2dFormat_8bit,
    kPixelBinnerTex2dFormat_16bit,
    kPixelBinnerTex2dFormat_32bit,
    kPixelBinnerTex2dFormat_Count
} PixelBinnerTex2dFormat;

static DXGI_FORMAT GTex2dUintResFormat[] =
{
    DXGI_FORMAT_R8_TYPELESS,
    DXGI_FORMAT_R16_TYPELESS,
    DXGI_FORMAT_R32_TYPELESS
};

static DXGI_FORMAT GTex2dUintUavFormat[] =
{
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R32_UINT
};

#ifdef _GAMING_XBOX_SCARLETT
static D3D12XBOX_IMAGE_FORMAT GTex2dUintSrvImageFormat[] =
{
    D3D12XBOX_IMAGE_FORMAT_8_UINT,
    D3D12XBOX_IMAGE_FORMAT_16_UINT,
    D3D12XBOX_IMAGE_FORMAT_32_UINT
};
#else
static D3D12XBOX_DATA_FORMAT GTex2dUintSrvDataFormat[] =
{
    D3D12XBOX_DATA_FORMAT_8,
    D3D12XBOX_DATA_FORMAT_16,
    D3D12XBOX_DATA_FORMAT_32
};
#endif

static inline void createCommittedRes(PixelBinnerResources *outRes, uint32_t mask, PixelBinnerResId resId, struct ID3D12Device *device, D3D12_RESOURCE_DESC const *resDesc, D3D12_HEAP_PROPERTIES const *heapProps)
{
    if (mask & (1u << resId))
    {
        ID3D12Resource *res = createUavRes(device, heapProps, resDesc);
        outRes->m_d3dPtrs[resId] = res;
        outRes->m_gpuAddr[resId] = res->GetGPUVirtualAddress();
        outRes->m_memoryInfos[resId] = queryResMemReq(device, resDesc);
    }
}

static inline void queryMemoryRes(PixelBinnerResources *outRes, uint32_t mask, PixelBinnerResId resId, struct ID3D12Device *device, D3D12_RESOURCE_DESC const *resDesc)
{
    if (mask & (1u << resId))
    {
        outRes->m_memoryInfos[resId] = queryResMemReq(device, resDesc);
    }
}

static inline void resByteBufferSetNumElements(D3D12_RESOURCE_DESC *resDesc, uint32_t numElements)
{
    resDesc->Width = numElements * sizeof(uint32_t);
}

typedef struct PixelBinnerResourceInfo
{
    uint32_t numU32PrefixL1;
    uint32_t numU32PrefixL2;
    uint32_t numU32BinnedPixels;
    uint32_t numU32PixelOffsets;

    PixelBinnerTex2dFormat binIdTexFmt;
    PixelBinnerTex2dFormat checkTexFmt;
} PixelBinnerResourceInfo;

static inline void initResourceInfo(PixelBinnerResourceInfo *outInfo, PixelBinnerConfig const *config)
{
    outInfo->numU32PrefixL1     = config->m_numU32CountersRequired;
    outInfo->numU32PrefixL2     = 1u << config->m_log2PrefixSizeL2;
    outInfo->numU32BinnedPixels = (uint32_t)config->m_resX * config->m_resY;
    outInfo->numU32PixelOffsets = 5 * kPixelBinnerLimitsMaxBins;

    outInfo->binIdTexFmt = config->m_numBinsRequired > 256 ? kPixelBinnerTex2dFormat_16bit : kPixelBinnerTex2dFormat_8bit;
    outInfo->checkTexFmt = kPixelBinnerTex2dFormat_32bit;
}

/** Helper macro list of resource initialization commands. In-place macro definition is required per command */
#define PixelBinnerResourceCreationOpSequence(info)         \
    ByteBufferInitOp(D3D12_RESOURCE_FLAG_NONE)              \
    ByteBufferSetNumElementsOp(info.numU32PrefixL1)         \
    ResourceCreationOp(kPixelBinnerResId_PrefixLevel1)      \
    ByteBufferSetNumElementsOp(info.numU32PrefixL2)         \
    ResourceCreationOp(kPixelBinnerResId_PrefixLevel2)      \
    ByteBufferSetNumElementsOp(info.numU32BinnedPixels)     \
    ResourceCreationOp(kPixelBinnerResId_BinnedPixels)      \
    ByteBufferInitOp(D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER) \
    ByteBufferSetNumElementsOp(info.numU32PixelOffsets)     \
    ResourceCreationOp(kPixelBinnerResId_PixelOffsets)      \
    UintTex2dInitOp()                                       \
    UintTex2dSetFormatOp(info.binIdTexFmt)                  \
    ResourceCreationOp(kPixelBinnerResId_BinIdTex2d)        \
    UintTex2dSetFormatOp(info.checkTexFmt)                  \
    ResourceCreationOp(kPixelBinnerResId_CheckTex2d)        \

void pixelBinnerQueryMemoryRequirements(PixelBinnerResources *outRes, uint32_t mask, struct ID3D12Device *device, PixelBinnerConfig const *config)
{
    PixelBinnerResourceInfo info;
    initResourceInfo(&info, config);

    D3D12_RESOURCE_DESC resDesc;

#define ResourceCreationOp(id)                  queryMemoryRes(outRes, mask, id, device, &resDesc);
#define ByteBufferInitOp(flags)                 initCommonUavBufferDesc(&resDesc, flags);
#define UintTex2dInitOp()                       initCommonUavTex2dDesc(&resDesc, config->m_resX, config->m_resY);
#define ByteBufferSetNumElementsOp(numElems)    resByteBufferSetNumElements(&resDesc, numElems);
#define UintTex2dSetFormatOp(fmt)               resDesc.Format = GTex2dUintResFormat[fmt];

    PixelBinnerResourceCreationOpSequence(info)

#undef UintTex2dSetFormatOp
#undef ByteBufferSetNumElementsOp
#undef UintTex2dInitOp
#undef ByteBufferInitOp
#undef ResourceCreationOp
}

void pixelBinnerCreateCommittedResources(PixelBinnerResources *outRes, uint32_t mask, struct ID3D12Device *device, PixelBinnerConfig const *config)
{
    PixelBinnerResourceInfo info;
    initResourceInfo(&info, config);

    D3D12_HEAP_PROPERTIES heapProps = {
        /*.Type                 =*/ D3D12_HEAP_TYPE_DEFAULT,
        /*.CPUPageProperty      =*/ D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        /*.MemoryPoolPreference =*/ D3D12_MEMORY_POOL_UNKNOWN,
        /*.CreationNodeMask     =*/ 0x1,
        /*.VisibleNodeMask      =*/ 0x1
    };

    D3D12_RESOURCE_DESC resDesc;
#define ResourceCreationOp(id)                  createCommittedRes(outRes, mask, id, device, &resDesc, &heapProps);
#define ByteBufferInitOp(flags)                 initCommonUavBufferDesc(&resDesc, flags);
#define UintTex2dInitOp()                       initCommonUavTex2dDesc(&resDesc, config->m_resX, config->m_resY);
#define ByteBufferSetNumElementsOp(numElems)    resByteBufferSetNumElements(&resDesc, numElems);
#define UintTex2dSetFormatOp(fmt)               resDesc.Format = GTex2dUintResFormat[fmt];

    PixelBinnerResourceCreationOpSequence(info)

#undef UintTex2dSetFormatOp
#undef ByteBufferSetNumElementsOp
#undef UintTex2dInitOp
#undef ByteBufferInitOp
#undef ResourceCreationOp
}

static inline void createRawUavDesc(PixelBinnerResources const *outResStore, uint32_t mask, PixelBinnerResId resId, struct ID3D12Device *device, D3D12_RESOURCE_DESC const *resDesc, D3D12XBOX_UNORDERED_ACCESS_VIEW_DESC *uavDesc)
{
    if ((mask & (1u << resId)) && (outResStore->m_cpuUavDescs[resId] != 0))
    {
        uavDesc->ResourceLocation = outResStore->m_gpuAddr[resId];
        device->CreatePlacedRawUnorderedAccessViewX(resDesc, uavDesc, { outResStore->m_cpuUavDescs[resId] });
    }
}

static inline void uavByteBufferInit(D3D12_RESOURCE_DESC *resDesc, D3D12XBOX_UNORDERED_ACCESS_VIEW_DESC *uavDesc, D3D12_RESOURCE_FLAGS AdditionalFlags)
{
    initCommonUavBufferDesc(resDesc, AdditionalFlags);
    uavDesc->Format                      = DXGI_FORMAT_R32_UINT;
    uavDesc->ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc->Buffer.FirstElement         = 0;
    uavDesc->Buffer.StructureByteStride  = 0;
    uavDesc->Buffer.CounterOffsetInBytes = 0;
    uavDesc->Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_RAW;
    uavDesc->CounterResourceLocation     = 0;
    uavDesc->MemoryType                  = 0;
}

static inline void uavUintTex2dInit(D3D12_RESOURCE_DESC *resDesc, D3D12XBOX_UNORDERED_ACCESS_VIEW_DESC *uavDesc, uint16_t w, uint16_t h)
{
    initCommonUavTex2dDesc(resDesc, w, h);
    uavDesc->Format                     = DXGI_FORMAT_UNKNOWN;
    uavDesc->ViewDimension              = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc->Texture2D.MipSlice         = 0;
    uavDesc->Texture2D.PlaneSlice       = 0;
    uavDesc->CounterResourceLocation    = 0;
    uavDesc->MemoryType                 = 0;
}

static inline void uavByteBufferSetNumElements(D3D12_RESOURCE_DESC *resDesc, D3D12XBOX_UNORDERED_ACCESS_VIEW_DESC *uavDesc, uint32_t numElements)
{
    uavDesc->Buffer.NumElements = numElements;
    resDesc->Width = numElements * sizeof(uint32_t);
}

static inline void uavUintTex2dSetFormat(D3D12_RESOURCE_DESC *resDesc, D3D12XBOX_UNORDERED_ACCESS_VIEW_DESC *uavDesc, PixelBinnerTex2dFormat fmt)
{
    resDesc->Format = GTex2dUintResFormat[fmt];
    uavDesc->Format = GTex2dUintUavFormat[fmt];
}

void pixelBinnerCreateUavDescriptors(PixelBinnerResources const *outResStore, uint32_t mask, struct ID3D12Device *device, PixelBinnerConfig const *config)
{
    PixelBinnerResourceInfo info;
    initResourceInfo(&info, config);

    D3D12_RESOURCE_DESC resDesc;
    D3D12XBOX_UNORDERED_ACCESS_VIEW_DESC uavDesc;

#define ResourceCreationOp(id)                  createRawUavDesc(outResStore, mask, id, device, &resDesc, &uavDesc);
#define ByteBufferInitOp(flags)                 uavByteBufferInit(&resDesc, &uavDesc, flags);
#define UintTex2dInitOp()                       uavUintTex2dInit(&resDesc, &uavDesc, config->m_resX, config->m_resY);
#define ByteBufferSetNumElementsOp(numElems)    uavByteBufferSetNumElements(&resDesc, &uavDesc, numElems);
#define UintTex2dSetFormatOp(fmt)               uavUintTex2dSetFormat(&resDesc, &uavDesc, fmt);

    PixelBinnerResourceCreationOpSequence(info)

#undef UintTex2dSetFormatOp
#undef ByteBufferSetNumElementsOp
#undef UintTex2dInitOp
#undef ByteBufferInitOp
#undef ResourceCreationOp
}

static inline void createRawSrvDesc(PixelBinnerResources const *outResStore, uint32_t mask, PixelBinnerResId resId, struct ID3D12Device *device, D3D12_RESOURCE_DESC const *resDesc, D3D12XBOX_SHADER_RESOURCE_VIEW_DESC *srvDesc)
{
    if ((mask & (1u << resId)) && (outResStore->m_cpuSrvDescs[resId] != 0))
    {
        srvDesc->ResourceLocation = outResStore->m_gpuAddr[resId];
        device->CreatePlacedRawShaderResourceViewX(resDesc, srvDesc, { outResStore->m_cpuSrvDescs[resId] });
    }
}

static inline void srvByteBufferInit(D3D12_RESOURCE_DESC *resDesc, D3D12XBOX_SHADER_RESOURCE_VIEW_DESC *srvDesc, D3D12_RESOURCE_FLAGS AdditionalFlags)
{
    initCommonUavBufferDesc(resDesc, AdditionalFlags);
    srvDesc->Format                      = DXGI_FORMAT_R32_UINT;
    srvDesc->ViewDimension               = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc->Shader4ComponentMapping     = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc->Buffer.FirstElement         = 0;
    srvDesc->Buffer.StructureByteStride  = 0;
    srvDesc->Buffer.Flags                = D3D12_BUFFER_SRV_FLAG_RAW;
#ifndef _GAMING_XBOX_SCARLETT
    srvDesc->NumberFormat                = D3D12XBOX_NUMBER_FORMAT_UINT;
    srvDesc->DataFormat                  = D3D12XBOX_DATA_FORMAT_32;
#else
    srvDesc->ImageFormat                 = D3D12XBOX_IMAGE_FORMAT_32_UINT;
#endif
    srvDesc->MemoryType                  = 0;
    srvDesc->TextureWarnLevelOfDetail    = 0;
    srvDesc->TexturePerfModulation       = 0;
}

static inline void srvUintTex2dInit(D3D12_RESOURCE_DESC *resDesc, D3D12XBOX_SHADER_RESOURCE_VIEW_DESC *srvDesc, uint16_t w, uint16_t h)
{
    initCommonUavTex2dDesc(resDesc, w, h);
    srvDesc->Format                         = DXGI_FORMAT_UNKNOWN;
    srvDesc->Shader4ComponentMapping        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc->ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc->Texture2D.MostDetailedMip      = 0;
    srvDesc->Texture2D.MipLevels            = 1;
    srvDesc->Texture2D.PlaneSlice           = 0;
    srvDesc->Texture2D.ResourceMinLODClamp  = 0.0;
#ifndef _GAMING_XBOX_SCARLETT
    srvDesc->NumberFormat                   = D3D12XBOX_NUMBER_FORMAT_UINT;
    srvDesc->DataFormat                     = D3D12XBOX_DATA_FORMAT_INVALID;
#else
    srvDesc->ImageFormat                    = D3D12XBOX_IMAGE_FORMAT_INVALID;
#endif
    srvDesc->MemoryType                     = 0;
    srvDesc->TextureWarnLevelOfDetail       = 0;
    srvDesc->TexturePerfModulation          = 0;
}

static inline void srvByteBufferSetNumElements(D3D12_RESOURCE_DESC *resDesc, D3D12XBOX_SHADER_RESOURCE_VIEW_DESC *srvDesc, uint32_t numElements)
{
    srvDesc->Buffer.NumElements = numElements;
    resDesc->Width = numElements * sizeof(uint32_t);
}

static inline void srvUintTex2dSetFormat(D3D12_RESOURCE_DESC *resDesc, D3D12XBOX_SHADER_RESOURCE_VIEW_DESC *srvDesc, PixelBinnerTex2dFormat fmt)
{
    resDesc->Format = GTex2dUintResFormat[fmt];
    srvDesc->Format = GTex2dUintUavFormat[fmt];
#ifdef _GAMING_XBOX_SCARLETT
    srvDesc->ImageFormat = GTex2dUintSrvImageFormat[fmt];
#else
    srvDesc->DataFormat = GTex2dUintSrvDataFormat[fmt];
#endif
}

void pixelBinnerCreateSrvDescriptors(PixelBinnerResources const *outResStore, uint32_t mask, struct ID3D12Device *device, PixelBinnerConfig const *config)
{
    PixelBinnerResourceInfo info;
    initResourceInfo(&info, config);

    D3D12_RESOURCE_DESC resDesc;
    D3D12XBOX_SHADER_RESOURCE_VIEW_DESC srvDesc;

#define ResourceCreationOp(id)                  createRawSrvDesc(outResStore, mask, id, device, &resDesc, &srvDesc);
#define ByteBufferInitOp(flags)                 srvByteBufferInit(&resDesc, &srvDesc, flags);
#define UintTex2dInitOp()                       srvUintTex2dInit(&resDesc, &srvDesc, config->m_resX, config->m_resY);
#define ByteBufferSetNumElementsOp(numElems)    srvByteBufferSetNumElements(&resDesc, &srvDesc, numElems);
#define UintTex2dSetFormatOp(fmt)               srvUintTex2dSetFormat(&resDesc, &srvDesc, fmt);

    PixelBinnerResourceCreationOpSequence(info)

#undef UintTex2dSetFormatOp
#undef ByteBufferSetNumElementsOp
#undef UintTex2dInitOp
#undef ByteBufferInitOp
#undef ResourceCreationOp
}

void pixelBinnerSubmit(ID3D12GraphicsCommandList *cmdList, PixelBinnerConfig const *pbConfig, PixelBinnerResources const *passData)
{
    PixelBinnerCountAndAllocCbData cbData;
    prepareConstantBuffer(&cbData, pbConfig);

    PixelBinnerRsAndPsoData rsAndPsoData;
    prepareRootSignaturesAndPipelineStates(&rsAndPsoData, pbConfig);

    D3D12XBOX_FLUSH flushMask = D3D12XBOX_FLUSH_BOP_CS_PARTIAL | D3D12XBOX_FLUSH_BOP_TEXTURE_L1_INVALIDATE;

    const uint64_t *gpuAddr = passData->m_gpuAddr;
    {
        uint32_t maxUintsHandledL1 = pbConfig->m_numU32CountersRequired;
        uint32_t maxUintsHandledL2 = 1u << pbConfig->m_log2PrefixSizeL2;

        cmdList->FillMemoryWith32BitValueX(gpuAddr[kPixelBinnerResId_PrefixLevel1], (UINT64)maxUintsHandledL1 * 4, 0, D3D12XBOX_COPY_FLAG_NONE);

        /** NOTE: Clear only if second level prefix storage is used */
        if (pbConfig->m_dispatchSizeL1 != 1u)
            cmdList->FillMemoryWith32BitValueX(gpuAddr[kPixelBinnerResId_PrefixLevel2], (UINT64)maxUintsHandledL2 * 4, 0, D3D12XBOX_COPY_FLAG_NONE);
    }
    cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
    {
        cmdList->SetComputeRootSignature(rsAndPsoData.m_rsCountAndAlloc);
        cmdList->SetPipelineState(rsAndPsoData.m_psoCount);

        cmdList->SetComputeRootDescriptorTable(0, { passData->m_gpuSrvDescs[kPixelBinnerResId_BinIdTex2d] } );
        cmdList->SetComputeRootUnorderedAccessView(1, gpuAddr[kPixelBinnerResId_PrefixLevel1]);
        cmdList->SetComputeRoot32BitConstants(5, sizeof(cbData) / sizeof(UINT), (void*)&cbData, 0);

        cmdList->Dispatch(pbConfig->m_numMicroTilesX, pbConfig->m_numMicroTilesY, 1);
    }
    cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
    {
        cmdList->SetComputeRootSignature(rsAndPsoData.m_rsPrefixSumL1L2);
        if (pbConfig->m_dispatchSizeL1 == 1u)
        {
            cmdList->SetPipelineState(rsAndPsoData.m_psoPrefixSumL1);
            cmdList->SetComputeRootUnorderedAccessView(0, gpuAddr[kPixelBinnerResId_PrefixLevel1]);
            cmdList->SetComputeRootUnorderedAccessView(1, 0);

            // Don't output per threadgroup sum
            cmdList->SetComputeRoot32BitConstant(2, 0x0, 0);
            cmdList->Dispatch(1, 1, 1);
        }
        else
        {
            cmdList->SetPipelineState(rsAndPsoData.m_psoPrefixSumL1);
            cmdList->SetComputeRootUnorderedAccessView(0, gpuAddr[kPixelBinnerResId_PrefixLevel1]);
            cmdList->SetComputeRootUnorderedAccessView(1, gpuAddr[kPixelBinnerResId_PrefixLevel2]);

            // Output per threadgroup sum
            cmdList->SetComputeRoot32BitConstant(2, 0x1, 0);
            cmdList->Dispatch(pbConfig->m_dispatchSizeL1, 1, 1);

            cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);

            if (rsAndPsoData.m_psoPrefixSumL1 != rsAndPsoData.m_psoPrefixSumL2)
                cmdList->SetPipelineState(rsAndPsoData.m_psoPrefixSumL2);

            cmdList->SetComputeRootUnorderedAccessView(0, gpuAddr[kPixelBinnerResId_PrefixLevel2]);
            cmdList->SetComputeRootUnorderedAccessView(1, 0);

            // Don't output per threadgroup sum
            cmdList->SetComputeRoot32BitConstant(2, 0x0, 0);
            cmdList->Dispatch(1, 1, 1);
        }
    }
    cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
    {
        cmdList->SetComputeRootSignature(rsAndPsoData.m_rsCountAndAlloc);
        cmdList->SetPipelineState(rsAndPsoData.m_psoAlloc);

        cmdList->SetComputeRootDescriptorTable(0, { passData->m_gpuSrvDescs[kPixelBinnerResId_BinIdTex2d] });
        cmdList->SetComputeRootUnorderedAccessView(1, gpuAddr[kPixelBinnerResId_PrefixLevel1]);
        cmdList->SetComputeRootUnorderedAccessView(2, gpuAddr[kPixelBinnerResId_PrefixLevel2]);
        cmdList->SetComputeRootUnorderedAccessView(3, gpuAddr[kPixelBinnerResId_BinnedPixels]);
        cmdList->SetComputeRoot32BitConstants(5, sizeof(cbData) / sizeof(UINT), (void*)&cbData, 0);

        cmdList->Dispatch(pbConfig->m_numMicroTilesX, pbConfig->m_numMicroTilesY, 1);
    }
    cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);

    if (pbConfig->m_packingType == kPixelBinnerPackingType_Global)
    {
        cmdList->SetComputeRootSignature(GPixelBinnerPsoAndRs.m_rsWriteExecuteIndirectArgs);
        cmdList->SetPipelineState(GPixelBinnerPsoAndRs.m_psoWriteExecuteIndirectArgs);

        cmdList->SetComputeRootShaderResourceView(0, gpuAddr[kPixelBinnerResId_PrefixLevel1]);
        cmdList->SetComputeRootShaderResourceView(1, gpuAddr[kPixelBinnerResId_PrefixLevel2]);
        cmdList->SetComputeRootUnorderedAccessView(2, gpuAddr[kPixelBinnerResId_PixelOffsets]);

        uint32_t numMacroTiles = (uint32_t)(pbConfig->m_numMacroTilesX * pbConfig->m_numMacroTilesY);
        uint32_t log2PrefixSizeL1 = pbConfig->m_log2PrefixSizeL1;
        uint32_t numBins = pbConfig->m_numBinsRequired;
        uint32_t log2ExecuteIndirectSize = 6;
        cmdList->SetComputeRoot32BitConstant(4, numMacroTiles, 0);
        cmdList->SetComputeRoot32BitConstant(4, log2PrefixSizeL1, 1);
        cmdList->SetComputeRoot32BitConstant(4, numBins, 2);
        cmdList->SetComputeRoot32BitConstant(4, log2ExecuteIndirectSize, 3);

        cmdList->Dispatch(1, 1, 1);
        cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
    }
}

void pixelBinnerSubmitDbgVisPass(ID3D12GraphicsCommandList *cmdList, PixelBinnerConfig const *config, PixelBinnerResources const *passData, PixelBinnerDbgVisPassId passId)
{
    if (passId == kPixelBinnerDbgVisPassId_Divergency)
    {
        cmdList->SetComputeRootSignature(GPixelBinnerPsoAndRs.m_rsDebugVisBinIdDivergence);

        if (config->m_numBinsRequired > 1024)
            cmdList->SetPipelineState(GPixelBinnerPsoAndRs.m_psoDebugVis_BinIdDivergence2048);
        else
            cmdList->SetPipelineState(GPixelBinnerPsoAndRs.m_psoDebugVis_BinIdDivergence1024);

        cmdList->SetComputeRootDescriptorTable(0, { passData->m_gpuSrvDescs[kPixelBinnerResId_BinIdTex2d] });
        cmdList->SetComputeRootDescriptorTable(1, { passData->m_gpuUavDescs[kPixelBinnerResId_OutputTex2d] });

        PixelBinnerCountAndAllocCbData cbData;
        prepareConstantBuffer(&cbData, config);
        cmdList->SetComputeRoot32BitConstants(2, sizeof(cbData) / sizeof(UINT), (void *)&cbData, 0);

        cmdList->Dispatch(config->m_numMicroTilesX, config->m_numMicroTilesY, 1);
    }
    else if (passId == kPixelBinnerDbgVisPassId_PixelOrder)
    {
        cmdList->SetComputeRootSignature(GPixelBinnerPsoAndRs.m_rsDebugVisBinIdToColor);
        cmdList->SetPipelineState(GPixelBinnerPsoAndRs.m_psoDebugVis_BinIdToColor);

        cmdList->SetComputeRootDescriptorTable(0, { passData->m_gpuSrvDescs[kPixelBinnerResId_BinIdTex2d] });
        cmdList->SetComputeRootShaderResourceView(1, passData->m_gpuAddr[kPixelBinnerResId_BinnedPixels] );
        cmdList->SetComputeRootDescriptorTable(2, { passData->m_gpuUavDescs[kPixelBinnerResId_OutputTex2d] });

        const uint32_t numBinnedPixels = (uint32_t)config->m_resX * config->m_resY;

        uint32_t log2MacroTileSizeX = config->m_log2MacroTileSizeX;
        uint32_t log2MacroTileSizeY = config->m_log2MacroTileSizeY;
        if (config->m_packingType == kPixelBinnerPackingType_Global)
        {
            log2MacroTileSizeX = nextPow2(config->m_resX);
            log2MacroTileSizeY = nextPow2(config->m_resY);
        }

        cmdList->SetComputeRoot32BitConstant(3, config->m_numBinsRequired, 0);
        cmdList->SetComputeRoot32BitConstant(3, numBinnedPixels, 1);
        cmdList->SetComputeRoot32BitConstant(3, config->m_resX, 2);
        cmdList->SetComputeRoot32BitConstant(3, config->m_resY, 3);
        cmdList->SetComputeRoot32BitConstant(3, log2MacroTileSizeX, 4);
        cmdList->SetComputeRoot32BitConstant(3, log2MacroTileSizeY, 5);

        cmdList->Dispatch((numBinnedPixels + 63) >> 6, 1u, 1u);
    }
}

void BarrierTransition(D3D12_RESOURCE_BARRIER* outDesc, ID3D12Resource *Res, D3D12_RESOURCE_STATES Prev, D3D12_RESOURCE_STATES Next)
{
    outDesc->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    outDesc->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    outDesc->Transition.pResource   = Res;
    outDesc->Transition.Subresource = 0;
    outDesc->Transition.StateBefore = Prev;
    outDesc->Transition.StateAfter  = Next;
}

void pixelBinnerSubmitCheckPass(ID3D12GraphicsCommandList *cmdList, PixelBinnerConfig const *config, PixelBinnerResources const *passData)
{
    const D3D12XBOX_FLUSH flushMask = D3D12XBOX_FLUSH_BOP_CS_PARTIAL | D3D12XBOX_FLUSH_BOP_TEXTURE_L1_INVALIDATE;
    {
        UINT ClearValues[] =
        {
            0x11111111, 0x11111111, 0x11111111, 0x11111111
        };

        cmdList->FillMemoryWith128BitValueX(passData->m_gpuAddr[kPixelBinnerResId_CheckTex2d], passData->m_memoryInfos[kPixelBinnerResId_CheckTex2d].m_size, ClearValues, D3D12XBOX_COPY_FLAG_NONE);
    }
    cmdList->FlushPipelineX(flushMask | D3D12XBOX_FLUSH_TOP_COMPUTE_MASK, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);

    if (config->m_packingType == kPixelBinnerPackingType_Global)
    {
        cmdList->SetComputeRootSignature(GPixelBinnerPsoAndRs.m_rsDebugVisTouchOnceTest);
        cmdList->SetPipelineState(GPixelBinnerPsoAndRs.m_psoDebugVis_TouchOnce);

        cmdList->SetComputeRootUnorderedAccessView(0, passData->m_gpuAddr[kPixelBinnerResId_BinnedPixels]);
        cmdList->SetComputeRootDescriptorTable(1, { passData->m_gpuUavDescs[kPixelBinnerResId_CheckTex2d] });
        cmdList->SetComputeRootDescriptorTable(2, { passData->m_gpuUavDescs[kPixelBinnerResId_OutputTex2d] });

        const uint32_t numBinnedPixels = (uint32_t)config->m_resX * config->m_resY;
        cmdList->SetComputeRoot32BitConstant(3, numBinnedPixels, 0);
        cmdList->SetComputeRoot32BitConstant(3, 0, 1);
        cmdList->SetComputeRoot32BitConstant(3, 0, 2);
        cmdList->SetComputeRoot32BitConstant(3, 0, 3);

        if (0) // set to 1 to avoid using indirect dispatch
        {
            cmdList->Dispatch((numBinnedPixels + 63) >> 6, 1u, 1u);
        }
        else
        {
            {
                D3D12_RESOURCE_BARRIER barrier;
                BarrierTransition
                (
                    &barrier,
                    passData->m_d3dPtrs[kPixelBinnerResId_PixelOffsets],
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
                );
                cmdList->ResourceBarrier(1, &barrier);
            }

            cmdList->ExecuteIndirect
            (
                GPixelBinnerPsoAndRs.m_csWriteExecuteIndirectArgs,
                config->m_numBinsRequired,
                passData->m_d3dPtrs[kPixelBinnerResId_PixelOffsets],
                0,
                0,
                0
            );

            {
                D3D12_RESOURCE_BARRIER barrier;
                BarrierTransition
                (
                    &barrier,
                    passData->m_d3dPtrs[kPixelBinnerResId_PixelOffsets],
                    D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                );
                cmdList->ResourceBarrier(1, &barrier);

            }
        }
        cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);

        cmdList->SetPipelineState(GPixelBinnerPsoAndRs.m_psoDebugVis_TouchOnceTest);
        cmdList->Dispatch
        (
            shiftRightAndRoundUp(config->m_resX, 3),
            shiftRightAndRoundUp(config->m_resY, 3),
            1
        );
        cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
    }
    else
    {
        const uint32_t numVerificationPasses = (uint32_t)config->m_numMacroTilesX * config->m_numMacroTilesY;

        const uint32_t numPixelsPerMacroTileX = 1u << ((uint32_t)config->m_log2MacroTileSizeX);
        const uint32_t numPixelsPerMacroTileY = 1u << ((uint32_t)config->m_log2MacroTileSizeY);

        // Constraints based on vertification kernel size
        PIXBIN_ASSERT(numPixelsPerMacroTileX >= 8);
        PIXBIN_ASSERT(numPixelsPerMacroTileY >= 8);
        PIXBIN_ASSERT((numPixelsPerMacroTileX % 8) == 0);
        PIXBIN_ASSERT((numPixelsPerMacroTileY % 8) == 0);

        cmdList->SetComputeRootSignature(GPixelBinnerPsoAndRs.m_rsDebugVisTouchOnceTest);
        cmdList->SetComputeRootUnorderedAccessView(0, passData->m_gpuAddr[kPixelBinnerResId_BinnedPixels]);
        cmdList->SetComputeRootDescriptorTable(1, { passData->m_gpuUavDescs[kPixelBinnerResId_CheckTex2d] });
        cmdList->SetComputeRootDescriptorTable(2, { passData->m_gpuUavDescs[kPixelBinnerResId_OutputTex2d] });

        uint32_t numPixelsTouchedSoFar = 0;

        for (uint32_t i = 0; i < numVerificationPasses; ++i)
        {
            const uint32_t macroTileX = i % config->m_numMacroTilesX;
            const uint32_t macroTileY = i / config->m_numMacroTilesX;

            const uint32_t macroTileOffsetInPixelsX = macroTileX << config->m_log2MacroTileSizeX;
            const uint32_t macroTileOffsetInPixelsY = macroTileY << config->m_log2MacroTileSizeY;

            const uint32_t numPixelsX = minU32(config->m_resX, macroTileOffsetInPixelsX + numPixelsPerMacroTileX) - macroTileOffsetInPixelsX;
            const uint32_t numPixelsY = minU32(config->m_resY, macroTileOffsetInPixelsY + numPixelsPerMacroTileY) - macroTileOffsetInPixelsY;

            const uint32_t numPixelsToDispatch = numPixelsX * numPixelsY;

            cmdList->SetComputeRoot32BitConstant(3, numPixelsToDispatch, 0);

            cmdList->SetComputeRoot32BitConstant(3, numPixelsTouchedSoFar, 1);
            cmdList->SetComputeRoot32BitConstant(3, macroTileOffsetInPixelsX, 2);
            cmdList->SetComputeRoot32BitConstant(3, macroTileOffsetInPixelsY, 3);
            {
                cmdList->SetPipelineState(GPixelBinnerPsoAndRs.m_psoDebugVis_TouchOnce);
                cmdList->Dispatch(shiftRightAndRoundUp(numPixelsToDispatch, 6), 1, 1);
            }
            cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
            {
                cmdList->SetPipelineState(GPixelBinnerPsoAndRs.m_psoDebugVis_TouchOnceTest);
                cmdList->Dispatch(shiftRightAndRoundUp(numPixelsPerMacroTileX, 3), shiftRightAndRoundUp(numPixelsPerMacroTileY, 3), 1);
            }
            cmdList->FlushPipelineX(flushMask, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);

            numPixelsTouchedSoFar += numPixelsToDispatch;
        }
    }
}
