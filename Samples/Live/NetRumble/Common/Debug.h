//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#define DEBUG_LOGGING
#define DEBUG_LOG_CREATE_NEW_ON_LAUNCH          1
#define DEBUG_LOG_FILENAME                      "NetRumbleDebugLog-"
#define DEBUG_LOG_ENTRY_PREFIX                  "NETRUM: "
#define DEBUG_LOG_ENTRY_PREFIX                  "NETRUM: "

void DebugInit();
std::string DebugWrite(const char* format, ...);

#define DEBUGLOG(x, ...)    DebugWrite(x, __VA_ARGS__)
