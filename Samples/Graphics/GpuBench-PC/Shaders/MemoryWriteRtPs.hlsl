//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

[ROOT_SIGNATURE]
float4 main() : SV_Target0
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}