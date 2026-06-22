//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define D3D12_SHADING_RATE_X_AXIS_SHIFT 2
#define D3D12_SHADING_RATE_VALID_MASK 3

#define D3D12_MAKE_COARSE_SHADING_RATE(x,y) ((x) << D3D12_SHADING_RATE_X_AXIS_SHIFT | (y))

