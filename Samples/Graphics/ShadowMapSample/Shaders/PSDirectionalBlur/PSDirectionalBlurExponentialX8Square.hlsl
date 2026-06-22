//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define g_iBlurKernelSize 8
#define CalculateBlurWeights CalculateWeightsSquare
#define g_iOffsetX 1
#define BlurInputType float
#define BlurOutputType float
#define RemappingFunction RemappingFunctionExponential

#include "PSDirectionalBlur.hlsli"