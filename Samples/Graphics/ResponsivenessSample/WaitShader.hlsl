//--------------------------------------------------------------------------------------
// WaitShader.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

RWStructuredBuffer<uint> buffer : register(u0);

[RootSignature("UAV(u0)")]
[NumThreads(64, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)
{
	float x = asfloat(dispatchThreadId);

	[unroll]
	for (uint i = 0; i < 1000; i++) {
		x = x * x + x;
	}

    buffer[0] = x;
}
