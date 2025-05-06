//--------------------------------------------------------------------------------------
// UnshuffleBC3.hlsl
//
// DirectStorage Shuffle Formats: 
// - <https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/system/overviews/directstorage/directstorage-shuffles>
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

RWByteAddressBuffer inputOutput : register(u0);

#define BLOCK_SIZE 			32768
#define A0_OFFSET			0                       // A0 is at the start of the block - 2KB
#define A1_OFFSET			(BLOCK_SIZE / 16)       // A1 is at 2KB - 4KB
#define AIND_OFFSET			(BLOCK_SIZE / 8)        // Alpha indices are at 4KB - 16KB
#define C0_OFFSET			(BLOCK_SIZE / 2)        // C0 is at 16KB - 20KB
#define C1_OFFSET			(BLOCK_SIZE * 5) / 8    // C1 is at 20KB - 24KB
#define CIND_OFFSET		    (BLOCK_SIZE * 3) / 4    // Color indices are at 24KB - 32KB

#define THREADS_PER_WAVE 		64
#define DATA_LOADED_PER_THREAD 	(BLOCK_SIZE / THREADS_PER_WAVE)
#define A0_SIZE_PER_THREAD		(DATA_LOADED_PER_THREAD / 16)
#define A1_SIZE_PER_THREAD		A0_SIZE_PER_THREAD
#define AIND_SIZE_PER_THREAD	(DATA_LOADED_PER_THREAD * 6) / 16
#define C0_SIZE_PER_THREAD		(DATA_LOADED_PER_THREAD / 8)
#define C1_SIZE_PER_THREAD		C0_SIZE_PER_THREAD
#define CIND_SIZE_PER_THREAD	(DATA_LOADED_PER_THREAD / 4)

#define BYTES_PER_LOAD       16
#define BYTES_PER_AIND_LOAD  12
#define NUM_BYTES_FOR_ALPHA_INDICES 6

#define NUM_SUB_BLOCKS              2
#define NUM_ALPHA_ENDPOINT_LOADS    (A0_SIZE_PER_THREAD / BYTES_PER_LOAD)
#define NUM_AIND_LOADS              (AIND_SIZE_PER_THREAD / BYTES_PER_AIND_LOAD)
#define NUM_COLOR_ENDPOINT_LOADS    (C0_SIZE_PER_THREAD / BYTES_PER_LOAD)
#define NUM_CIND_LOADS              (CIND_SIZE_PER_THREAD / BYTES_PER_LOAD)
#define SIZE_OF_BC3_BLOCK           16
#define NUM_BC3_BLOCKS_PER_THREAD   (DATA_LOADED_PER_THREAD / SIZE_OF_BC3_BLOCK)
#define NUM_BC3_BLOCKS_PER_SUB_BLOCK (NUM_BC3_BLOCKS_PER_THREAD / NUM_SUB_BLOCKS)

struct BC3_BLOCK
{
    uint64_t a0_a1_aIndices; // [0:7]    = a0. [8:15]    = a1. [16:63] = alpha indices;
    uint64_t c0_c1_cIndices; // [0:15]   = c0. [16:31]   = c1. [32:63] = color indices;
};

BC3_BLOCK FormBC3Block(uint a0, uint a1, uint16_t3 alphaIndices, uint c0, uint c1, uint colorIndices)
{
    BC3_BLOCK block;
    block.a0_a1_aIndices = uint64_t(a0) | (uint64_t(a1) << 8) | (uint64_t(alphaIndices.x) << 16) | (uint64_t(alphaIndices.y) << 32) | (uint64_t(alphaIndices.z) << 48);
    block.c0_c1_cIndices = uint64_t(c0) | (uint64_t(c1) << 16) | (uint64_t(colorIndices) << 32);
    return block;
}

[numthreads(THREADS_PER_WAVE, 1, 1)]
[RootSignature("UAV(u0)")]

void main(uint groupID : SV_GroupID)
{
    uint threadIDInWave = WaveGetLaneIndex();
	
    uint blockAddress = groupID * BLOCK_SIZE; // Byte offset to our BLOCK_SIZE KB block.

    uint a0BaseAddress = blockAddress + A0_OFFSET;
    uint a1BaseAddress = blockAddress + A1_OFFSET;
    uint alphaIndicesBaseAddress = blockAddress + AIND_OFFSET;
    uint c0BaseAddress = blockAddress + C0_OFFSET;
    uint c1BaseAddress = blockAddress + C1_OFFSET;
    uint colorIndicesBaseAddress = blockAddress + CIND_OFFSET;
 	
    uint a0Arr[NUM_ALPHA_ENDPOINT_LOADS * 4];
    uint a1Arr[NUM_ALPHA_ENDPOINT_LOADS * 4];
    uint16_t alphaIndicesArr[NUM_AIND_LOADS * BYTES_PER_AIND_LOAD / sizeof(uint16_t)]; // Store as uint16_t because each block needs 6 bytes which is not a multiple of 4.
    uint c0Arr[NUM_COLOR_ENDPOINT_LOADS * 4];
    uint c1Arr[NUM_COLOR_ENDPOINT_LOADS * 4];
    uint colorIndicesArr[NUM_CIND_LOADS * 4];
    
    const uint STEP_SIZE = BYTES_PER_LOAD * THREADS_PER_WAVE;

    for (int i = 0; i < NUM_ALPHA_ENDPOINT_LOADS; ++i)
    {
        uint4 a0 = inputOutput.Load<uint4>(a0BaseAddress + (threadIDInWave * BYTES_PER_LOAD));
        uint4 a1 = inputOutput.Load<uint4>(a1BaseAddress + (threadIDInWave * BYTES_PER_LOAD));
        
        a0Arr[i * 4 + 0] = a0.x;
        a0Arr[i * 4 + 1] = a0.y;
        a0Arr[i * 4 + 2] = a0.z;
        a0Arr[i * 4 + 3] = a0.w;
        
        a1Arr[i * 4 + 0] = a1.x;
        a1Arr[i * 4 + 1] = a1.y;
        a1Arr[i * 4 + 2] = a1.z;
        a1Arr[i * 4 + 3] = a1.w;
        
        a0BaseAddress += (STEP_SIZE); // += 1KB
        a1BaseAddress += (STEP_SIZE);
    }
    
    // Alpha indices.
    uint alphaIndex = 0;

    [unroll]
    for (int i = 0; i < NUM_ALPHA_ENDPOINT_LOADS; ++i)
    {
        // For reach alpha endpoint load, (16 bytes, 16 A0's) we need to load 6x as much alpha endpoint indices.
        uint aIndPerThreadPerSubBlock = AIND_SIZE_PER_THREAD / NUM_SUB_BLOCKS;
        uint addr = alphaIndicesBaseAddress + (threadIDInWave * aIndPerThreadPerSubBlock);

        [unroll]
        for (int j = 0; j < aIndPerThreadPerSubBlock / BYTES_PER_AIND_LOAD; j++)
        {
            uint3 alphaIndices = inputOutput.Load<uint3>(addr + (j * BYTES_PER_AIND_LOAD));

            // Flatten the array from uint3 to uint for easier indexing.
            alphaIndicesArr[alphaIndex * 6 + 0] = (uint16_t) (alphaIndices.x & 0xFFFF);
            alphaIndicesArr[alphaIndex * 6 + 1] = (uint16_t) (alphaIndices.x >> 16);
            alphaIndicesArr[alphaIndex * 6 + 2] = (uint16_t) (alphaIndices.y & 0xFFFF);
            alphaIndicesArr[alphaIndex * 6 + 3] = (uint16_t) (alphaIndices.y >> 16);
            alphaIndicesArr[alphaIndex * 6 + 4] = (uint16_t) (alphaIndices.z & 0xFFFF);
            alphaIndicesArr[alphaIndex * 6 + 5] = (uint16_t) (alphaIndices.z >> 16);

            alphaIndex++;
        }
        
        alphaIndicesBaseAddress += (STEP_SIZE * 6); // TODO FIX (BYTES_PER_AIND_LOAD * THREADS_PER_WAVE * 6);
    }

    // Colour endpoints
    uint cIndex = 0;

    [unroll]
    for (int i = 0; i < NUM_ALPHA_ENDPOINT_LOADS; ++i)
    {
        // For reach alpha endpoint load, (16 bytes, 16 A0's) we need to load 2x as much colour endpoint data from c0 and c1
        uint c0Addr = c0BaseAddress + (threadIDInWave * BYTES_PER_LOAD * 2);
        uint c1Addr = c1BaseAddress + (threadIDInWave * BYTES_PER_LOAD * 2);

        [unroll]
        for (int j = 0; j < 2; j++)
        {
            uint4 c0 = inputOutput.Load<uint4>(c0Addr + (j * BYTES_PER_LOAD));
            uint4 c1 = inputOutput.Load<uint4>(c1Addr + (j * BYTES_PER_LOAD));

            c0Arr[cIndex * 4 + 0] = c0.x;
            c0Arr[cIndex * 4 + 1] = c0.y;
            c0Arr[cIndex * 4 + 2] = c0.z;
            c0Arr[cIndex * 4 + 3] = c0.w;
        
            c1Arr[cIndex * 4 + 0] = c1.x;
            c1Arr[cIndex * 4 + 1] = c1.y;
            c1Arr[cIndex * 4 + 2] = c1.z;
            c1Arr[cIndex * 4 + 3] = c1.w;

            cIndex++;
        }
        
        c0BaseAddress += (STEP_SIZE * 2);
        c1BaseAddress += (STEP_SIZE * 2);
    }

    // Colour indices
    uint cIndIndex = 0;

    [unroll]
    for (int i = 0; i < NUM_ALPHA_ENDPOINT_LOADS; ++i)
    {
        // For reach alpha endpoint load, (16 bytes, 16 A0's) we need to load 4x as much colour index data
        uint addr = colorIndicesBaseAddress + (threadIDInWave * BYTES_PER_LOAD * 4);

        [unroll]
        for (int j = 0; j < 4; j++)
        {
            uint4 colorIndices = inputOutput.Load<uint4>(addr + (j * BYTES_PER_LOAD));
        
            colorIndicesArr[cIndIndex * 4 + 0] = colorIndices.x;
            colorIndicesArr[cIndIndex * 4 + 1] = colorIndices.y;
            colorIndicesArr[cIndIndex * 4 + 2] = colorIndices.z;
            colorIndicesArr[cIndIndex * 4 + 3] = colorIndices.w;
        
            cIndIndex++;
        }
        
        colorIndicesBaseAddress += (STEP_SIZE * 4);
    }
        
    uint writeAddress = blockAddress + (threadIDInWave * (SIZE_OF_BC3_BLOCK * NUM_BC3_BLOCKS_PER_SUB_BLOCK));
    
    // Run through the 32 BC3 blocks we need to write, writing them one at a time (i.e. 16 bytes at a time).
    // A 32KB block is handled by 64 threads, meaning each thread is responsible for 512 bytes of data (32 BC3 blocks).
    // Because of the way we're trying to read and write contiguous data, we loaded from 2 separate locations (subblocks)
    // For each one of those 2 locations, we need to write 16 BC1 blocks worth of data (1 at a time).
    [unroll]
    for (int subBlock = 0; subBlock < NUM_SUB_BLOCKS; subBlock++)
    {
        [unroll]
        for (int i = 0; i < NUM_BC3_BLOCKS_PER_SUB_BLOCK; i++)
        {
            uint idx = subBlock * NUM_BC3_BLOCKS_PER_SUB_BLOCK + i;

            uint a0 = (a0Arr[idx / 4] >> (idx % 4) * 8) & 0xFF;         // Extract the 8-bit alpha value from the 32-bit uint.
            uint a1 = (a1Arr[idx / 4] >> (idx % 4) * 8) & 0xFF;         // Extract the 8-bit alpha value from the 32-bit uint.
            uint c0 = (c0Arr[idx / 2] >> (idx % 2) * 16) & 0xFFFF;      // Extract the 16-bit color value from the 32-bit uint.
            uint c1 = (c1Arr[idx / 2] >> (idx % 2) * 16) & 0xFFFF;      // Extract the 16-bit color value from the 32-bit uint.
            uint16_t3 alphaIndices = uint16_t3(alphaIndicesArr[idx * 3 + 0], alphaIndicesArr[idx * 3 + 1], alphaIndicesArr[idx * 3 + 2]);
            uint colorIndices = colorIndicesArr[idx];
        
            BC3_BLOCK bc3Block = FormBC3Block(a0, a1, alphaIndices, c0, c1, colorIndices);
        
            inputOutput.Store<BC3_BLOCK>(writeAddress + (i * SIZE_OF_BC3_BLOCK), bc3Block);
        }
           
        writeAddress += (BLOCK_SIZE / NUM_SUB_BLOCKS);
    }
}
