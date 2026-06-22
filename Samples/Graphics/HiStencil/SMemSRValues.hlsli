//--------------------------------------------------------------------------------------
// SMemSRValues.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Decode HTile values for SR0/1 and SMem
// SMem	  Description
// 0	  Clear - The entire tile has the Clear value.
// 1	  Single Value - Entire tile has a single stencil value. 1st 8-bits of data 
//        in the stencil buffer tile is the value for entire tile
// 2	  Expanded and Clear - Unused currently on XBox. All the samples for this tile 
//        in the stencil buffer have the clear value.
// 3	  Expanded - The tile has been expanded, so the individual samples in the 
//        stencil buffer have the correct stencil value
//
// SR*	Description 
//  0	Cleared or not compared
//  1	May Fail - At least one sample has failed HiStencil test
//  2	May Pass- At least one sample has passed HiStencil test
//  3	May Pass or May Fail.At least one sample passed HiStencil test and 
//      at least one sample failed HiStencil
//
//--------------------------------------------------------------------------------------

// Interpret SR* values from HTile
#define STENCIL_CLEAR 0x0
#define STENCIL_MAY_FAIL 0x1
#define STENCIL_MAY_PASS 0x2
#define STENCIL_MAY_PASS_OR_FAIL 0x3

// Interpret SMem values from HTile
#define SMEM_CLEAR 0x0
#define SMEM_SINGLE_VALUE 0x1
#define SMEM_EXPANDED_AND_CLEAR 0x2
#define SMEM_EXPANDED 0x3
