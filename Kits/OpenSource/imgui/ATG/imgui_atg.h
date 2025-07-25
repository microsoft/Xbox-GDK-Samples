//--------------------------------------------------------------------------------------
// imgui_atg.h
//
// ATG-specific ImGui implementations and overrides
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//
// Ensure this file is included after imgui, stdlib, and Windows headers
//

#pragma once

// override stdlib functions to their secure equivalents
#define sscanf sscanf_s
#define strncpy(d,s,c) strncpy_s(d,c+1,s,c)
#define sprintf sprintf_s
#define _snprintf(b,c,f,...) _snprintf_s(b,c,c,f,__VA_ARGS__)
#define _vsnprintf(b,c,f,...) _vsnprintf_s(b,c,c,f,__VA_ARGS__)
