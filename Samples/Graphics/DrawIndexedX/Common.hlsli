//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Common stuff used by all shaders
//--------------------------------------------------------------------------------------

#include "Common.h"

#define ROOT_SIGNATURE_MESH \
    RootSignature\
    (\
       "RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | \
                    DENY_DOMAIN_SHADER_ROOT_ACCESS | \
                    DENY_GEOMETRY_SHADER_ROOT_ACCESS | \
                    DENY_HULL_SHADER_ROOT_ACCESS ), \
        DescriptorTable (SRV(t0, space=1, numDescriptors=unbounded)), \
        CBV(b0), \
        SRV(t2), \
        SRV(t3), \
        UAV(u0), \
        StaticSampler(s0)" \
    )



