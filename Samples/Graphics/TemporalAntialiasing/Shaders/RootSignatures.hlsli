//--------------------------------------------------------------------------------------
// RootSignatures.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Geometry pass root signature
#define GeometryPassRS "\
    RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), \
    CBV(b0), \
    CBV(b1), \
    DescriptorTable( \
        SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s0, visibility = SHADER_VISIBILITY_PIXEL)"

// Fullscreen pass root signature
#define FullscreenPassRS "\
    RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), \
    CBV(b0), \
    DescriptorTable( \
        SRV(t0, numDescriptors = 2), visibility = SHADER_VISIBILITY_PIXEL), \
    DescriptorTable( \
        SRV(t2, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL), \
    DescriptorTable( \
        UAV(u0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s0, visibility = SHADER_VISIBILITY_PIXEL)" 

// Temporal resolve pass root signature
#define TemporalResolveRS "\
    CBV(b0), \
    DescriptorTable( \
        SRV(t0, numDescriptors = 2)), \
    DescriptorTable( \
        SRV(t2, numDescriptors = 1)), \
    DescriptorTable( \
        SRV(t3, numDescriptors = 1)), \
    DescriptorTable( \
        UAV(u0, numDescriptors = 1)), \
    StaticSampler(s0, \
        addressU = TEXTURE_ADDRESS_BORDER, \
        addressV = TEXTURE_ADDRESS_BORDER, \
        addressW = TEXTURE_ADDRESS_BORDER, \
        borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, \
        filter = FILTER_MIN_MAG_MIP_LINEAR)"

// Sharpening pass root signature
#define SharpenPassRS " \
    CBV(b0), \
    DescriptorTable(\
        SRV(t0, numDescriptors = 1)), \
    DescriptorTable(\
        UAV(u0, numDescriptors = 1))"

// Zoom border root signature
#define ZoomBorderPassRS "\
    RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), \
    CBV(b0)"

// Magnifying glass pass root signature
#define MagnifyingGlassPassRS "\
    RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), \
    CBV(b0), \
    DescriptorTable( \
        SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL), \
    StaticSampler(s0, \
        filter = FILTER_MIN_MAG_MIP_POINT, \
        visibility = SHADER_VISIBILITY_PIXEL)" 
