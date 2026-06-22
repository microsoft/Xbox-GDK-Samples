//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define NUM_SUBKEY_BITS (8)
#define NUM_SUBKEYS (1u << NUM_SUBKEY_BITS)
#define SUBKEY_MASK (NUM_SUBKEYS - 1u)
#define KERNEL_SIZE (2048)

cbuffer ShaderParams : register(b0)
{
    uint NumCountersPerSubKey;
    uint SubKeyShift;
    uint PrefixBlockSizeLog2L1;
};

uint v_bfe_u32(uint x, uint shift, uint width)
{
    return __XB_UBFE(width, shift, x);
}

uint UnpackSubKey(uint Key)
{
    return v_bfe_u32(Key, SubKeyShift, NUM_SUBKEY_BITS);
}

uint CountBits64(uint2 x)
{
    return countbits(x.x) + countbits(x.y);
}

uint AdvanceOffset(uint Offset, uint NumThreads)
{
    return (Offset + NumThreads) & (KERNEL_SIZE - 1);
}

struct RadixSubKeys8
{
    uint V[8];
};

RadixSubKeys8 Load8SubKeys(ByteAddressBuffer Keys, uint GroupId, uint ThreadIdInGroup, uint NumThreadsInGroup)
{
    RadixSubKeys8 SubKeys;
#if 1
    uint Base = GroupId * KERNEL_SIZE + ThreadIdInGroup;

    uint Offset = ((GroupId & 0xf) * NumThreadsInGroup) & (KERNEL_SIZE - 1);

    SubKeys.V[0] = Keys.Load((Base + Offset) << 2);
    Offset = AdvanceOffset(Offset, NumThreadsInGroup);

    SubKeys.V[1] = Keys.Load((Base + Offset) << 2);
    Offset = AdvanceOffset(Offset, NumThreadsInGroup);

    SubKeys.V[2] = Keys.Load((Base + Offset) << 2);
    Offset = AdvanceOffset(Offset, NumThreadsInGroup);

    SubKeys.V[3] = Keys.Load((Base + Offset) << 2);
    Offset = AdvanceOffset(Offset, NumThreadsInGroup);

    SubKeys.V[4] = Keys.Load((Base + Offset) << 2);
    Offset = AdvanceOffset(Offset, NumThreadsInGroup);

    SubKeys.V[5] = Keys.Load((Base + Offset) << 2);
    Offset = AdvanceOffset(Offset, NumThreadsInGroup);

    SubKeys.V[6] = Keys.Load((Base + Offset) << 2);
    Offset = AdvanceOffset(Offset, NumThreadsInGroup);

    SubKeys.V[7] = Keys.Load((Base + Offset) << 2);
#else
    uint Base = GroupId * KERNEL_SIZE + ThreadIdInGroup * 4;

    const uint4 Data0 = Keys.Load4(Base << 2);
    SubKeys.V[0] = Data0.x;
    SubKeys.V[1] = Data0.y;
    SubKeys.V[2] = Data0.z;
    SubKeys.V[3] = Data0.w;

    Base += 4 * NumThreadsInGroup;

    const uint4 Data1 = Keys.Load4(Base << 2);
    SubKeys.V[4 + 0] = Data1.x;
    SubKeys.V[4 + 1] = Data1.y;
    SubKeys.V[4 + 2] = Data1.z;
    SubKeys.V[4 + 3] = Data1.w;
#endif
    return SubKeys;
}

#define RootSig     \
    "RootFlags(0)," \
    "SRV(t0),"      \
    "SRV(t1),"      \
    "SRV(t2),"      \
    "UAV(u0),"      \
    "RootConstants(b0, num32bitconstants=3)"
