//--------------------------------------------------------------------------------------
// UnshuffleBC1.hlsl
//
// DirectStorage Shuffle Formats: 
// - <https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/system/overviews/directstorage/directstorage-shuffles>
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

RWByteAddressBuffer inputOutput : register(u0);

#define BLOCK_SIZE 			16384
#define E0_OFFSET			0
#define E1_OFFSET			(BLOCK_SIZE / 4)
#define INDICES_OFFSET		(BLOCK_SIZE / 2)

#define THREADS_PER_WAVE 		64
#define DATA_LOADED_PER_THREAD 	(BLOCK_SIZE / THREADS_PER_WAVE)
#define E0_SIZE_PER_THREAD		(DATA_LOADED_PER_THREAD / 4)
#define E1_SIZE_PER_THREAD		E0_SIZE_PER_THREAD
#define INDICES_SIZE_PER_THREAD	(DATA_LOADED_PER_THREAD / 2)

#define BYTES_PER_LOAD          16

#define NUM_ENDPOINT_LOADS      (E0_SIZE_PER_THREAD / BYTES_PER_LOAD)
#define NUM_INDICES_LOADS       (INDICES_SIZE_PER_THREAD / BYTES_PER_LOAD)
#define SIZE_OF_BC1_BLOCK       8
#define NUM_BC1_BLOCKS_PER_THREAD   (DATA_LOADED_PER_THREAD / SIZE_OF_BC1_BLOCK)

[numthreads(THREADS_PER_WAVE,1,1)]
[RootSignature("UAV(u0)")]

void main(uint groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint threadIDInWave = WaveGetLaneIndex();
	
    uint blockAddress = groupID * BLOCK_SIZE;               // Byte offset to our BLOCK_SIZE KB block.
    uint e0BaseAddress = blockAddress + E0_OFFSET;
    uint e1BaseAddress = blockAddress + E1_OFFSET;
    uint indicesBaseAddress = blockAddress + INDICES_OFFSET;
 	
    uint ep0Arr[NUM_ENDPOINT_LOADS * 4];
    uint ep1Arr[NUM_ENDPOINT_LOADS * 4];
    uint indicesArr[NUM_INDICES_LOADS * 4];
    
    const uint STEP_SIZE = BYTES_PER_LOAD * THREADS_PER_WAVE;

    // Each thread loads 16 bytes of EP0 or EP1 at a time, with all threads doing a contiguous load of 16 * 64 bytes at a time (no holes!).
    // The GPU's caches don't like it when you load with gaps (spaced apart by 64 bytes from each other), so we load in a contiguous fashion
    // and store in a contiguous fashion.
    // The old method would load bytes 0-15, 64-79, 128-143, etc. This would cause cache misses and thrash the cache.
    // Now we load bytes 0-15, 16-31, 32-47, etc. This is contiguous and the cache is happy.
    // This just means that each thread loads 8 BC blocks worth of data at a time, and then jumps ahead by (8*64) BC blocks each time
    // for the 2nd, 3rd and 4th loads.
    for (int i = 0; i < NUM_ENDPOINT_LOADS; ++i)
    {
        uint4 ep0 = inputOutput.Load<uint4>(e0BaseAddress + (threadIDInWave * BYTES_PER_LOAD));
        uint4 ep1 = inputOutput.Load<uint4>(e1BaseAddress + (threadIDInWave * BYTES_PER_LOAD));
        
        ep0Arr[i * 4 + 0] = ep0.x;
        ep0Arr[i * 4 + 1] = ep0.y;
        ep0Arr[i * 4 + 2] = ep0.z;
        ep0Arr[i * 4 + 3] = ep0.w;
        
        ep1Arr[i * 4 + 0] = ep1.x;
        ep1Arr[i * 4 + 1] = ep1.y;
        ep1Arr[i * 4 + 2] = ep1.z;
        ep1Arr[i * 4 + 3] = ep1.w;
        
        e0BaseAddress += (STEP_SIZE);
        e1BaseAddress += (STEP_SIZE);
    }
    
    for (int i = 0; i < NUM_INDICES_LOADS / 2; ++i)
    {
        uint addr = indicesBaseAddress + (threadIDInWave * BYTES_PER_LOAD * 2); // 32 bytes per load, so we need to multiply by 2.
        
        uint4 indicesA = inputOutput.Load<uint4>(addr);        
        uint4 indicesB = inputOutput.Load<uint4>(addr + 16);
        
        // Flatten the array from uint4 to uint for easier indexing.
        indicesArr[i * 8 + 0] = indicesA.x;
        indicesArr[i * 8 + 1] = indicesA.y;
        indicesArr[i * 8 + 2] = indicesA.z;
        indicesArr[i * 8 + 3] = indicesA.w;
        indicesArr[i * 8 + 4] = indicesB.x;
        indicesArr[i * 8 + 5] = indicesB.y;
        indicesArr[i * 8 + 6] = indicesB.z;
        indicesArr[i * 8 + 7] = indicesB.w;
        
        indicesBaseAddress += (STEP_SIZE * 2);
    }

    uint writeAddress = blockAddress + (threadIDInWave * (SIZE_OF_BC1_BLOCK * 8)); // Each thread writes 64B (8 BC1 blocks).
    
    // Run through the 32 BC1 blocks we need to write, writing them in pairs so we write 16 bytes at a time.
    // A 16KB block is handled by 64 threads, meaning each thread is responsible for 256 bytes of data (32 BC1 blocks).
    // Because of the way we're trying to read and write contiguous data, we loaded from 4 separate locations inside EP0, EP1 and Indices.
    // For each one of those 4 locations, we need to write 8 BC1 blocks worth of data (2 at a time).
    for (int subBlock = 0; subBlock < 4; subBlock++)
    {        
        for (int i = 0; i < 4; i++)
        {
            uint idx = subBlock * 4 + i;
            uint ep0AB = ep0Arr[idx];
            uint ep1AB = ep1Arr[idx];
            uint indicesA = indicesArr[idx * 2 + 0];
            uint indicesB = indicesArr[idx * 2 + 1];
        
            uint4 toWrite;
            toWrite.x = (ep0AB & 0xFFFF) | (ep1AB << 16);
            toWrite.y = indicesA;
            toWrite.z = (ep0AB >> 16) | (ep1AB & 0xFFFF0000);
            toWrite.w = indicesB;

            inputOutput.Store<uint4>(writeAddress + (i * 16), toWrite);
        }
   
        writeAddress += (BLOCK_SIZE / 4);
    }
    
}
