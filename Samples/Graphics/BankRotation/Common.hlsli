//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Common stuff used by all shaders
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE_MESH \
    RootSignature\
    (\
       "RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | \
                    DENY_DOMAIN_SHADER_ROOT_ACCESS | \
                    DENY_GEOMETRY_SHADER_ROOT_ACCESS | \
                    DENY_HULL_SHADER_ROOT_ACCESS ), \
        DescriptorTable (SRV(t0, numDescriptors=4)), \
        CBV(b0), \
        DescriptorTable(UAV(u0, numDescriptors = 4)), \
        StaticSampler(s0)" \
    )



