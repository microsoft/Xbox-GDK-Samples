#pragma once
#if defined(PLAYFAB_PLATFORM_XBOX)
//------------------------------------------------------------------------------
// HttpRequest.h
//
// An example use of IXMLHTTPRequest2Callback interface presented
// as a simplified HTTP request object.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------


#include <objbase.h>
#include <windows.h>
#include <string>
#include <memory>
#include <winapifamily.h>
#include <vector>

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
#include <xdk.h>
#endif

#include <wrl.h>

#include <ctime>
#include <chrono>

#include <ixmlhttprequest2.h>
#include <wrl.h>

namespace PlayFab
{
    using namespace Microsoft::WRL;

    // Used for setting headers
    struct HttpHeaderInfo
    {
        std::wstring wstrHeaderName;
        std::wstring wstrHeaderValue;
    };

    //------------------------------------------------------------------------------
    // Name: HttpCallback
    // Desc: Implement the IXMLHTTPRequest2Callback functions for our sample with
    //       basic error reporting and an Event signalling when the request is
    //       complete.
    //------------------------------------------------------------------------------
    class HttpCallback : public Microsoft::WRL::RuntimeClass<RuntimeClassFlags<ClassicCom>, IXMLHTTPRequest2Callback>
    {
    public:
        // Required functions
        STDMETHODIMP OnRedirect(IXMLHTTPRequest2 *pXHR, const WCHAR *pwszRedirectUrl);
        STDMETHODIMP OnHeadersAvailable(IXMLHTTPRequest2 *pXHR, DWORD dwStatus, const WCHAR *pwszStatus);
        STDMETHODIMP OnDataAvailable(IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream);
        STDMETHODIMP OnResponseReceived(IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream);
        STDMETHODIMP OnError(IXMLHTTPRequest2 *pXHR, HRESULT hrError);

        // Helper functions
        HRESULT ReadDataFromStream(ISequentialStream *pStream);
        const BOOL      IsFinished();
        const BOOL      WaitForFinish();
        const std::wstring& GetHeaders()  const { return m_headers; };
        const std::wstring& GetData()     const { return m_data; };
        const std::wstring& GetResponseRequestId()  const { return m_responseRequestId; };
        HRESULT   GetHR()           const { return m_hr; };
        DWORD     GetHTTPStatus()   const { return m_httpStatus; };

        HttpCallback() : m_hr(S_OK), m_httpStatus(0), m_hComplete(nullptr), m_headers(), m_data() {}
        ~HttpCallback();

        STDMETHODIMP RuntimeClassInitialize();

    private:
        HANDLE       m_hComplete;
        HRESULT      m_hr;
        DWORD        m_httpStatus;
        std::wstring m_headers;
        std::wstring m_data;
        std::wstring m_responseRequestId;
    };

    // ----------------------------------------------------------------------------
    // Name: RequestStream
    // Desc: Encapsulates a request data stream. It inherits ISequentialStream,
    // which the IXMLHTTPRequest2 class uses to read from our buffer. It also 
    // inherits IDispatch, which the IXMLHTTPRequest2 interface on Xbox One requires 
    // (unlike on Windows, where only ISequentialStream is necessary).
    // ----------------------------------------------------------------------------
    class RequestStream : public Microsoft::WRL::RuntimeClass<RuntimeClassFlags<ClassicCom>, ISequentialStream, IDispatch>
    {
    public:

        RequestStream();
        ~RequestStream();

        // ISequentialStream
        STDMETHODIMP Open(LPCSTR psBuffer, ULONG cbBufferSize);
        STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
        STDMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten);

        //Helper
        const STDMETHODIMP_(ULONGLONG) Size();

        //IUnknown
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);

        //IDispatch
        STDMETHODIMP GetTypeInfoCount(unsigned int FAR*  pctinfo);
        STDMETHODIMP GetTypeInfo(unsigned int  iTInfo, LCID  lcid, ITypeInfo FAR* FAR*  ppTInfo);
        STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgDispId);
        STDMETHODIMP Invoke(
            DISPID  dispIdMember,
            REFIID  riid,
            LCID  lcid,
            WORD  wFlags,
            DISPPARAMS FAR*  pDispParams,
            VARIANT FAR*  pVarResult,
            EXCEPINFO FAR*  pExcepInfo,
            unsigned int FAR*  puArgErr
        );

    private:
        LONG    m_cRef;
        CHAR*   m_buffer;
        size_t  m_buffSize;
        size_t  m_buffSeekIndex;
    };

    // ----------------------------------------------------------------------------
    // Name: HttpRequest
    // Desc: Encapsulates a single HTTP request.
    // ----------------------------------------------------------------------------
    class HttpRequest
    {
    public:
        HttpRequest();
        ~HttpRequest();

        //send
        HRESULT Open(const std::wstring& verb, const std::wstring& url, const std::vector<HttpHeaderInfo>& headers, const std::string& data);
        HRESULT Open(const std::wstring& verb, const std::wstring& url, const std::vector<HttpHeaderInfo>& headers, const uint8_t* data, const size_t datalength);

        DWORD   WaitForFinish();
        BOOL    IsFinished();
        HRESULT GetResult() { return m_pHttpCallback->GetHR(); }
        DWORD   GetStatus() { return m_pHttpCallback->GetHTTPStatus(); }
        void    Cleanup();

        const std::wstring& GetHeaders() { return m_pHttpCallback->GetHeaders(); }
        const std::wstring& GetData() { return m_pHttpCallback->GetData(); }
        const std::wstring& GetResponseRequestId() { return m_pHttpCallback->GetResponseRequestId(); }

    private:
        HRESULT CreateRequest(const std::wstring& verb, const std::wstring& url);
        HRESULT SetHeaders(HttpHeaderInfo HeadersToAdd[], INT iHeaderCount, BOOL bAddGuid = FALSE, BOOL ignoreCacheAndETag = FALSE);
        HRESULT SendRequest(char* data);

        ComPtr<IXMLHTTPRequest2>         m_pXHR;
        ComPtr<IXMLHTTPRequest2Callback> m_pXHRCallback;
        ComPtr<HttpCallback>             m_pHttpCallback;
        ComPtr<RequestStream>            m_requestStream;
    };

}  // namespace PlayFab
#endif // defined(PLAYFAB_PLATFORM_XBOX)