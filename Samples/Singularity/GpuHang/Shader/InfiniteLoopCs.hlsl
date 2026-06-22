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


struct Node
{
    uint next;
    uint value;
};
StructuredBuffer<Node> bufIn : register(t0);
struct Output
{
    uint value;
};
RWStructuredBuffer<Output> bufOut : register(u0);

[numthreads(1, 1, 1)]
[ROOT_SIGNATURE]
void main(uint3 id : SV_DispatchThreadID)
{
    // Follow the linked list to the end.
    // This is very sensitive to bad data.
    Node node = bufIn[id.x];
    while (node.next != 0xffffffff)
    {
        node = bufIn[node.next];
    }

    bufOut[0].value = node.value;
}