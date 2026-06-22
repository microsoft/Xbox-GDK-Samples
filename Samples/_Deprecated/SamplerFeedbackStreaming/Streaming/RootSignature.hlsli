//--------------------------------------------------------------------------------------
// RootSignature.hlsli
//
// HLSL root signature definitions for scene rendering (RS_Streaming12) and UI image
// rendering (RS_UISprite12).
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "..\Common\CommonRootSignature.hlsli"

#define RS_Streaming12 \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
    "CBV(b0)," \
    "CBV(b1)," \
    "DescriptorTable( "\
        "SRV(t0, space = 0, numDescriptors = 9)" \
    ")," \
    "DescriptorTable( "\
        "SRV(t0, space = 1, numDescriptors = 9)" \
    ")," \
    "DescriptorTable( "\
        "UAV(u0, space = 2, numDescriptors = 9)" \
    ")," \
    "DescriptorTable( "\
        "Sampler(s0, numDescriptors = 16)" \
    ")" 

#define RS_FeedbackResolve \
    "DescriptorTable( "\
        "UAV(u0, numDescriptors = 1)" \
    ")," \
    "DescriptorTable( "\
        "UAV(u1, numDescriptors = 1)" \
    ")," \
    "CBV(b0)" 
