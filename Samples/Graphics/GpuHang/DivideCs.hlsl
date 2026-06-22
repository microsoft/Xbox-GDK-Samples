//------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE\
    RootSignature\
    (\
        "\
            SRV(t0),\
            SRV(t1),\
            UAV(u0),\
        "\
    )\


StructuredBuffer<float> bufInNumerator : register(t0);
StructuredBuffer<float> bufInDenominator : register(t1);
RWStructuredBuffer<float> bufOut : register(u0);

[numthreads(64, 1, 1)]
[ROOT_SIGNATURE]
void main(uint3 id : SV_DispatchThreadID)
{
    bufOut[id.x] = bufInNumerator[id.x] / bufInDenominator[id.x];
}
