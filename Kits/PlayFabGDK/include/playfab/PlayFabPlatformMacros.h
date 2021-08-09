// Copyright (C) Microsoft Corporation. All rights reserved.
//
// This header file is used to define PLAYFAB_PLATFORM macros.
// Any platform supported by the XPlatCppSdk must be added and redefined here.
//
// Format used : PLAYFAB_PLATFORM_<PlatformName>
//
// In the PlayFab codebase, this newly defined macro will be used.

#pragma once

// What is the appropriate label here? I'm not seeing any trigger on the install,
// THESE ARE DEFINED BY THE VCXPROJ, MAKE SURE YOU HAVE IT THERE FIRST.
// TEST THIS BECAUSE THERE IS NO SOLID (non-theoretical) ANSWER:
//      Will there be any issues if we don't have PLAYFAB_PLATFORM_WINDOWS enabled as well as GDK defines?
#define PLAYFAB_PLATFORM_GDK
//#ifdef _GAMING_XBOX_SCARLETT || _GAMING_XBOX || _GAMING_DESKTOP
//#define PLAYFAB_PLATFORM_GDK
//#endif // GDK

#ifdef _DURANGO
#define PLAYFAB_PLATFORM_XBOX
#endif // _DURANGO

#if defined(__linux__) && !defined(__ANDROID__)
#define PLAYFAB_PLATFORM_LINUX
#endif // __linux__ && !__ANDROID__

#ifdef __APPLE__
#define PLAYFAB_PLATFORM_IOS
#endif // __APPLE__

#ifdef __ANDROID__
#define PLAYFAB_PLATFORM_ANDROID
#endif // __ANDROID__

// Durango is also defined as _WIN32.
// Hence to specify only Windows, we have check for ! _DURANGO.
#if defined(_WIN32) && !defined(_DURANGO)
#define PLAYFAB_PLATFORM_WINDOWS
#endif //_WIN32