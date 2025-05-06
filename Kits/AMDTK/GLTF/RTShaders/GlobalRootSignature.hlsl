//--------------------------------------------------------------------------------------
// GlobalRootSignature.hlsl
//
// The Global Root Signature for our Raytracing Pipeline State Object
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define GlobalRootSignature  "RootFlags(0), SRV(t0), DescriptorTable(SRV(t1)),DescriptorTable( SRV(t2)),DescriptorTable( SRV(t3)),DescriptorTable( SRV(t4)), CBV(b0), DescriptorTable(UAV(u0))"
