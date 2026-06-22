//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define CalcUnshadowedAmount CalcUnshadowedAmountPCFSampleCmpStep1
#define g_iBlurKernelSize 6
#define CalculateBlurWeights CalculateWeightsGaussian

#include "PSLitWithShadow.hlsli"