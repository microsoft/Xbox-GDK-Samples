//--------------------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

typedef struct RadixConfig
{
    uint32_t    m_numElements;

    uint8_t     m_numSubkeyBits;

    uint8_t     m_log2KernelSize;

    uint16_t    m_numKernels;

    uint32_t    m_numU32CountersRequired;   /**< The number of 4-byte counters required to complete binning for a given configuration */

    uint8_t     m_log2PrefixSizeL1;         /**< log2 of the size of the kernel computing level1 prefix sum */
    uint8_t     m_log2PrefixSizeL2;         /**< log2 of the size of the kernel computing level2 prefix sum */

    uint16_t    m_dispatchSizeL1;           /**< The dispatch dimension of level1 prefix sum */

} RadixConfig;

void radixInit(struct ID3D12Device *device);
void radixTerm();

void radixInitConfig(RadixConfig *outConfig, uint32_t numElements);

typedef enum RadixResId
{
    kRadixResId_Subkeys,
    kRadixResId_Payload,
    kRadixResId_PrefixL1,
    kRadixResId_PrefixL2,
    kRadixResId_Count,

    kRadixResId_MaskAll = (1u << kRadixResId_Subkeys)
                        | (1u << kRadixResId_Payload)
                        | (1u << kRadixResId_PrefixL1)
                        | (1u << kRadixResId_PrefixL2)

} RadixResId;

typedef struct RadixResources
{
    ID3D12Resource *m_d3dPtrs[kRadixResId_Count];
    uint64_t        m_gpuAddr[kRadixResId_Count];
} RadixResources;

void radixCreateCommittedResources(RadixResources *outRes, uint32_t mask, struct ID3D12Device *device, RadixConfig const *config);

void radixSubmit(struct ID3D12GraphicsCommandList *cmdList, RadixConfig const *config, RadixResources *resources);
