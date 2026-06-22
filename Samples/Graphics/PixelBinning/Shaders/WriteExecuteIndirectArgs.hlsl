//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define NUM_THREADS (512)

ByteAddressBuffer PrefixL1 : register(t0);
ByteAddressBuffer PrefixL2 : register(t1);

RWByteAddressBuffer BinOffsets : register(u0);
RWByteAddressBuffer DispatchArgs : register(u1);

#define RootSig \
    "SRV(t0)," \
    "SRV(t1)," \
    "UAV(u0)," \
    "UAV(u1)," \
    "RootConstants(b0, num32bitconstants=4)"

cbuffer ShaderParams : register(b0)
{
    uint numMacroTiles;

    uint log2PrefixSizeL1;

    uint numBins;

    uint log2ExecIndirectKernel;
};

groupshared uint LdsStorage[NUM_THREADS * 4];

[RootSignature(RootSig)]
[numthreads(NUM_THREADS, 1, 1)]
void main(uint ThreadId : SV_GroupThreadId)
{
    for (uint binIndex = ThreadId; binIndex < numBins; binIndex += NUM_THREADS)
    {
        const uint prefixL1Index = numMacroTiles * (binIndex + 1) - 1;
        const uint prefixL2Index = prefixL1Index >> log2PrefixSizeL1;

        const uint L1Offset = PrefixL1.Load(prefixL1Index << 2);
        const uint L2Offset = PrefixL2.Load(prefixL2Index << 2);

        LdsStorage[binIndex] = L1Offset + L2Offset;
    }
    GroupMemoryBarrierWithGroupSync();

    for (uint binIndex = ThreadId; binIndex < numBins; binIndex += NUM_THREADS)
    {
        uint PrevBinInclusivePrefix = 0;
        [branch] if (binIndex > 0)
        {
            PrevBinInclusivePrefix = LdsStorage[binIndex - 1];
        }
        uint ThisBinInclusivePrefix = LdsStorage[binIndex];

        uint NumPixels = ThisBinInclusivePrefix - PrevBinInclusivePrefix;
        uint NumGroups = (NumPixels + (1u << log2ExecIndirectKernel) - 1) >> log2ExecIndirectKernel;

        BinOffsets.Store(binIndex * 20, NumPixels);
        BinOffsets.Store4(binIndex * 20 + 4, uint4(PrevBinInclusivePrefix, NumGroups, 1, 1));
    }
}
