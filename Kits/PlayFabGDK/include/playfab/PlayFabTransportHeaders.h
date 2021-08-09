// Copyright (C) Microsoft Corporation. All rights reserved.
//
// This header file is used to include headers of transport plugins supported on each platform.

#pragma once

#include <playfab/PlayFabPlatformMacros.h>

#ifdef PLAYFAB_PLATFORM_GDK
#include <playfab/PlayFabCurlHttpPlugin.h>
#endif // PLAYFAB_PLATFORM_GDK

#ifdef PLAYFAB_PLATFORM_XBOX
#include <playfab/PlayFabIXHR2HttpPlugin.h>
#endif // PLAYFAB_PLATFORM_XBOX

#ifndef PLAYFAB_PLATFORM_GDK
#ifdef PLAYFAB_PLATFORM_WINDOWS
#include <playfab/PlayFabWinHttpPlugin.h>
#endif // PLAYFAB_PLATFORM_WINDOWS
#endif // !PLAYFAB_PLATFORM_GDK

#ifdef PLAYFAB_PLATFORM_LINUX
#include <playfab/PlayFabCurlHttpPlugin.h>
#endif // PLAYFAB_PLATFORM_LINUX

#ifdef PLAYFAB_PLATFORM_IOS
#include <playfab/PlayFabIOSHttpPlugin.h>
#endif // PLAYFAB_PLATFORM_IOS

#ifdef PLAYFAB_PLATFORM_ANDROID
#include <playfab/PlayFabAndroidHttpPlugin.h>
#endif // PLAYFAB_PLATFORM_ANDROID
