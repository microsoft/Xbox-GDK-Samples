//--------------------------------------------------------------------------------------
// DynamicLodAS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define __XBOX_ENABLE_WAVE32 1

#include "Common.hlsli"
#include "CommonMS.hlsli"

// Threadgroup's export payload data
groupshared Payload s_Payload;

// Working groupshared memory for accumulating counts & offsets (copied to payload)
groupshared uint s_InstanceCounts[MAX_LOD_LEVELS];
groupshared uint s_GroupOffsets[MAX_LOD_LEVELS + 1];
groupshared uint s_InstanceList[AS_GROUP_SIZE];
groupshared uint s_InstanceOffsets[MAX_LOD_LEVELS + 1];

//----------------------------------------------------------------------------------
// NOTE: This shader is only intended to be ran as a threadgroup with a single wave.
//       Thus AS_GROUP_SIZE is defined as the platform's wave size in Shared.h

[RootSignature(ROOT_SIG)]
[NumThreads(AS_GROUP_SIZE, 1, 1)]
void main(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    // Zero out groupshared memory which requires it.
    s_InstanceList[gtid] = 0;

    if (gtid == 0)
    {
        s_InstanceOffsets[0] = 0;
        s_GroupOffsets[0] = 0;
    }

    if (gtid < MAX_LOD_LEVELS)
    {
        s_InstanceCounts[gtid] = 0;
    }


    // Cull & compact the instances
    uint lodLevel = MAX_LOD_LEVELS; // LOD level of this thread's instance
    uint lodOffset = 0;             // Offset into its LOD-level instance list

    uint instanceIndex = DrawParams.InstanceOffset + dtid;
    if (instanceIndex < DrawParams.InstanceCount)
    {
        Instance instance = Instances[instanceIndex];

        if (IsVisible(instance.BoundingSphere))
        {
            lodLevel = ComputeLOD(instance.BoundingSphere);
        }
    }

    // Determine counts for each LOD and each thread's offset into its LOD instance list
    [unroll]
    for (uint i = 0; i < MAX_LOD_LEVELS; ++i)
    {
        bool lodMatch = lodLevel == i; // Whether this thread's calculated LOD matches the current one.

        if (lodMatch)
        {
            lodOffset = PrefixCountBits(lodMatch);
        }

        s_InstanceCounts[i] = ActiveCountBits(lodMatch);
    }

    // Compute instance and Level-of-detail
    if (gtid < MAX_LOD_LEVELS)
    {
        uint lodInstanceCount = s_InstanceCounts[gtid];

        s_InstanceOffsets[gtid + 1] = lodInstanceCount;

        uint unpackedCount = (MeshInfo[gtid].MeshletCount - 1) * lodInstanceCount;
        uint packedCount = (MeshInfo[gtid].LastMeshletSize * lodInstanceCount + MS_GROUP_SIZE - 1) / MS_GROUP_SIZE;

        s_GroupOffsets[gtid + 1] = unpackedCount + packedCount;
    }

    // Accumulate LOD & instance counts to create offset lookup tables for mesh shader threadgroups
#ifndef __XBOX_SCARLETT 
    if (gtid <= MAX_LOD_LEVELS)
    {
        uint instanceCount = s_InstanceOffsets[gtid];
        s_InstanceOffsets[gtid] = instanceCount + WavePrefixSum(instanceCount);

        uint lodCount = s_GroupOffsets[gtid];
        s_GroupOffsets[gtid] = lodCount + WavePrefixSum(lodCount);
    }
#else
    // WavePrefixSum() wave intrinsic currently unimplemented on Scarlett
    if (gtid == 0)
    {
        for (uint i = 2; i <= MAX_LOD_LEVELS; ++i)
        {
            uint instanceCount = s_InstanceOffsets[i];
            s_InstanceOffsets[i] = s_InstanceOffsets[i] + s_InstanceOffsets[i - 1];

            uint lodCount = s_GroupOffsets[i];
            s_GroupOffsets[i] = s_GroupOffsets[i] + s_GroupOffsets[i - 1];
        }
    }
#endif

    // Place this thread's instance index at its assigned slot from compaction.
    if (lodLevel != MAX_LOD_LEVELS)
    {
        uint lodStart = s_InstanceOffsets[lodLevel];

        s_InstanceList[lodStart + lodOffset] = instanceIndex;
    }

    // Store the instance counts for each LOD level into the stats UAV
    if (gtid < MAX_LOD_LEVELS)
    {
        uint globalGroupId = (DrawParams.InstanceOffset / AS_GROUP_SIZE) + gid;
        LodCounts.Store((globalGroupId * MAX_LOD_LEVELS + gtid) * 4, s_InstanceCounts[gtid]);
    }

    // Copy groupshared memory to payload
    s_Payload.InstanceList[gtid] = s_InstanceList[gtid];

    if (gtid < MAX_LOD_LEVELS)
    {
        s_Payload.InstanceCounts[gtid] = s_InstanceCounts[gtid];
    }

    if (gtid <= MAX_LOD_LEVELS)
    {
        s_Payload.GroupOffsets[gtid] = s_GroupOffsets[gtid];
        s_Payload.InstanceOffsets[gtid] = s_InstanceOffsets[gtid];
    }

    // NOTE: Be aware that the maximum threadgroup count of a single dimension is 65536.
    //       This method is limited to an LOD 0 size of (65536 / AS_GROUP_SIZE) meshlets.
    DispatchMesh(s_GroupOffsets[Constants.LODCount], 1, 1, s_Payload);
}
