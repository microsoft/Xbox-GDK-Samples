//--------------------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------------------

#define USE_16BIT_TYPES 0               /**< Don't use 16bit types */
#define USE_TWO_PIXELS_PER_THREAD 1     /**< Process two pixels per thread */
#define USE_MANUAL_PIXEL_PACKING 1      /**< Pack two floats into float2 manually. Assume slightly 
                                             different codegen. Compare codegen with variant without 
                                             packing */

#include "Scattering.hlsli"
