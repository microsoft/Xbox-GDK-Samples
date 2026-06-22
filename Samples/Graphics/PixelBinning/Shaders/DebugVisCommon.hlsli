//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define DebugVisRootSig \
    "UAV(u0)," \
    "DescriptorTable(UAV(u1, numDescriptors=1))," \
    "DescriptorTable(UAV(u2, numDescriptors=1))," \
    "RootConstants(b0, num32bitconstants=4)"

#define DEBUG_VIS_VERIFY_PIXEL_TOUCHED_INIT_KEY 0x11111111
#define DEBUG_VIS_VERIFY_PIXEL_TOUCHED_SEED_KEY 0x5eed5eed
#define DEBUG_VIS_VERIFY_PIXEL_TOUCHED_FAIL_KEY 0xf411f411

cbuffer ShaderParams : register(b0)
{
    uint  dispatchSize;
    uint  dispatch1dOffset;
    uint2 dispatch2dOffset;
};