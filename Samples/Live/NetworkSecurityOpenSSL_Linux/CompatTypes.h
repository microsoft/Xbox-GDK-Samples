#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>

typedef int32_t HRESULT;

#define CALLBACK

#ifndef __cdecl
#define __cdecl 
#endif

#ifndef __stdcal
#define __stdcall
#endif

#ifndef __forceinline 
#define __forceinline inline
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#endif

#ifndef HANDLE
typedef void* HANDLE;
#endif

#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1

#define SUCCEEDED(hr)           (((HRESULT)(hr)) >= 0)
#define FAILED(hr)              (((HRESULT)(hr)) < 0)
#define SOCKET_ERROR            (-1)
#define HRESULT_CODE(hr)        ((hr) & 0xFFFF)
#define SCODE_CODE(sc)          ((sc) & 0xFFFF)

#define HRESULT_FACILITY(hr)    (((hr) >> 16) & 0x1fff)
#define SCODE_FACILITY(sc)      (((sc) >> 16) & 0x1fff)

#define HRESULT_SEVERITY(hr)    (((hr) >> 31) & 0x1)
#define SCODE_SEVERITY(sc)      (((sc) >> 31) & 0x1)

#define MAKE_HRESULT(sev,fac,code) \
        ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
#define MAKE_SCODE(sev,fac,code) \
        ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )

#define FACILITY_WIN32                   7
#define FACILITY_INTERNET                12
#define FACILITY_HTTP                    25

#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
#define _HRESULTYPEDEF_(_sc) ((HRESULT)_sc)
#define __HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))

#define S_OK                                    ((HRESULT)0L)
#define E_NOTIMPL                               _HRESULTYPEDEF_(0x80004001L)
#define E_OUTOFMEMORY                           _HRESULTYPEDEF_(0x8007000EL)
#define E_INVALIDARG                            _HRESULTYPEDEF_(0x80070057L)
#define E_ABORT                                 _HRESULTYPEDEF_(0x80004004L)
#define E_FAIL                                  _HRESULTYPEDEF_(0x80004005L)
#define E_ACCESSDENIED                          _HRESULTYPEDEF_(0x80070005L)
#define E_PENDING                               _HRESULTYPEDEF_(0x8000000AL)
#define E_UNEXPECTED                            _HRESULTYPEDEF_(0x8000FFFFL)
#define E_POINTER                               _HRESULTYPEDEF_(0x80004003L)
#define E_TIME_CRITICAL_THREAD                  _HRESULTYPEDEF_(0x800701A0L)
#define E_NO_TASK_QUEUE                         _HRESULTYPEDEF_(0x800701ABL)
#define E_NOT_SUPPORTED                         _HRESULTYPEDEF_(0x80070032L)
#define E_NOT_SUFFICIENT_BUFFER                 _HRESULTYPEDEF_(0x8007007AL)
#define E_NOINTERFACE                           _HRESULTYPEDEF_(0x80004002L)
#define E_BOUNDS                                _HRESULTYPEDEF_(0x8000000BL)
#define E_ILLEGAL_METHOD_CALL                   _HRESULTYPEDEF_(0x8000000EL)
#define HTTP_E_STATUS_AMBIGUOUS                 _HRESULTYPEDEF_(0x8019012CL)
#define HTTP_E_STATUS_BAD_GATEWAY               _HRESULTYPEDEF_(0x801901F6L)
#define HTTP_E_STATUS_BAD_METHOD                _HRESULTYPEDEF_(0x80190195L)
#define HTTP_E_STATUS_BAD_REQUEST               _HRESULTYPEDEF_(0x80190190L)
#define HTTP_E_STATUS_CONFLICT                  _HRESULTYPEDEF_(0x80190199L)
#define HTTP_E_STATUS_DENIED                    _HRESULTYPEDEF_(0x80190191L)
#define HTTP_E_STATUS_EXPECTATION_FAILED        _HRESULTYPEDEF_(0x801901A1L)
#define HTTP_E_STATUS_429_TOO_MANY_REQUESTS     _HRESULTYPEDEF_(0x801901ADL)
#define HTTP_E_STATUS_FORBIDDEN                 _HRESULTYPEDEF_(0x80190193L)
#define HTTP_E_STATUS_GATEWAY_TIMEOUT           _HRESULTYPEDEF_(0x801901F8L)
#define HTTP_E_STATUS_GONE                      _HRESULTYPEDEF_(0x8019019AL)
#define HTTP_E_STATUS_LENGTH_REQUIRED           _HRESULTYPEDEF_(0x8019019BL)
#define HTTP_E_STATUS_MOVED                     _HRESULTYPEDEF_(0x8019012DL)
#define HTTP_E_STATUS_NONE_ACCEPTABLE           _HRESULTYPEDEF_(0x80190196L)
#define HTTP_E_STATUS_NOT_FOUND                 _HRESULTYPEDEF_(0x80190194L)
#define HTTP_E_STATUS_NOT_MODIFIED              _HRESULTYPEDEF_(0x80190130L)
#define HTTP_E_STATUS_NOT_SUPPORTED             _HRESULTYPEDEF_(0x801901F5L)
#define HTTP_E_STATUS_PAYMENT_REQ               _HRESULTYPEDEF_(0x80190192L)
#define HTTP_E_STATUS_PRECOND_FAILED            _HRESULTYPEDEF_(0x8019019CL)
#define HTTP_E_STATUS_PROXY_AUTH_REQ            _HRESULTYPEDEF_(0x80190197L)
#define HTTP_E_STATUS_RANGE_NOT_SATISFIABLE     _HRESULTYPEDEF_(0x801901A0L)
#define HTTP_E_STATUS_REDIRECT                  _HRESULTYPEDEF_(0x8019012EL)
#define HTTP_E_STATUS_REDIRECT_KEEP_VERB        _HRESULTYPEDEF_(0x80190133L)
#define HTTP_E_STATUS_REDIRECT_METHOD           _HRESULTYPEDEF_(0x8019012FL)
#define HTTP_E_STATUS_REQUEST_TIMEOUT           _HRESULTYPEDEF_(0x80190198L)
#define HTTP_E_STATUS_REQUEST_TOO_LARGE         _HRESULTYPEDEF_(0x8019019DL)
#define HTTP_E_STATUS_SERVER_ERROR              _HRESULTYPEDEF_(0x801901F4L)
#define HTTP_E_STATUS_SERVICE_UNAVAIL           _HRESULTYPEDEF_(0x801901F7L)
#define HTTP_E_STATUS_UNEXPECTED                _HRESULTYPEDEF_(0x80190001L)
#define HTTP_E_STATUS_UNEXPECTED_SERVER_ERROR   _HRESULTYPEDEF_(0x80190005L)
#define HTTP_E_STATUS_UNSUPPORTED_MEDIA         _HRESULTYPEDEF_(0x8019019FL)
#define HTTP_E_STATUS_URI_TOO_LONG              _HRESULTYPEDEF_(0x8019019EL)
#define HTTP_E_STATUS_USE_PROXY                 _HRESULTYPEDEF_(0x80190131L)
#define HTTP_E_STATUS_VERSION_NOT_SUP           _HRESULTYPEDEF_(0x801901F9L)
#define ONL_E_ACTION_REQUIRED                   _HRESULTYPEDEF_(0x8086000CL)
#define WEB_E_INVALID_JSON_STRING               _HRESULTYPEDEF_(0x83750007L)
#define WEB_E_UNEXPECTED_CONTENT                _HRESULTYPEDEF_(0x83750005L)

#define ERROR_ARITHMETIC_OVERFLOW               534L
#define ERROR_BAD_CONFIGURATION                 1610L
#define ERROR_BAD_LENGTH                        24L
#define ERROR_CANCELLED                         1223L
#define ERROR_NO_SUCH_USER                      1317L
#define ERROR_RESOURCE_DATA_NOT_FOUND           1812L

typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY  *Flink;
    struct _LIST_ENTRY  *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

#define ZeroMemory(Destination,Length) memset((Destination),0,(Length))

//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//

#define CONTAINING_RECORD(address, type, field) \
        ((type *)((char*)(address) - (uintptr_t)(&((type *)0)->field)))

#ifndef _Field_size_
#define _Field_size_(bytes) 
#endif

#ifndef _Field_size_bytes_
#define _Field_size_bytes_(bytes) 
#endif

#ifndef _Field_size_bytes_opt_
#define _Field_size_bytes_opt_(bytes) 
#endif

#ifndef _Field_size_opt_
#define _Field_size_opt_(bytes)
#endif

#ifndef _Field_z_
#define _Field_z_ 
#endif

#ifndef _In_
#define _In_
#endif

#ifndef _In_opt_
#define _In_opt_ 
#endif

#ifndef _In_opt_z_
#define _In_opt_z_ 
#endif

#ifndef _In_reads_
#define _In_reads_(size) 
#endif

#ifndef _In_reads_opt_
#define _In_reads_opt_(size) 
#endif

#ifndef _In_reads_bytes_
#define _In_reads_bytes_(size) 
#endif

#ifndef _In_reads_bytes_opt_
#define _In_reads_bytes_opt_(size) 
#endif

#ifndef _In_reads_z_
#define _In_reads_z_(size) 
#endif

#ifndef _In_z_
#define _In_z_ 
#endif

#ifndef _Inout_
#define _Inout_ 
#endif

#ifndef _Inout_updates_bytes_
#define _Inout_updates_bytes_(size)
#endif

#ifndef _Null_terminated_
#define _Null_terminated_ 
#endif

#ifndef _Out_
#define _Out_ 
#endif

#ifndef _Out_opt_
#define _Out_opt_ 
#endif

#ifndef _Out_range_
#define _Out_range_(x, y)  
#endif

#ifndef _Out_writes_
#define _Out_writes_(bytes)
#endif

#ifndef _Out_writes_bytes_
#define _Out_writes_bytes_(bytes)
#endif

#ifndef _Out_writes_bytes_opt_
#define _Out_writes_bytes_opt_(size)
#endif

#ifndef _Out_writes_bytes_to_
#define _Out_writes_bytes_to_(size, buffer)
#endif

#ifndef _Out_writes_bytes_to_opt_
#define _Out_writes_bytes_to_opt_(size, buffer)
#endif

#ifndef _Out_writes_to_
#define _Out_writes_to_(bytes, buffer)
#endif

#ifndef _Out_writes_to_opt_
#define _Out_writes_to_opt_(buffersize, size)
#endif

#ifndef _Out_writes_z_
#define _Out_writes_z_(bytes)
#endif

#ifndef _Outptr_
#define _Outptr_ 
#endif

#ifndef _Outptr_result_maybenull_
#define _Outptr_result_maybenull_
#endif

#ifndef _Outptr_result_bytebuffer_maybenull_
#define _Outptr_result_bytebuffer_maybenull_(size)
#endif

#ifndef _Post_invalid_
#define _Post_invalid_ 
#endif

#ifndef _Post_writable_byte_size_
#define _Post_writable_byte_size_(X)
#endif

#ifndef _Printf_format_string_
#define _Printf_format_string_ 
#endif

#ifndef _Ret_maybenull_
#define _Ret_maybenull_
#endif

#ifndef _Ret_z_
#define _Ret_z_
#endif

#ifndef _Deref_out_opt_
#define _Deref_out_opt_
#endif

#ifndef __analysis_assume
#define __analysis_assume(condition)
#endif

#ifndef STDAPIVCALLTYPE
#define STDAPIVCALLTYPE         __cdecl
#endif

#ifndef STDAPI
#define STDAPI                  EXTERN_C HRESULT STDAPIVCALLTYPE
#endif

#ifndef STDAPI_
#define STDAPI_(type)           EXTERN_C type STDAPIVCALLTYPE
#endif

#ifndef DEFINE_ENUM_FLAG_OPERATORS
    #ifdef __cplusplus

    extern "C++" {

        template <std::size_t S>
        struct _ENUM_FLAG_INTEGER_FOR_SIZE;

        template <>
        struct _ENUM_FLAG_INTEGER_FOR_SIZE<1>
        {
            typedef int8_t type;
        };

        template <>
        struct _ENUM_FLAG_INTEGER_FOR_SIZE<2>
        {
            typedef int16_t type;
        };

        template <>
        struct _ENUM_FLAG_INTEGER_FOR_SIZE<4>
        {
            typedef int32_t type;
        };

        template <>
        struct _ENUM_FLAG_INTEGER_FOR_SIZE<8>
        {
            typedef int64_t type;
        };

        // used as an approximation of std::underlying_type<T>
        template <class T>
        struct _ENUM_FLAG_SIZED_INTEGER
        {
            typedef typename _ENUM_FLAG_INTEGER_FOR_SIZE<sizeof(T)>::type type;
        };

    }

    #define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
    extern "C++" { \
    inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) throw() { return ENUMTYPE(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) | ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
    inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) throw() { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) |= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
    inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) throw() { return ENUMTYPE(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) & ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
    inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) throw() { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) &= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
    inline ENUMTYPE operator ~ (ENUMTYPE a) throw() { return ENUMTYPE(~((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a)); } \
    inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) throw() { return ENUMTYPE(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) ^ ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
    inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) throw() { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) ^= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
    }
    #else
    #define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) // NOP, C allows these operators.
    #endif
#endif // DEFINE_ENUM_FLAG_OPERATORS

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

#define LOG_IF_FAILED(hr)                                       do { HRESULT __hrRet = hr; if (FAILED(__hrRet)) { HC_TRACE_ERROR(HTTPCLIENT, "%s: 0x%08", #hr, __hrRet); }} while (0, 0)

#define FAIL_FAST_MSG(fmt, ...)                        \
    ASSERT(false);                                     \


#define FAIL_FAST_IF_FAILED(hr)                                 do { HRESULT __hrRet = hr; if (FAILED(__hrRet)) { FAIL_FAST_MSG("%s 0x%08", #hr, __hrRet); }} while (0, 0)

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) sizeof(x) / sizeof(x[0])
#endif

#ifndef _WIN32
#define UNREFERENCED_PARAMETER(args) (void)(args);
#endif

#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif

#ifndef CONST
#define CONST const
#endif

#ifndef MAXUINT
#define MAXUINT UINT_MAX
#endif

typedef unsigned char BYTE;
typedef unsigned long DWORD;
typedef void *PVOID;
typedef void VOID;

typedef BYTE BOOLEAN;
typedef BYTE *PBYTE;
typedef PVOID HANDLE;

#define INVALID_SOCKET (-1)

struct OVERLAPPED {
    uint64_t Internal;
    uint64_t InternalHigh;
    union {
        struct {
            uint32_t Offset;
            uint32_t OffsetHigh;
        } DUMMYSTRUCTNAME;
        PVOID Pointer;
    } DUMMYUNIONNAME;

    HANDLE  hEvent;
};

#define HRESULT_FROM_WIN32(x) __HRESULT_FROM_WIN32(x)

#define CERT_E_EXPIRED                   _HRESULT_TYPEDEF_(0x800B0101L)
#define TRUST_E_SUBJECT_NOT_TRUSTED      _HRESULT_TYPEDEF_(0x800B0004L)

#define NO_ERROR                         0L
#define ERROR_INVALID_DATA               13L
#define ERROR_BUFFER_OVERFLOW            111L
#define ERROR_CONNECTION_UNAVAIL         1201L
#define ERROR_INSUFFICIENT_BUFFER        122L   
#define ERROR_ALREADY_EXISTS             183L
#define ERROR_NOT_FOUND                  1168L
#define ERROR_NO_MATCH                   1169L
#define ERROR_EMPTY                      4306L