//--------------------------------------------------------------------------------------
// RootSignature.hlsli
//
// Common code for resolve using a pixel shader
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// The standard D3D11 root signature, for compatibility
// Omitting unused HS and DS stages, since they would overflow userdata registers 
#define ROOT_SIGNATURE_GRAPHICS \
[\
    RootSignature\
    (\
       "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
        DescriptorTable(CBV(b0, numDescriptors = 14), visibility = SHADER_VISIBILITY_VERTEX),\
        DescriptorTable(Sampler(s0, numDescriptors = 16), visibility = SHADER_VISIBILITY_VERTEX),\
        DescriptorTable(SRV(t0, numDescriptors = 128), visibility = SHADER_VISIBILITY_VERTEX),\
        DescriptorTable(UAV(u0, numDescriptors = 64), visibility = SHADER_VISIBILITY_VERTEX),\
        DescriptorTable(CBV(b0, numDescriptors = 14), visibility = SHADER_VISIBILITY_GEOMETRY),\
        DescriptorTable(Sampler(s0, numDescriptors = 16), visibility = SHADER_VISIBILITY_GEOMETRY),\
        DescriptorTable(SRV(t0, numDescriptors = 128), visibility = SHADER_VISIBILITY_GEOMETRY),\
        DescriptorTable(UAV(u0, numDescriptors = 64), visibility = SHADER_VISIBILITY_GEOMETRY),\
        DescriptorTable(CBV(b0, numDescriptors = 14), visibility = SHADER_VISIBILITY_PIXEL),\
        DescriptorTable(Sampler(s0, numDescriptors = 16), visibility = SHADER_VISIBILITY_PIXEL),\
        DescriptorTable(SRV(t0, numDescriptors = 128), visibility = SHADER_VISIBILITY_PIXEL),\
        DescriptorTable(UAV(u0, numDescriptors = 64), visibility = SHADER_VISIBILITY_PIXEL),"\
    )\
]

#define ROOT_SIGNATURE_COMPUTE \
[\
    RootSignature\
    (\
       "DescriptorTable(CBV(b0, numDescriptors = 14)),\
        DescriptorTable(Sampler(s0, numDescriptors = 16)),\
        DescriptorTable(SRV(t0, numDescriptors = 128)),\
        DescriptorTable(UAV(u0, numDescriptors = 64)),"\
    )\
]

