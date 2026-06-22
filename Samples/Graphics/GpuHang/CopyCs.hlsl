//------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE\
    RootSignature\
    (\
        "\
            SRV(t0),\
            UAV(u0),\
        "\
    )\


StructuredBuffer<uint> bufIn : register(t0);
RWStructuredBuffer<uint> bufOut : register(u0);

[numthreads(64, 1, 1)]
[ROOT_SIGNATURE]
void main(uint3 id : SV_DispatchThreadID)
{
    bufOut[id.x] = bufIn[id.x];
}
