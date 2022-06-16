//--------------------------------------------------------------------------------------
// GlobalRootSignature.hlsl
//
// The Global Root Signature for our Raytracing Pipeline State Object
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define GlobalRootSignature  "RootFlags(0), SRV(t0), RootConstants(b0, num32bitconstants=4), DescriptorTable(UAV(u0))"
