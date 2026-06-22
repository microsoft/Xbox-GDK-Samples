//-----------------------------------------------------------------------------
// Debug.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include <memory>
#include <string>
#include <system_error>

#define DEBUG_LOGGING
#define DEBUG_LOG_CREATE_NEW_ON_LAUNCH          1
#define DEBUG_LOG_FILENAME                      "SampleDebugLog-"

void DebugInit();
std::string DebugWrite(const char* format, ...);

#define DEBUGLOG(x, ...)    DebugWrite(x, __VA_ARGS__)

#ifdef _DEBUG
    #define Assert(condition) do { if(!(condition)) { __debugbreak(); }} while(false)
#else
    #define Assert(...) (void)(__VA_ARGS__)
#endif

std::string GetErrorMessage(HRESULT hr);

class DtlsException : public std::exception
{
public:
    DtlsException(HRESULT hr, const char* err) :
        std::exception(err),
        m_hr(hr)
    {
        m_err = std::system_category().message(m_hr);
    }

    const char* what() const
    {
        return m_err.c_str();
    }

    HRESULT hr() const
    {
        return m_hr;
    }

private:
    std::string m_err;
    HRESULT m_hr;
};
