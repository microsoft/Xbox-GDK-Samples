//--------------------------------------------------------------------------------------
// RootSignature.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Root Signature used by non-PBR shaders
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE_MAIN \
    RootSignature\
    (\
       "RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | \
                    DENY_DOMAIN_SHADER_ROOT_ACCESS | \
                    DENY_GEOMETRY_SHADER_ROOT_ACCESS | \
                    DENY_HULL_SHADER_ROOT_ACCESS ), \
        DescriptorTable (SRV(t0, numDescriptors=32)), \
        DescriptorTable (UAV(u0, numDescriptors=8)), \
        CBV(b0), \
        SRV(t32), \
        StaticSampler(s0)" \
    )
