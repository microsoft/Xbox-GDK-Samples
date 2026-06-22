//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//  Warning X4803: XBOX PERF: Generated shader instructions make use of per-thread scratch memory.
#pragma warning(disable : 4803)

#define ROOT_SIGNATURE\
    RootSignature\
    (\
        "\
            SRV(t0),\
            UAV(u0),\
        "\
    )\


struct Input
{
    uint value;
};
StructuredBuffer<Input> bufIn : register(t0);
struct Output
{
    uint value;
};
RWStructuredBuffer<Output> bufOut : register(u0);

[numthreads(1, 1, 1)]
[ROOT_SIGNATURE]
void main(uint3 id : SV_DispatchThreadID)
{
    // Currently, the compiler spills scratchArray to scratch memory
    // To be sure, check the disassembly for "ScratchSize" in the comments
    uint scratchArray[255];
    for (uint i = 0; i < 255; ++i)
    {
        scratchArray[i] = bufIn[i].value;
    }

    bufOut[0].value = scratchArray[bufIn[0].value];
}
