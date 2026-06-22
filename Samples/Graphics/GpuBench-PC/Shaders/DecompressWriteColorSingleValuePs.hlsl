//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

[ROOT_SIGNATURE]
void main(float4 position : SV_Position, out uint4 color : SV_Target)
{
    color = float4(1, 2, 3, 4);
}
