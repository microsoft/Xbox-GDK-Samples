//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define CalcUnshadowedAmount CalcUnshadowedAmountPCFSampleCmpStep1
#define g_iBlurKernelSize 4
#define CalculateBlurWeights CalculateWeightsSquare

#include "PSLitWithShadow.hlsli"