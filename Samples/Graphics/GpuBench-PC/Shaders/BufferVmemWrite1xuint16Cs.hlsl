//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "BufferFormat.hlsli"

#define ELEMENT_BYTES 2
#define NUM_FORMAT NUM_FORMAT_UINT
#define DATA_FORMAT DATA_FORMAT_16
#define STORE_OP XB_TypedStore
#define CHANNEL_MASK x

#include "BufferVmemWriteCs.hlsli"