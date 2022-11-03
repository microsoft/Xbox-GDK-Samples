//-----------------------------------------------------------------------------
// Debug.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#define DEBUG_LOGGING
#define DEBUG_LOG_CREATE_NEW_ON_LAUNCH          1
#define DEBUG_LOG_FILENAME                      "SimpleMPADebugLog-"
#define DEBUG_LOG_ENTRY_PREFIX                  "SimpleMPA: "

void DebugInit();
std::string DebugWrite(const char* format, ...);

#define DEBUGLOG(x, ...)    DebugWrite(x, __VA_ARGS__)

#define STRINGIFY(x) #x

void LogError_HRESULT(const char* functionName, HRESULT hr);
void LogError_HRESULTWithMessage(const char* functionName, HRESULT hr, const char* errorMessage);
void LogError_ErrorWithMessage(const char* functionName, uint32_t error, const char* errorMessage);
