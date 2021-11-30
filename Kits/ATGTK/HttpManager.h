//--------------------------------------------------------------------------------------
// HttpManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

struct HttpHeader;
struct HttpRequestContext;

#include <XCurl.h>
#include <functional>

class HttpManager
{
public:
    HttpManager();
    ~HttpManager();
    HttpManager(std::function<void(const std::string&)> LogFunction);

    void Update();

    void HttpManagerLog(std::string message);

    HRESULT MakeHttpRequestAsync(XUserHandle user, const std::string& verb, const std::string& uri, const std::vector<HttpHeader>& headers, uint8_t* bodyBytes, size_t bodyLen, std::function<void(HttpRequestContext*)> OnCompleted);
    HRESULT MakeHttpRequestAsync(XUserHandle user, const std::string& verb, const std::string& uri, const std::vector<HttpHeader>& headers, std::string bodyString, size_t bodyLen, std::function<void(HttpRequestContext*)> OnCompleted);

private:
    void Initialize();
    void CleanUp();

    HRESULT BeginAuthorization(HttpRequestContext* context);
    HRESULT SendHttpRequest(HttpRequestContext* context);
    HRESULT CompleteHttpRequest(HttpRequestContext* context);
    HRESULT RetryHttpRequest(HttpRequestContext* context);

    bool m_initialized = false;
    const uint32_t m_maxRequestRetries = 4;

    CURLM* m_curlMultiHandle = nullptr;

    bool m_useDebugFunction = false;

    std::function<void(const std::string&)> m_Logger;
};

struct HttpRequestContext
{
    HttpRequestContext(HttpManager* inHttpManager, XUserHandle user, const std::string& verb, const std::string& url, const std::vector<HttpHeader>& headers, uint8_t* bodyBytes, size_t bodyLen, std::function<void(HttpRequestContext*)> inOnCompleted) :
        httpManager(inHttpManager),
        authUser(user),
        verb(verb),
        url(url),
        requestHeaders(headers),
        requestBodyBytes(bodyBytes),
        bodySize(bodyLen),
        onCompleted(inOnCompleted)
    {
    }

    HttpRequestContext(HttpManager* inHttpManager, XUserHandle user, const std::string& verb, const std::string& url, const std::vector<HttpHeader>& headers, const std::string& bodyString, size_t bodyLen, std::function<void(HttpRequestContext*)> inOnCompleted) :
        httpManager(inHttpManager),
        authUser(user),
        verb(verb),
        url(url),
        requestHeaders(headers),
        requestBodyBytes(nullptr),
        requestBodyString(bodyString),
        bodySize(bodyLen),
        onCompleted(inOnCompleted)
    {
    }

    ~HttpRequestContext()
    {
        if (headersForSignature != nullptr)
        {
            delete[] headersForSignature;
            headersForSignature = nullptr;
        }

        // Clean up the memory for this linked list
        if (curlHeaderList != nullptr)
        {
            curl_slist_free_all(curlHeaderList);
            curlHeaderList = nullptr;
        }
    }

    HttpManager* httpManager = nullptr;

    XUserHandle authUser = nullptr;
    std::string verb;
    std::string url;
    std::vector<HttpHeader> requestHeaders;
    struct curl_slist* curlHeaderList = nullptr;
    uint8_t* requestBodyBytes = nullptr;
    std::string requestBodyString;
    std::string requestBodyContentType;
    size_t bodySize = 0;
    size_t bytesSent = 0;
    std::function<void(HttpRequestContext*)> onCompleted = nullptr;

    std::vector<HttpHeader> responseHeaders;
    std::vector<uint8_t> responseBody;
    uint64_t responseBytesReceived = 0;
    uint32_t responseStatusCode = 0;

    bool shouldRetry = false;
    uint32_t numRetries = 0;

    XUserGetTokenAndSignatureData* tokenAndSignature = nullptr;
    XUserGetTokenAndSignatureHttpHeader* headersForSignature = nullptr;
    std::vector<uint8_t> tokenAndSignatureBuffer;
};

struct HttpHeader
{
    HttpHeader(const std::string& inName, const std::string& inValue) :
        name(inName),
        value(inValue)
    {
    }

    std::string name;
    std::string value;

    bool IsValid() const
    {
        return name.empty() == false;
    }

    std::string ToString() const
    {
        return name + ": " + value;
    }
};
