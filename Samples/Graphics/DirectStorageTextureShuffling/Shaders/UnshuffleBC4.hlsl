//--------------------------------------------------------------------------------------
// UnshuffleBC4.hlsl
//
// DirectStorage Shuffle Formats: 
// - <https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/system/overviews/directstorage/directstorage-shuffles>
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

RWByteAddressBuffer inputOutput : register(u0);

#define BLOCK_SIZE 			32768
#define R0_OFFSET			0
#define R1_OFFSET			(BLOCK_SIZE / 8)
#define INDICES_OFFSET		(BLOCK_SIZE / 4)

#define THREADS_PER_WAVE 		64
#define DATA_LOADED_PER_THREAD 	(BLOCK_SIZE / THREADS_PER_WAVE)
#define R0_SIZE_PER_THREAD		(DATA_LOADED_PER_THREAD / 8)
#define R1_SIZE_PER_THREAD		R0_SIZE_PER_THREAD
#define INDICES_SIZE_PER_THREAD	(DATA_LOADED_PER_THREAD * 6) / 8

#define BYTES_PER_LOAD          16
#define BYTES_PER_STORE         16
#define BYTES_PER_INDICES_LOAD  12

#define NUM_SUB_BLOCKS          4
#define NUM_ENDPOINT_LOADS      (R0_SIZE_PER_THREAD / BYTES_PER_LOAD)
#define NUM_INDICES_LOADS       (INDICES_SIZE_PER_THREAD / BYTES_PER_INDICES_LOAD)
#define SIZE_OF_BC4_BLOCK       8
#define NUM_BC4_BLOCKS_PER_THREAD   (DATA_LOADED_PER_THREAD / SIZE_OF_BC4_BLOCK)
#define NUM_BC4_BLOCKS_PER_THREAD_PER_SUB_BLOCK   (NUM_BC4_BLOCKS_PER_THREAD / NUM_SUB_BLOCKS)

struct BC4_BLOCK
{
    uint64_t r0_r1_indices; // [0:7] = r0. [8:15] = r1. [16:63] = indices;
};

struct DoubleBC4Block
{
    BC4_BLOCK block0;
    BC4_BLOCK block1;
};

BC4_BLOCK FormBC4Block(uint r0, uint r1, uint16_t3 indices)
{
    BC4_BLOCK block;
    block.r0_r1_indices = uint64_t(r0) | (uint64_t(r1) << 8) | (uint64_t(indices.x) << 16) | (uint64_t(indices.y) << 32) | (uint64_t(indices.z) << 48);
    return block;
}

[numthreads(THREADS_PER_WAVE, 1, 1)]
[RootSignature("UAV(u0)")]
void main(uint groupID : SV_GroupID)
{
    uint threadIDInWave = WaveGetLaneIndex();
	
    uint blockAddress = groupID * BLOCK_SIZE;
    uint r0BaseAddress = blockAddress + R0_OFFSET;
    uint r1BaseAddress = blockAddress + R1_OFFSET;
    uint indicesBaseAddress = blockAddress + INDICES_OFFSET;
 	
    uint r0Arr[NUM_ENDPOINT_LOADS * 4];
    uint r1Arr[NUM_ENDPOINT_LOADS * 4];
    uint16_t indicesArr[NUM_INDICES_LOADS * 6];
    
    const uint STEP_SIZE = BYTES_PER_LOAD * THREADS_PER_WAVE;
    
    [unroll]
    for (int i = 0; i < NUM_ENDPOINT_LOADS; ++i)
    {
        uint4 r0 = inputOutput.Load<uint4>(r0BaseAddress + (threadIDInWave * BYTES_PER_LOAD));
        uint4 r1 = inputOutput.Load<uint4>(r1BaseAddress + (threadIDInWave * BYTES_PER_LOAD));
        
        r0Arr[i * 4 + 0] = r0.x;
        r0Arr[i * 4 + 1] = r0.y;
        r0Arr[i * 4 + 2] = r0.z;
        r0Arr[i * 4 + 3] = r0.w;
        
        r1Arr[i * 4 + 0] = r1.x;
        r1Arr[i * 4 + 1] = r1.y;
        r1Arr[i * 4 + 2] = r1.z;
        r1Arr[i * 4 + 3] = r1.w;
        
        r0BaseAddress += (STEP_SIZE);
        r1BaseAddress += (STEP_SIZE);
    }
    
    // Indices.
    uint index = 0;

    [unroll]
    for (int i = 0; i < NUM_ENDPOINT_LOADS; ++i)
    {
        // For each endpoint load, (16 bytes, 16 R0's) we need to load 6x as much endpoint index data.
        uint indPerThreadPerSubBlock = INDICES_SIZE_PER_THREAD / NUM_SUB_BLOCKS;
        uint addr = indicesBaseAddress + (threadIDInWave * indPerThreadPerSubBlock);
        
        for (int j = 0; j < indPerThreadPerSubBlock / BYTES_PER_INDICES_LOAD; j++)
        {
            uint3 indices = inputOutput.Load<uint3>(addr + (j * BYTES_PER_INDICES_LOAD));

            // Flatten the array from uint3 to uint16_t for easier indexing.
            indicesArr[index * 6 + 0] = (uint16_t)(indices.x & 0xFFFF);
            indicesArr[index * 6 + 1] = (uint16_t)(indices.x >> 16);
            indicesArr[index * 6 + 2] = (uint16_t)(indices.y & 0xFFFF);
            indicesArr[index * 6 + 3] = (uint16_t)(indices.y >> 16);
            indicesArr[index * 6 + 4] = (uint16_t)(indices.z & 0xFFFF);
            indicesArr[index * 6 + 5] = (uint16_t)(indices.z >> 16);

            index++;
        }
        
        indicesBaseAddress += (STEP_SIZE * 6);
    }
        
    uint writeAddress = blockAddress + (threadIDInWave * (SIZE_OF_BC4_BLOCK * (NUM_BC4_BLOCKS_PER_THREAD / NUM_SUB_BLOCKS))); // Each thread writes 64B (8 BC1 blocks).    
    static const uint numWritesPerSubBlock = NUM_BC4_BLOCKS_PER_THREAD_PER_SUB_BLOCK / 4; // We write 4 BC4 blocks at a time.
    
    // Run through the 64 BC4 blocks we need to write, writing them in pairs so we write 16 bytes at a time.
    // A 32KB block is handled by 64 threads, meaning each thread is responsible for 512 bytes of data (64 BC4 blocks).
    // Because of the way we're trying to read and write contiguous data, we loaded from 4 separate locations inside R0, R1 and Indices.
    // For each one of those 4 locations, we need to write 16 BC4 blocks worth of data (4 at a time).
    [unroll]
    for (int subBlock = 0; subBlock < NUM_SUB_BLOCKS; subBlock++)
    {
        [unroll]
        for (int i = 0; i < numWritesPerSubBlock; i++)
        {
            uint idx = subBlock * numWritesPerSubBlock + i;

            uint r0ABCD = r0Arr[idx];
            uint r1ABCD = r1Arr[idx];
            uint16_t3 indicesA = uint16_t3(indicesArr[idx * 12 + 0], indicesArr[idx * 12 + 1], indicesArr[idx * 12 + 2]);
            uint16_t3 indicesB = uint16_t3(indicesArr[idx * 12 + 3], indicesArr[idx * 12 + 4], indicesArr[idx * 12 + 5]);
            uint16_t3 indicesC = uint16_t3(indicesArr[idx * 12 + 6], indicesArr[idx * 12 + 7], indicesArr[idx * 12 + 8]);
            uint16_t3 indicesD = uint16_t3(indicesArr[idx * 12 + 9], indicesArr[idx * 12 + 10], indicesArr[idx * 12 + 11]);
        
            DoubleBC4Block blockAB, blockCD;

            blockAB.block0 = FormBC4Block((r0ABCD >> 0) & 0xFF, (r1ABCD >> 0) & 0xFF, indicesA);
            blockAB.block1 = FormBC4Block((r0ABCD >> 8) & 0xFF, (r1ABCD >> 8) & 0xFF, indicesB);
            blockCD.block0 = FormBC4Block((r0ABCD >> 16) & 0xFF, (r1ABCD >> 16) & 0xFF, indicesC);
            blockCD.block1 = FormBC4Block((r0ABCD >> 24) & 0xFF, (r1ABCD >> 24) & 0xFF, indicesD);

            inputOutput.Store<DoubleBC4Block>(writeAddress + (i * 32) + 0, blockAB);
            inputOutput.Store<DoubleBC4Block>(writeAddress + (i * 32) + 16, blockCD);
        }

        writeAddress += (BLOCK_SIZE / NUM_SUB_BLOCKS);
    }
}
