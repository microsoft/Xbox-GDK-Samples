//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define CalcUnshadowedAmount CalcUnshadowedAmountPCFSampleCmpStep2
#define g_iBlurKernelSize 4
#define CalculateBlurWeights CalculateWeightsGaussian

#include "PSLitWithShadow.hlsli"