// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#if defined(PLAYFAB_PLATFORM_WINDOWS) || defined(PLAYFAB_PLATFORM_XBOX)
typedef signed __int64 Int64;
typedef signed __int32 Int32;
typedef signed __int16 Int16;

typedef unsigned __int64 Uint64;
typedef unsigned __int32 Uint32;
typedef unsigned __int16 Uint16;
#elif defined(PLAYFAB_PLATFORM_LINUX) || defined(PLAYFAB_PLATFORM_IOS) || defined(PLAYFAB_PLATFORM_ANDROID) || defined(PLAYFAB_PLATFORM_PLAYSTATION) || defined(PLAYFAB_PLATFORM_SWITCH)
#include <cstdint>

typedef int64_t Int64;
typedef int32_t Int32;
typedef int16_t Int16;

typedef uint64_t Uint64;
typedef uint32_t Uint32;
typedef uint16_t Uint16;
#endif
