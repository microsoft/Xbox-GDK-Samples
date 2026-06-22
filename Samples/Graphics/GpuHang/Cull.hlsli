//------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

bool CullCalculation(uint i)
{
    // Pretend this does useful work
    return i & 0x1;
}

#define ROOT_SIGNATURE \
    RootSignature\
    (\
        "\
        SRV(t0),\
        DescriptorTable(UAV(u0)), \
        "\
    )

struct D3D12_SET_COMPUTE_ROOT_32BIT_CONSTANT_ARGS
{
    uint SrcData;
};
struct D3D12_DISPATCH_ARGUMENTS
{
    uint ThreadGroupCountX;
    uint ThreadGroupCountY;
    uint ThreadGroupCountZ;
};
struct IndirectArgsGraphics
{
    D3D12_SET_COMPUTE_ROOT_32BIT_CONSTANT_ARGS m_setComputeRoot32BitConstantArgs;
    D3D12_DISPATCH_ARGUMENTS m_dispatchArgs;
};

struct D3D12_DISPATCHX_ARGUMENTS
{
    uint ThreadGroupCountX;
    uint ThreadGroupCountY;
    uint ThreadGroupCountZ;
    uint Padding;
};
struct IndirectArgsAsync
{
    D3D12_DISPATCHX_ARGUMENTS m_dispatchArgs;
};

StructuredBuffer<IndirectArgsGraphics> bufIndirectArgsIn : register(t0);
AppendStructuredBuffer<IndirectArgsOut> bufIndirectArgsOut : register(u0);

[ROOT_SIGNATURE]
[numthreads(64, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint i = DispatchThreadID.x;
    if (CullCalculation(i))
    {
        IndirectArgsGraphics indirectArgsIn = bufIndirectArgsIn[i];
        IndirectArgsOut indirectArgsOut = { -1, -1, -1, -1, };

        indirectArgsOut.m_dispatchArgs.ThreadGroupCountX = indirectArgsIn.m_dispatchArgs.ThreadGroupCountX;
        indirectArgsOut.m_dispatchArgs.ThreadGroupCountY = indirectArgsIn.m_dispatchArgs.ThreadGroupCountY;
        indirectArgsOut.m_dispatchArgs.ThreadGroupCountZ = indirectArgsIn.m_dispatchArgs.ThreadGroupCountZ;
        
        bufIndirectArgsOut.Append(indirectArgsOut);
    }
}
