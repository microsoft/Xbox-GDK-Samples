//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define CalcUnshadowedAmount CalcUnshadowedAmountPCFSampleCmpStep2
#define g_iBlurKernelSize 8
#define CalculateBlurWeights CalculateWeightsSquare

#include "PSLitWithShadow.hlsli"