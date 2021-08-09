// Copyright (C) Microsoft Corporation. All rights reserved.
//
// This header file is used to include platform-specific headers of JsonCpp library.

#pragma once

#include <playfab/PlayFabPlatformMacros.h>

#if defined(PLAYFAB_PLATFORM_SWITCH)
#include <json/json.h>
#include <json/json-forwards.h>
#elif defined(PLAYFAB_PLATFORM_LINUX)
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#else
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#endif
