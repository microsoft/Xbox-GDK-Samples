//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define g_iBlurKernelSize 1
#define CalculateBlurWeights CalculateWeightsSquare
#define g_iOffsetX 1
#define BlurInputType float
#define BlurOutputType float4
#define RemappingFunction RemappingFunctionExponentialVariance

#include "PSDirectionalBlur.hlsli"