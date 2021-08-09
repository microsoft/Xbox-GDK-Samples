//--------------------------------------------------------------------------------------
// PassVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// This VS will be followed by GS or HS and not PS 
#ifdef __XBOX_SCARLETT
#define __XBOX_PRECOMPILE_VS_GS 1
#define __XBOX_PRECOMPILE_VS_HS 1
#define __XBOX_PRECOMPILE_VS_PS 0
#endif

#include "Shared.hlsli"

// forwarding vertex shader
[RootSignature(CommonRS)]
VSOutGSIn main(VSIn vin)
{
    VSOutGSIn vout;
    vout.posSize = vin.posSize;
    vout.clr = vin.clr;

    return vout;
}
