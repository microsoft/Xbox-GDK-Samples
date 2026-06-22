//--------------------------------------------------------------------------------------
// Common.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Constants for the sample - used by cpp and hlsl files
//--------------------------------------------------------------------------------------

#define NUM_PSO 6
#define VERTEX_ID_MASK 0xFFFFF
#define MAX_DRAWS 0x7FF

#define BUFFER_OFFSET_TOTAL_WAVES_VS 0
#define BUFFER_OFFSET_TOTAL_WAVES_PS (BUFFER_OFFSET_TOTAL_WAVES_VS+1)
#define BUFFER_OFFSET_DIVERGENT_WAVES_PS (BUFFER_OFFSET_TOTAL_WAVES_PS+1)
#define BUFFER_TOTAL_ITEMS (BUFFER_OFFSET_DIVERGENT_WAVES_PS+1)