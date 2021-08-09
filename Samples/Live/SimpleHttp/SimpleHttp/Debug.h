//-----------------------------------------------------------------------------
// Debug.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#define DEBUG_LOGGING
#define DEBUG_LOG_CREATE_NEW_ON_LAUNCH          1
#define DEBUG_LOG_FILENAME                      "SampleDebugLog-"
#define DEBUG_LOG_ENTRY_PREFIX                  "SIMPLE_HTTP: "

void DebugInit();
std::string DebugWrite(const char* format, ...);

#define DEBUGLOG(x, ...)    DebugWrite(x, __VA_ARGS__)
