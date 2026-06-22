#pragma once
#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <bcrypt.h>
#include <wincrypt.h>

#include <array>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <algorithm>
#include <string_view>
#include <charconv>
#include <memory>
#include <string>
#include <iostream>
#include <cassert>
#include <Objidl.h>

#define _HRESULTYPEDEF_(_sc) ((HRESULT)_sc)
#define E_NOT_SUPPORTED                                         _HRESULTYPEDEF_(0x80070032L)

#define RETURN_HR(hr)                                           return(hr)
#define RETURN_LAST_ERROR()                                     return HRESULT_FROM_WIN32(GetLastError())
#define RETURN_WIN32(win32err)                                  return HRESULT_FROM_WIN32(win32err)

#define RETURN_IF_FAILED(hr)                                    do { HRESULT __hrRet = hr; if (FAILED(__hrRet)) { RETURN_HR(__hrRet); }} while (0, 0)
#define RETURN_IF_WIN32_BOOL_FALSE(win32BOOL)                   do { BOOL __boolRet = win32BOOL; if (!__boolRet) { RETURN_LAST_ERROR(); }} while (0, 0)
#define RETURN_IF_NULL_ALLOC(ptr)                               do { if ((ptr) == nullptr) { RETURN_HR(E_OUTOFMEMORY); }} while (0, 0)
#define RETURN_HR_IF(hr, condition)                             do { if (condition) { RETURN_HR(hr); }} while (0, 0)
#define RETURN_HR_IF_FALSE(hr, condition)                       do { if (!(condition)) { RETURN_HR(hr); }} while (0, 0)
#define RETURN_LAST_ERROR_IF(condition)                         do { if (condition) { RETURN_LAST_ERROR(); }} while (0, 0)
#define RETURN_LAST_ERROR_IF_NULL(ptr)                          do { if ((ptr) == nullptr) { RETURN_LAST_ERROR(); }} while (0, 0)
#define LOG_IF_FAILED(hr)                                       do { HRESULT __hrRet = hr; if (FAILED(__hrRet)) { /* HC_TRACE_ERROR(HTTPCLIENT, "%s: 0x%08", #hr, __hrRet); */ }} while (0, 0)

#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif

#define FAIL_FAST_MSG(fmt, ...)                        \
    ASSERT(false);                                     \

#define FAIL_FAST_IF_FAILED(hr)                                 do { HRESULT __hrRet = hr; if (FAILED(__hrRet)) { FAIL_FAST_MSG("%s 0x%08", #hr, __hrRet); }} while (0, 0)

#include "XAsync.h"
#include "Debug.h"
#include "BCryptHandle.h"

