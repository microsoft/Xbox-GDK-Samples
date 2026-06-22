//--------------------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------------------

#define USE_16BIT_TYPES 0               /**< Don't use 16bit types */
#define USE_TWO_PIXELS_PER_THREAD 0     /**< Process single pixel per thread */
#define USE_MANUAL_PIXEL_PACKING 0      /**< Don't use packing option because only single pixel is processed */

#include "Scattering.hlsli"
