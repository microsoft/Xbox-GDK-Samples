//--------------------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------------------

#define USE_16BIT_TYPES 1               /**< Enable 16bit types */
#define USE_TWO_PIXELS_PER_THREAD 1     /**< Process two pixels per thread */
#define USE_MANUAL_PIXEL_PACKING 0      /**< Don't pack manually two halfs into half2 to make sure 
                                             the compiler has an opportunity to figure itself identical 
                                             instruction dependency chains and turn them into 16bit 
                                             packed math instruction where possible */
#include "Scattering.hlsli"
