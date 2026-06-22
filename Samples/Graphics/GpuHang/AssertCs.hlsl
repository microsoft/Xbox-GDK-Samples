//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE\
    RootSignature\
    (\
        "\
            SRV(t0),\
            UAV(u0),\
        "\
    )\

#define SHADER_ASSERT(cond)\
{\
    if (!(cond))\
    {\
        __XB_DebugBreak();\
    }\
}

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
    for (uint i = 0; i < 255; ++i)
    {
        SHADER_ASSERT(bufIn[i].value < 100);

        bufOut[i] = (Output) bufIn[i];
    }
}
