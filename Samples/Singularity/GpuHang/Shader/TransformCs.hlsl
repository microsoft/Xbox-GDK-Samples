//------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

float4 ApplyTransform(float4x4 transform, float4 position)
{
    return mul(position, transform);
}

#define ROOT_SIGNATURE \
    RootSignature\
    (\
        "\
        CBV(b0),\
        SRV(t0),\
        UAV(u0), \
        RootConstants(num32BitConstants=1, b1), \
        "\
    )

struct Transform
{
    float4x4 transform;
};
ConstantBuffer<Transform> bufTransform : register(b0);

struct Offset
{
    uint offset;
};
ConstantBuffer<Offset> bufOffset : register(b1);

struct Data
{
    float4 position;
};
StructuredBuffer<Data> bufIn : register(t0);
RWStructuredBuffer<Data> bufOut : register(u0);

// In a real case, we might be doing something more complicated, like pre-skinning, or applying morphs
[ROOT_SIGNATURE]
[numthreads(64, 1, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    // Without the +1, we would read/write out of bounds, given that we have set up the offset to be -1
    uint i = DispatchThreadID.x + bufOffset.offset + 1;
    bufOut[i].position = ApplyTransform(bufTransform.transform, bufIn[i].position);
}
