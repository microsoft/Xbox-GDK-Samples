//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// This VS will be followed by GS and not PS 
#ifdef __XBOX_SCARLETT
#define __XBOX_PRECOMPILE_VS_GS 1
#define __XBOX_PRECOMPILE_VS_PS 0
#endif

// Empty vertex shader, for geometry-shader FSQ pass
#include "Common.hlsli"

[RootSignature(ROOT_SIG)]
InterpolantsPoint main(VertexPoint In)
{
    InterpolantsPoint Out;
    Out.position = In.position;
    return Out;
}
