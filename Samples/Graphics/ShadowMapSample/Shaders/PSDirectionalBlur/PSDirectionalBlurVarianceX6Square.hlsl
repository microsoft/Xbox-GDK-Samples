//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define g_iBlurKernelSize 6
#define CalculateBlurWeights CalculateWeightsSquare
#define g_iOffsetX 1
#define BlurInputType float
#define BlurOutputType float2
#define RemappingFunction RemappingFunctionVariance

#include "PSDirectionalBlur.hlsli"