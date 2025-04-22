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

#define MACROTILE_SIZE 			    32768
#define R0_OFFSET			        0
#define R1_OFFSET			        (MACROTILE_SIZE / 16)
#define R_INDICES_OFFSET		    (MACROTILE_SIZE / 8)
#define G0_OFFSET			        (MACROTILE_SIZE / 2)
#define G1_OFFSET			        G0_OFFSET + (MACROTILE_SIZE / 16)
#define G_INDICES_OFFSET		    G1_OFFSET + (MACROTILE_SIZE / 16)

#define THREADS_PER_WAVE 		    64

#define DATA_LOADED_PER_THREAD 	    (MACROTILE_SIZE / THREADS_PER_WAVE)
#define R0_SIZE_PER_THREAD		    (DATA_LOADED_PER_THREAD / 16)
#define R1_SIZE_PER_THREAD		    R0_SIZE_PER_THREAD
#define R_INDICES_SIZE_PER_THREAD	(DATA_LOADED_PER_THREAD / 2)  - R0_SIZE_PER_THREAD - R1_SIZE_PER_THREAD

#define G0_SIZE_PER_THREAD		    R0_SIZE_PER_THREAD
#define G1_SIZE_PER_THREAD		    R1_SIZE_PER_THREAD
#define G_INDICES_SIZE_PER_THREAD	R_INDICES_SIZE_PER_THREAD 

#define BYTES_PER_LOAD              16
#define UINTS_PER_LOAD              BYTES_PER_LOAD / 4
#define ENDPOINTS_PER_UINT          4
#define BYTES_PER_UINT              4

#define NUM_ENDPOINT_LOADS          (R0_SIZE_PER_THREAD / BYTES_PER_LOAD)
#define NUM_WRITE_LOCATIONS         NUM_ENDPOINT_LOADS
#define SIZE_OF_BC5_BLOCK           16 // In Bytes
#define NUM_BC5_BLOCKS_PER_THREAD   (DATA_LOADED_PER_THREAD / SIZE_OF_BC5_BLOCK)
#define BC5_BLOCKS_PER_WRITEGROUP   NUM_BC5_BLOCKS_PER_THREAD / NUM_WRITE_LOCATIONS / ENDPOINTS_PER_UINT

[numthreads(THREADS_PER_WAVE, 1, 1)]
[RootSignature("UAV(u0)")]

// For every executing wave of 64 threads, a 32 KB macro tile of BC5 data is processed with its data originally swizzled the following way:
// [R0 (2K)][R1 (2K)][R-Indices (12K)][G0 (2K)][G1 (2K)][G-Indices (12K)]
// The data is loaded into shader registers [1], and then written back into the UAV as reconstructed BC5 blocks [2].

// Block Compression formats can be found here for reference: 
// https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression

void main(uint groupID : SV_GroupID)
{
    uint threadIDInWave = WaveGetLaneIndex();
	
    uint macroTileAddress = groupID * MACROTILE_SIZE;

    uint r0BaseAddress = macroTileAddress + R0_OFFSET;
    uint r1BaseAddress = macroTileAddress + R1_OFFSET;
    uint rIndicesBaseAddress = macroTileAddress + R_INDICES_OFFSET;

    uint g0BaseAddress = macroTileAddress + G0_OFFSET;
    uint g1BaseAddress = macroTileAddress + G1_OFFSET;
    uint gIndicesBaseAddress = macroTileAddress + G_INDICES_OFFSET;
 	
    uint r0Arr[R0_SIZE_PER_THREAD / ENDPOINTS_PER_UINT];
    uint r1Arr[R1_SIZE_PER_THREAD / ENDPOINTS_PER_UINT];
    uint rIndicesArr[R_INDICES_SIZE_PER_THREAD / BYTES_PER_UINT];

    uint g0Arr[G0_SIZE_PER_THREAD / ENDPOINTS_PER_UINT];
    uint g1Arr[G1_SIZE_PER_THREAD / ENDPOINTS_PER_UINT];
    uint gIndicesArr[G_INDICES_SIZE_PER_THREAD / BYTES_PER_UINT];
    
    const uint STEP_SIZE = BYTES_PER_LOAD * THREADS_PER_WAVE;

    // [1] Read Data from UAV to Registers

    // Each thread loads 16 bytes of R0, R1, G0 or G1 at a time, with all threads performing a contiguous load of 16 * 64 bytes at a time to minimize cache thrashing.
    // This is achieved by dividing 1 KB of endpoint data amongst threads in a wave, and then advancing each thread's load 1KB further than their last fetch address,
    // until all threads have gathered their required number of BC5 block data.

    [unroll]
    for (int i = 0; i < NUM_ENDPOINT_LOADS; ++i)
    {
        uint4 r0 = inputOutput.Load<uint4>(r0BaseAddress + (threadIDInWave * BYTES_PER_LOAD));
        uint4 r1 = inputOutput.Load<uint4>(r1BaseAddress + (threadIDInWave * BYTES_PER_LOAD));

        uint4 g0 = inputOutput.Load<uint4>(g0BaseAddress + (threadIDInWave * BYTES_PER_LOAD));
        uint4 g1 = inputOutput.Load<uint4>(g1BaseAddress + (threadIDInWave * BYTES_PER_LOAD));
        
        // An R0, R1, G0 or G1 array entry holds 4 texels worth of information.
        // Thus, every 16-byte load transfers 16 R0 or R1 values in total, on every loop iteration.

        r0Arr[i * 4 + 0] = r0.x;
        r0Arr[i * 4 + 1] = r0.y;
        r0Arr[i * 4 + 2] = r0.z;
        r0Arr[i * 4 + 3] = r0.w;
        
        r1Arr[i * 4 + 0] = r1.x;
        r1Arr[i * 4 + 1] = r1.y;
        r1Arr[i * 4 + 2] = r1.z;
        r1Arr[i * 4 + 3] = r1.w;

        g0Arr[i * 4 + 0] = g0.x;
        g0Arr[i * 4 + 1] = g0.y;
        g0Arr[i * 4 + 2] = g0.z;
        g0Arr[i * 4 + 3] = g0.w;
        
        g1Arr[i * 4 + 0] = g1.x;
        g1Arr[i * 4 + 1] = g1.y;
        g1Arr[i * 4 + 2] = g1.z;
        g1Arr[i * 4 + 3] = g1.w;
        
        r0BaseAddress += (STEP_SIZE);
        r1BaseAddress += (STEP_SIZE);
        g0BaseAddress += (STEP_SIZE);
        g1BaseAddress += (STEP_SIZE);
    }
    
    // Every BC5 block needs 6 bytes of R-indices and 6 bytes of G-indices of data. With 16 endpoints from a 16-byte load,  
    // each thread needs to load (6+6)x16 = 192 bytes of indices at a time.
    [unroll]
    for (int i = 0; i < NUM_ENDPOINT_LOADS; ++i)
    {
        uint rAddr = rIndicesBaseAddress + (threadIDInWave * BYTES_PER_LOAD * 6);
        uint gAddr = gIndicesBaseAddress + (threadIDInWave * BYTES_PER_LOAD * 6);

        // R-Indices
        uint4 indicesA = inputOutput.Load<uint4>(rAddr);
        uint4 indicesB = inputOutput.Load<uint4>(rAddr + 16);
        uint4 indicesC = inputOutput.Load<uint4>(rAddr + 32);
        uint4 indicesD = inputOutput.Load<uint4>(rAddr + 48);
        uint4 indicesE = inputOutput.Load<uint4>(rAddr + 64);
        uint4 indicesF = inputOutput.Load<uint4>(rAddr + 80);

        // Flatten the array from uint4 to uint for easier indexing.
        rIndicesArr[i * 24 + 0] = indicesA.x;
        rIndicesArr[i * 24 + 1] = indicesA.y;
        rIndicesArr[i * 24 + 2] = indicesA.z;
        rIndicesArr[i * 24 + 3] = indicesA.w;

        rIndicesArr[i * 24 + 4] = indicesB.x;
        rIndicesArr[i * 24 + 5] = indicesB.y;
        rIndicesArr[i * 24 + 6] = indicesB.z;
        rIndicesArr[i * 24 + 7] = indicesB.w;

        rIndicesArr[i * 24 + 8] = indicesC.x;
        rIndicesArr[i * 24 + 9] = indicesC.y;
        rIndicesArr[i * 24 + 10] = indicesC.z;
        rIndicesArr[i * 24 + 11] = indicesC.w;

        rIndicesArr[i * 24 + 12] = indicesD.x;
        rIndicesArr[i * 24 + 13] = indicesD.y;
        rIndicesArr[i * 24 + 14] = indicesD.z;
        rIndicesArr[i * 24 + 15] = indicesD.w;

        rIndicesArr[i * 24 + 16] = indicesE.x;
        rIndicesArr[i * 24 + 17] = indicesE.y;
        rIndicesArr[i * 24 + 18] = indicesE.z;
        rIndicesArr[i * 24 + 19] = indicesE.w;

        rIndicesArr[i * 24 + 20] = indicesF.x;
        rIndicesArr[i * 24 + 21] = indicesF.y;
        rIndicesArr[i * 24 + 22] = indicesF.z;
        rIndicesArr[i * 24 + 23] = indicesF.w;

        // G-Indices
        indicesA = inputOutput.Load<uint4>(gAddr);
        indicesB = inputOutput.Load<uint4>(gAddr + 16);
        indicesC = inputOutput.Load<uint4>(gAddr + 32);
        indicesD = inputOutput.Load<uint4>(gAddr + 48);
        indicesE = inputOutput.Load<uint4>(gAddr + 64);
        indicesF = inputOutput.Load<uint4>(gAddr + 80);

        // Flatten the array from uint4 to uint for easier indexing.
        gIndicesArr[i * 24 + 0] = indicesA.x;
        gIndicesArr[i * 24 + 1] = indicesA.y;
        gIndicesArr[i * 24 + 2] = indicesA.z;
        gIndicesArr[i * 24 + 3] = indicesA.w;

        gIndicesArr[i * 24 + 4] = indicesB.x;
        gIndicesArr[i * 24 + 5] = indicesB.y;
        gIndicesArr[i * 24 + 6] = indicesB.z;
        gIndicesArr[i * 24 + 7] = indicesB.w;

        gIndicesArr[i * 24 + 8] = indicesC.x;
        gIndicesArr[i * 24 + 9] = indicesC.y;
        gIndicesArr[i * 24 + 10] = indicesC.z;
        gIndicesArr[i * 24 + 11] = indicesC.w;

        gIndicesArr[i * 24 + 12] = indicesD.x;
        gIndicesArr[i * 24 + 13] = indicesD.y;
        gIndicesArr[i * 24 + 14] = indicesD.z;
        gIndicesArr[i * 24 + 15] = indicesD.w;

        gIndicesArr[i * 24 + 16] = indicesE.x;
        gIndicesArr[i * 24 + 17] = indicesE.y;
        gIndicesArr[i * 24 + 18] = indicesE.z;
        gIndicesArr[i * 24 + 19] = indicesE.w;

        gIndicesArr[i * 24 + 20] = indicesF.x;
        gIndicesArr[i * 24 + 21] = indicesF.y;
        gIndicesArr[i * 24 + 22] = indicesF.z;
        gIndicesArr[i * 24 + 23] = indicesF.w;
    
        rIndicesBaseAddress += (STEP_SIZE * 6);
        gIndicesBaseAddress += (STEP_SIZE * 6);
    }

    // [2] Reconstruct BC5 Blocks from Registers and Write Back to UAV

    // Each thread processes 512 bytes from the 32 KB macrotile and writes 32 BC5 blocks accordingly.
    // Since every thread loaded BC5 data from 2 different macrotile addresses, they can form
    // 32/2 = 16 contiguous BC5 blocks at a time. These blocks must be written
    // back to the UAV in the same order that threads have read from the resource.

    // For example, if thread 1 read the 1st-16th, 65th-80th, 145th-160th, and 225th-240th endpoints from the 4K pool in the UAV,  
    // then that thread needs to write the 1st-16th, 65th-80th, 145th-160th, and 225th-240th BC5 blocks in the UAV.
    // Otherwise, different colors will be written into different locations, resulting in a corrupted image.

    uint writeAddress = macroTileAddress + (threadIDInWave * (SIZE_OF_BC5_BLOCK * (NUM_BC5_BLOCKS_PER_THREAD / NUM_WRITE_LOCATIONS))); // Each thread writes 128B (16 BC5 blocks) at each write location.

    [unroll]
    for (int writeGroupIndex = 0; writeGroupIndex < NUM_WRITE_LOCATIONS; writeGroupIndex++)
    {
        [unroll]
        for (int i = 0; i < BC5_BLOCKS_PER_WRITEGROUP; i++)
        {
            // [2.1] Gather color and index data from registers for every 2 BC5 blocks to be written.
            uint idx = writeGroupIndex * BC5_BLOCKS_PER_WRITEGROUP + i;

            uint r0DCBA = r0Arr[idx]; // Each r0 in BC5 is 1 byte.
            uint r1DCBA = r1Arr[idx];

            uint g0DCBA = g0Arr[idx]; // Each r0 in BC5 is 1 byte.
            uint g1DCBA = g1Arr[idx];

            // As 12 indices bytes are required for each BC5 block, a total of 12 x 4 = 48 bytes need to be loaded. 
            uint rIndicesA  = rIndicesArr[idx * 6 + 0];
            uint rIndicesBA = rIndicesArr[idx * 6 + 1];
            uint rIndicesB  = rIndicesArr[idx * 6 + 2];

            uint rIndicesC  = rIndicesArr[idx * 6 + 3];
            uint rIndicesDC = rIndicesArr[idx * 6 + 4];
            uint rIndicesD  = rIndicesArr[idx * 6 + 5];

            uint gIndicesA  = gIndicesArr[idx * 6 + 0];
            uint gIndicesBA = gIndicesArr[idx * 6 + 1];
            uint gIndicesB  = gIndicesArr[idx * 6 + 2];

            uint gIndicesC  = gIndicesArr[idx * 6 + 3];
            uint gIndicesDC = gIndicesArr[idx * 6 + 4];
            uint gIndicesD  = gIndicesArr[idx * 6 + 5];

            // [2.2] Write and assemble first BC5 block (A).
            // BC5 Block has r0 = [0:7], r1 = [8:15], r_indices = [16:63], g0 = [64:71], g1 = [72:79], g_indices = [80:127]
            uint4 toWrite;
           
            toWrite.x = (r0DCBA & 0xFF) | ((r1DCBA << 8) & 0xFF00) | (rIndicesA << 16);
            toWrite.y = (rIndicesA >> 16) | (rIndicesBA << 16);
            toWrite.z = (g0DCBA & 0xFF) | ((g1DCBA << 8) & 0xFF00) | (gIndicesA << 16);
            toWrite.w = (gIndicesA >> 16) | (gIndicesBA << 16);
                        
            inputOutput.Store<uint4>(writeAddress + (i * 64), toWrite);
            
            // [2.3] Write and construct the next BC5 block (B).
            toWrite.x = ((r0DCBA >> 8) & 0xFF) | (r1DCBA & 0xFF00) | (rIndicesBA & 0xFFFF0000);
            toWrite.y = rIndicesB;
            toWrite.z = ((g0DCBA >> 8) & 0xFF) | (g1DCBA & 0xFF00) | (gIndicesBA & 0xFFFF0000);
            toWrite.w = gIndicesB;

            inputOutput.Store<uint4>(writeAddress + (i * 64) + 16, toWrite);

            // [2.4] Write and construct the next BC5 block (C).
            toWrite.x = ((r0DCBA >> 16) & 0xFF) | ((r1DCBA >> 8) & 0xFF00) | (rIndicesC << 16);
            toWrite.y = (rIndicesC >> 16) | (rIndicesDC << 16);
            toWrite.z = ((g0DCBA >> 16) & 0xFF) | ((g1DCBA >> 8) & 0xFF00) | (gIndicesC << 16);
            toWrite.w = (gIndicesC >> 16) | (gIndicesDC << 16);

            inputOutput.Store<uint4>(writeAddress + (i * 64) + 32, toWrite);

            // [2.5] Write and construct the next BC5 block (D).
            toWrite.x = (r0DCBA >> 24) | ((r1DCBA & 0xFF000000) >> 16) | (rIndicesDC & 0xFFFF0000);
            toWrite.y = rIndicesD;
            toWrite.z = (g0DCBA >> 24) | ((g1DCBA & 0xFF000000) >> 16) | (gIndicesDC & 0xFFFF0000);
            toWrite.w = gIndicesD;

            inputOutput.Store<uint4>(writeAddress + (i * 64) + 48, toWrite);
        }
        writeAddress += (MACROTILE_SIZE / NUM_WRITE_LOCATIONS);
    }
}
