//--------------------------------------------------------------------------------------
// Json.h
//
// Header to include the JSON serializer/deserializer found @ https://github.com/nlohmann/json
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wnan-infinity-disabled"
#endif

#pragma warning(push)
#pragma warning(disable : 4061)
#pragma warning(disable : 26451)
#pragma warning(disable : 26819) // Unannotated fallthrough between switch labels
#pragma warning(disable : 26495) // Variable is uninitialized. Always initialize a member variable

#include "json/json.hpp"

using json = nlohmann::json;

#pragma warning(pop)

#ifdef __clang__
#pragma clang diagnostic pop
#endif
