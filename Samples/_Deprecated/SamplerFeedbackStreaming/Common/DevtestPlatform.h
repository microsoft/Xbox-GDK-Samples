//--------------------------------------------------------------------------------------
// DevtestPlatform.h
//
// Root header file for the devtest platform headers.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// this define prevents defining min and max macros
// so STL headers can be included and std::min and std::max can be used
#if !defined(NOMINMAX)
#define NOMINMAX
#endif

// this define is needed to use math constants defined in cmath, e.g. M_PI
#if !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES
#endif

#if defined(_GAMING_XBOX) || defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_DESKTOP) || XGS_BUILD
#include "Platform_GDK.h"
#elif PC_BUILD
#include "Platform_Win32.h"
#else
#include "Platform_UWP.h"
#endif

#include <unordered_map>
#include <vector>

#include <algorithm>

#include <directxmath.h>
#include <directxpackedvector.h>
#include <DirectXColors.h>
