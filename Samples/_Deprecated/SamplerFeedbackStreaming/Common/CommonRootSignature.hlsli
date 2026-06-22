//--------------------------------------------------------------------------------------
// CommonRootSignature.hlsli
//
// HLSL root signature definitions for UI image rendering (RS_UISprite12).
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define RS_UISprite12 \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
    "DescriptorTable( "\
        "SRV(t0, space = 0, numDescriptors = 2)" \
    ")," \
    "DescriptorTable( "\
        "Sampler(s0, space = 0, numDescriptors = 1)" \
    ")," \
    "CBV(b0)," \
    "DescriptorTable( "\
        "UAV(u0, space = 0, numDescriptors = 1)" \
    ")" \
