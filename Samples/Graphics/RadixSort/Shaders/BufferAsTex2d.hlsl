//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define NUM_THREADS 64

RWByteAddressBuffer InBuffer : register(u0);
RWTexture2D<float3> OutTex2d : register(u1);

cbuffer ShaderParams : register(b0)
{
    uint TextureWidth;
}

[RootSignature(
    "UAV(u0),"
    "DescriptorTable(UAV(u1, numDescriptors=1)),"
    "RootConstants(b0, num32bitconstants=1)"
)]
[numthreads(NUM_THREADS, 1, 1)]
void main(uint GroupId : SV_GroupId, uint ThreadId : SV_GroupThreadId)
{
    const uint Base = GroupId * NUM_THREADS + ThreadId;
    const uint Data = InBuffer.Load(Base << 2);

    const uint CoordX = Base % TextureWidth;
    const uint CoordY = Base / TextureWidth;

    float3 Key = asfloat(Data);

    if (Base < 3840 * 2160-1)
    {
        uint DataNext = InBuffer.Load((Base << 2) + 4);
        const bool ErrorCondition = (DataNext) < (Data);
        #if 1
        if (any(__XB_Ballot64(ErrorCondition) != 0))
        #else
        if (ErrorCondition)
        #endif
        {
            Key = lerp(Key, float3(1.0, 0.0, 0.0), 0.5);
        }
    }

    #if 0
    OutTex2d[uint2(CoordX, CoordY)] = float(Data) / 4096.0;
    #else
    OutTex2d[uint2(CoordX, CoordY)] = Key;// asfloat(Data);
    #endif
}
