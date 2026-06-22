//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef __XBOX_SCARLETT
#define __XBOX_PRECOMPILE_VS_PS 0
#define __XBOX_PRECOMPILE_VS_GS 1
#endif

#define ROOT_SIGNATURE\
    RootSignature\
    (\
        "\
            SRV(t0, visibility=SHADER_VISIBILITY_GEOMETRY),\
            DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),\
            StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL),\
        "\
    )\

// Empty vertex shader, for geometry-shader-only rendering
[ROOT_SIGNATURE]
void main() {}
