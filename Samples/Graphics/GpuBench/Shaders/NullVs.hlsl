//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifdef __XBOX_SCARLETT
#define __XBOX_PRECOMPILE_VS_PS 0
#define __XBOX_PRECOMPILE_VS_GS 1
#endif

// Empty vertex shader, for geometry-shader-only rendering
[ROOT_SIGNATURE]
void main() {}
