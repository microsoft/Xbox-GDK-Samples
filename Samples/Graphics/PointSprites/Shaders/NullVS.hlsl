//--------------------------------------------------------------------------------------
// NullVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// This VS will be followed by GS and not PS 
#ifdef __XBOX_SCARLETT
#define __XBOX_PRECOMPILE_VS_GS 1
#define __XBOX_PRECOMPILE_VS_PS 0
#endif

#include "Shared.hlsli"

[RootSignature(CommonRS)]
VSNull main()
{
    // Use empty data as output which won't be used.
    // Pipelines that utilize on-chip GS memory throw an exceptionally uninformative 
    // exception of 'integer division by zero' during pipeline state creation if the 
    // preceding pipeline stage specifies void or an empty struct as output.
    return (VSNull)0;
}
