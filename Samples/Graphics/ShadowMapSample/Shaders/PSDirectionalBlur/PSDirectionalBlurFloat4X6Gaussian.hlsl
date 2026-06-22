//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define g_iBlurKernelSize 6
#define CalculateBlurWeights CalculateWeightsGaussian
#define g_iOffsetX 1
#define g_bUseBilinearFiltering true
#define BlurInputType float4
#define BlurOutputType float4

#include "PSDirectionalBlur.hlsli"
