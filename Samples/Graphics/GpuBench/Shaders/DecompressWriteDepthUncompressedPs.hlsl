//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

[ROOT_SIGNATURE]
void main(float4 position : SV_Position, out float depth : SV_Depth)
{
    depth = 0.5f;
}
