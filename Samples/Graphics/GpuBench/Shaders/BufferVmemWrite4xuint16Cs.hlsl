//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "BufferFormat.hlsli"

#define ELEMENT_BYTES 8
#define NUM_FORMAT NUM_FORMAT_UINT
#define DATA_FORMAT DATA_FORMAT_16_16_16_16
#define STORE_OP XB_TypedStore4
#define CHANNEL_MASK xyzw

#include "BufferVmemWriteCs.hlsli"