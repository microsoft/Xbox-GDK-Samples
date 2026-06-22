//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define g_iBlurKernelSize 4
#define CalculateBlurWeights CalculateWeightsGaussian
#define g_iOffsetX 1
#define g_bUseBilinearFiltering true
#define BlurInputType float2
#define BlurOutputType float2

#include "PSDirectionalBlur.hlsli"
