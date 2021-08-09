//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.h"

#define ROOT_SIG "CBV(b0), \
                  RootConstants(Num32BitConstants=2, b1), \
                  CBV(b2), \
                  SRV(t0), \
                  SRV(t1), \
                  SRV(t2), \
                  SRV(t3), \
                  SRV(t4)"

ConstantBuffer<Constants> Globals : register(b0);

struct VertexOut
{
    float3  PositionVS : POSITION0;
    uint    MeshletIndex : COLOR0;
    float3  Normal : NORMAL0;
    uint    _unused : COLOR1;
    float4  PositionHS : SV_Position;
};
