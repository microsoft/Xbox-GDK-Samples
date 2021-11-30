//--------------------------------------------------------------------------------------
// HttpManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HttpManager.h"

#pragma warning( disable : 4100 )

namespace 
{
    void ReplaceSubstring(std::string& source, const std::string& from, const std::string& to)
    {
        if (from.empty() == false)
        {
            for (auto pos = source.find(from);
                pos != std::string::npos;
                source.replace(pos, from.size(), to),
                pos = source.find(from, pos + to.size()))
            {
            }
        }
    }

    bool SplitString(const std::string& source, const std::string& delim, std::string& left, std::string& right)
    {
        if (delim.empty() == false)
        {
            std::string str = source;

            auto pos = str.find(delim);
            if (pos != std::string::npos)
            {
                left = str.substr(0, pos);
                str.erase(0, pos + delim.length());
                right = str;

                return true;
            }
        }

        return false;
    }

    void RemoveLeadingWhiteSpace(std::string& value)
    {
        if (value.empty() == false && value[0] == ' ')
        {
            value = value.substr(1, value.size() - 1);
        }
    }

    void* XCurl_Malloc(size_t size, uint32_t /*memoryType*/)
    {
        return malloc(size);
    }

    void XCurl_Free(void* Ptr, uint32_t /*memoryType*/)
    {
        free(Ptr);
    }

    void* XCurl_Realloc(void* ptr, size_t size, uint32_t /*memoryType*/)
    {
        return realloc(ptr, size);
    }

    char* Curl_Strdup(const char* str)
    {
        return _strdup(str);
    }

    void* XCurl_Calloc(size_t num, size_t size, uint32_t /*memoryType*/)
    {
        return calloc(num, size);
    }

    std::string GetCurlErrorString(CURLcode res)
    {
        return std::string(curl_easy_strerror(res));
    }

    std::string GetCurlMultiErrorString(CURLMcode res)
    {
        return std::string(curl_multi_strerror(res));
    }

    size_t Curl_ReadFunc(uint8_t* data, size_t size, size_t nmemb, void* ptr)
    {
        HttpRequestContext* context = static_cast<HttpRequestContext*>(ptr);
        if (context == nullptr)
        {
            OutputDebugStringA("HttpManager::Curl_ReadFunc: context was null");
            return 0;
        }

        size_t copiedSize = 0;
        if (size * nmemb > context->bodySize - context->bytesSent)
        {
            copiedSize = context->bodySize - context->bytesSent;
        }
        else
        {
            copiedSize = size * nmemb;
        }

        if (copiedSize > 0)
        {
            memcpy(data, context->requestBodyBytes + context->bytesSent, copiedSize);
            context->bytesSent += copiedSize;
        }

        return copiedSize;
    }

    size_t Curl_ReceiveResponseHeadersFunc(uint8_t* data, size_t size, size_t nmemb, void* ptr)
    {
        HttpRequestContext* context = static_cast<HttpRequestContext*>(ptr);
        if (context == nullptr)
        {
            return 0;
        }

        const size_t numBytes = size * nmemb;
        std::string str((char*)data, numBytes);

        ReplaceSubstring(str, "\n", "");
        ReplaceSubstring(str, "\r", "");

        std::string name;
        std::string value;
        if (SplitString(str, ":", name, value))
        {
            RemoveLeadingWhiteSpace(value);

            context->responseHeaders.emplace_back(name, value);
        }

        return numBytes;
    }

    size_t Curl_ReceiveResponseBodyFunc(uint8_t* data, size_t size, size_t nmemb, void* ptr)
    {
        HttpRequestContext* context = static_cast<HttpRequestContext*>(ptr);
        if (context == nullptr)
        {
            return 0;
        }

        const size_t numBytes = size * nmemb;
        context->responseBody.insert(context->responseBody.end(), &data[0], &data[numBytes]);

        return numBytes;
    }

    int Curl_DebugFunction(CURL* /*handle*/, curl_infotype type, char* data, size_t size, void* /*userp*/)
    {
        std::string text;

        switch (type)
        {
        case CURLINFO_TEXT:
        {
            text = "== Info: " + std::string(data);
            break;
        }
        case CURLINFO_HEADER_OUT:   text = "=> Send header (" + std::to_string(size) + " bytes)";   break;
        case CURLINFO_DATA_OUT:     text = "=> Send data (" + std::to_string(size) + " bytes)";     break;
        case CURLINFO_SSL_DATA_OUT: text = "=> Send SSL data (" + std::to_string(size) + " bytes)"; break;
        case CURLINFO_HEADER_IN:    text = "<= Recv header (" + std::to_string(size) + " bytes)";   break;
        case CURLINFO_DATA_IN:      text = "<= Recv data (" + std::to_string(size) + " bytes)";     break;
        case CURLINFO_SSL_DATA_IN:  text = "<= Recv SSL data (" + std::to_string(size) + " bytes)"; break;
        case CURLINFO_END:
            break;
        default:
            break;
        }

        OutputDebugStringA(text.c_str());

        return 0;
    }
}

HttpManager::HttpManager()
{
    Initialize();
}

HttpManager::HttpManager(std::function<void(const std::string&)> LogFunction)
{
    m_Logger = LogFunction;
    Initialize();
}

HttpManager::~HttpManager()
{
    CleanUp();
}

void HttpManager::HttpManagerLog(std::string message)
{
    if (m_Logger)
    {
        m_Logger(message);
    }
}

void HttpManager::Initialize()
{
    HttpManagerLog("HttpManager::Initialize:");

    if (m_initialized == false)
    {
        CURLcode res;

        // Use xcurl_global_init_mem if you want to hook into your own memory manager
        // You can still use curl_global_init if you don't want to use your own memory hooks
        res = xcurl_global_init_mem(CURL_GLOBAL_DEFAULT, XCurl_Malloc, XCurl_Free, XCurl_Realloc, Curl_Strdup, XCurl_Calloc);
        if (res != CURLE_OK)
        {
            HttpManagerLog("HttpManager::Initialize: xcurl_global_init_mem failed with error: " + GetCurlErrorString(res));
            return;
        }

        if (m_curlMultiHandle == nullptr)
        {
            m_curlMultiHandle = curl_multi_init();

            if (m_curlMultiHandle == nullptr)
            {
                HttpManagerLog("HttpManager::Initialize: curl_multi_init failed");
                return;
            }
        }

        m_initialized = true;
    }
}

void HttpManager::CleanUp()
{
    HttpManagerLog("HttpManager::CleanUp:");

    if (m_initialized == true)
    {
        if (m_curlMultiHandle)
        {
            CURLMcode res;

            res = curl_multi_cleanup(m_curlMultiHandle);
            if (res != CURLM_OK)
            {
                HttpManagerLog("HttpManager::CleanUp: curl_multi_cleanup failed with error: " + GetCurlMultiErrorString(res));
                return;
            }

            m_curlMultiHandle = nullptr;
        }

        curl_global_cleanup();

        m_initialized = false;
    }
}

HRESULT HttpManager::MakeHttpRequestAsync(XUserHandle user, const std::string& verb, const std::string& uri, const std::vector<HttpHeader>& headers, uint8_t* bodyBytes, size_t bodyLen, std::function<void(HttpRequestContext*)> OnCompleted)
{
    HttpManagerLog("HttpManager::MakeHttpRequestAsync: ");

    if (m_initialized == false)
    {
        HttpManagerLog("HttpManager::MakeHttpRequestAsync: HttpManager is not initialized");

        return E_FAIL;
    }

    HttpRequestContext* context = new HttpRequestContext(this, user, verb, uri, headers, bodyBytes, bodyLen, OnCompleted);
    if (context)
    {
        if (context->authUser)
        {
            return BeginAuthorization(context);
        }
        else
        {
            // Skip directly to sending the request
            return SendHttpRequest(context);
        }
    }

    return E_FAIL;
}

HRESULT HttpManager::MakeHttpRequestAsync(XUserHandle user, const std::string& verb, const std::string& uri, const std::vector<HttpHeader>& headers, std::string bodyString, size_t bodyLen, std::function<void(HttpRequestContext*)> OnCompleted)
{
    HttpManagerLog("HttpManager::MakeHttpRequestAsync: ");

    if (m_initialized == false)
    {
        HttpManagerLog("HttpManager::MakeHttpRequestAsync: HttpManager is not initialized");

        return E_FAIL;
    }

    HttpRequestContext* context = new HttpRequestContext(this, user, verb, uri, headers, bodyString, bodyLen, OnCompleted);
    if (context)
    {
        if (context->authUser)
        {
            return BeginAuthorization(context);
        }
        else
        {
            // Skip directly to sending the request
            return SendHttpRequest(context);
        }
    }

    return E_FAIL;
}

HRESULT HttpManager::BeginAuthorization(HttpRequestContext* context)
{
    HttpManagerLog("HttpManager::BeginAuthorization:");

    if (context == nullptr)
    {
        HttpManagerLog("HttpManager::BeginAuthorization: context was null");
        return E_FAIL;
    }

    auto async = new XAsyncBlock{};

    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        HttpRequestContext* context = reinterpret_cast<HttpRequestContext*>(async->context);

        size_t bufferSize = 0;

        // Get the size of the buffer, waiting for the operation to complete
        auto hr = XUserGetTokenAndSignatureResultSize(async, &bufferSize);

        if (FAILED(hr))
        {
            context->httpManager->HttpManagerLog("Error during XUserGetTokenAndSignatureResultSize: " + std::to_string(hr));

            delete async;
            async = nullptr;

            return;
        }

        size_t resultBufferUsed = 0;

        context->tokenAndSignatureBuffer.resize(bufferSize);

        // Read the token and signature
        hr = XUserGetTokenAndSignatureResult(
            async,
            bufferSize,
            context->tokenAndSignatureBuffer.data(),
            &context->tokenAndSignature,
            &resultBufferUsed
        );

        if (SUCCEEDED(hr))
        {
            if (HttpManager* httpManager = context->httpManager)
            {
                // Now we have the token and/or signature so we're ready to make the request
                // or retry the request if it failed authorization previously
                if (context->shouldRetry)
                {
                    httpManager->RetryHttpRequest(context);
                }
                else
                {
                    httpManager->SendHttpRequest(context);
                }
            }
        }
        else
        {
            context->httpManager->HttpManagerLog("Error during XUserGetTokenAndSignatureResult: " + std::to_string(hr));
        }

        delete async;
        async = nullptr;
    };

    //  We need to convert the headers to GetTokenAndSignatureHeaders so that they can be included in
    //  the signature generation if the signature policy requires those headers
    size_t headerCount = context->requestHeaders.size();

    context->headersForSignature = new XUserGetTokenAndSignatureHttpHeader[headerCount];

    for (uint64_t i = 0; i < headerCount; i++)
    {
        context->headersForSignature[i].name = context->requestHeaders[i].name.c_str();
        context->headersForSignature[i].value = context->requestHeaders[i].value.c_str();
    }

    // Request the token and signature to add to the request headers
    // Force a token refresh if we've already received a 401 unauthorized response to this query
    auto hr = XUserGetTokenAndSignatureAsync(
        context->authUser,
        context->responseStatusCode == 401 ? XUserGetTokenAndSignatureOptions::ForceRefresh : XUserGetTokenAndSignatureOptions::None,
        context->verb.c_str(),
        context->url.c_str(),
        headerCount,
        context->headersForSignature,
        context->bodySize,
        context->requestBodyBytes,
        async
    );

    if (FAILED(hr))
    {
        HttpManagerLog("Error during XUserGetTokenAndSignatureAsync: " + std::to_string(hr));

        delete async;
        async = nullptr;

        return hr;
    }

    return E_PENDING;
}

HRESULT HttpManager::SendHttpRequest(HttpRequestContext* context)
{
    HttpManagerLog("HttpManager::SendHttpRequest:");

    if (m_initialized == false)
    {
        HttpManagerLog("HttpManager::SendHttpRequest: HttpManager is not initialized");
        return E_FAIL;
    }

    if (context == nullptr)
    {
        HttpManagerLog("HttpManager::SendHttpRequest: context was null");
        return E_FAIL;
    }

    if (m_curlMultiHandle == nullptr)
    {
        HttpManagerLog("HttpManager::SendHttpRequest: m_curlMultiHandle was null");
        return E_FAIL;
    }

    if (CURL* curlEasyHandle = curl_easy_init())
    {
        CURLcode resEasy;

        if (m_useDebugFunction)
        {
            // Optional debugging features
            resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_DEBUGFUNCTION, Curl_DebugFunction);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_DEBUGFUNCTION failed with error: " + GetCurlErrorString(resEasy));
                return E_FAIL;
            }

            // Optional debugging features - DEBUGFUNCTION has no effect until we enable VERBOSE
            resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_VERBOSE, 1L);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_VERBOSE failed with error: " + GetCurlErrorString(resEasy));
                return E_FAIL;
            }
        }

        // URL
        resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_URL, context->url.c_str());
        if (resEasy != CURLE_OK)
        {
            HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_URL failed with error: " + GetCurlErrorString(resEasy));
            return E_FAIL;
        }

        // VERB
        if (context->verb == "POST")
        {
            resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_POST, 1);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_POST failed with error: " + GetCurlErrorString(resEasy));
                return E_FAIL;
            }

            if (!context->requestBodyString.empty())
            {
                resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_POSTFIELDS, context->requestBodyString.c_str());
                if (resEasy != CURLE_OK)
                {
                    HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_POSTFIELDS with requestBodyString failed with error: " + GetCurlErrorString(resEasy));
                    return E_FAIL;
                }
            }
            else
            {
                // Default to what is in the requestBytes structure by setting the PostFields to Null and using the CURLOPT_READDATA and
                // CURLOPT_READFUNCTION code below.
                resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_POSTFIELDS, NULL);
                if (resEasy != CURLE_OK)
                {
                    HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_POSTFIELDS NULL failed with error: " + GetCurlErrorString(resEasy));
                    return E_FAIL;
                }
            }
        }
        else if (context->verb == "PUT")
        {
            //  Not tested
            resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_UPLOAD, 1);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_PUT failed with error: " + GetCurlErrorString(resEasy));
                return E_FAIL;
            }

            resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_INFILESIZE_LARGE, context->bodySize);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_INFILESIZE_LARGE failed with error: " + GetCurlErrorString(resEasy));
                return E_FAIL;
            }

        }
        else  // Default to GET
        {
            resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_HTTPGET, 1);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_HTTPGET failed with error: " + GetCurlErrorString(resEasy));
                return E_FAIL;
            }
        }

        if (context->requestBodyBytes && context->bodySize != 0)
        {
            resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_READDATA, context);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_READDATA failed with error: " + GetCurlErrorString(resEasy));
                return E_FAIL;
            }

            resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_READFUNCTION, Curl_ReadFunc);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_READFUNCTION failed with error: " + GetCurlErrorString(resEasy));
                return E_FAIL;
            }
        }

        // Receive response headers (tell curl where the data will go)
        resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_HEADERDATA, context);
        if (resEasy != CURLE_OK)
        {
            HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_HEADERDATA failed with error: " + GetCurlErrorString(resEasy));
            return E_FAIL;
        }

        // Receive response headers (tell curl which function to use to write the data)
        resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_HEADERFUNCTION, Curl_ReceiveResponseHeadersFunc);
        if (resEasy != CURLE_OK)
        {
            HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_HEADERFUNCTION failed with error: " + GetCurlErrorString(resEasy));
            return E_FAIL;
        }

        // Receive response body (tell curl where the data will go)
        resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_WRITEDATA, context);
        if (resEasy != CURLE_OK)
        {
            HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_WRITEDATA failed with error: " + GetCurlErrorString(resEasy));
            return E_FAIL;
        }

        // Receive response body (tell curl which function to use to write the data)
        resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_WRITEFUNCTION, Curl_ReceiveResponseBodyFunc);
        if (resEasy != CURLE_OK)
        {
            HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_WRITEFUNCTION failed with error: " + GetCurlErrorString(resEasy));
            return E_FAIL;
        }

        // Add the token and signature if present
        if (context->tokenAndSignature != nullptr)
        {
            context->requestHeaders.emplace_back(HttpHeader("Authorization", context->tokenAndSignature->token));

            //  If there is no signature policy this value will be null
            if (context->tokenAndSignature->signature != nullptr)
            {
                context->requestHeaders.emplace_back(HttpHeader("Signature", context->tokenAndSignature->signature));
            }
        }

        if (context->requestHeaders.empty() == false)
        {
            for (const HttpHeader& header : context->requestHeaders)
            {
                if (header.IsValid())
                {
                    std::string formattedHeaderString = header.ToString();

                    HttpManagerLog("HttpManager::SendHttpRequest: appending header: " + formattedHeaderString);

                    // This list must remain valid for the full lifetime of the request
                    context->curlHeaderList = curl_slist_append(context->curlHeaderList, formattedHeaderString.c_str());
                }
            }

            // Headers
            resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_HTTPHEADER, context->curlHeaderList);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_HTTPHEADER failed with error: " + GetCurlErrorString(resEasy));
                return E_FAIL;
            }
        }

        // Context
        resEasy = curl_easy_setopt(curlEasyHandle, CURLOPT_PRIVATE, context);
        if (resEasy != CURLE_OK)
        {
            HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_setopt CURLOPT_PRIVATE failed with error: " + GetCurlErrorString(resEasy));
            return E_FAIL;
        }

        // Treat curl_multi_add_handle like StartRequest() that once you call it the request is in flight.
        CURLMcode resMulti = curl_multi_add_handle(m_curlMultiHandle, curlEasyHandle);
        if (resMulti != CURLM_OK)
        {
            HttpManagerLog("HttpManager::SendHttpRequest: curl_multi_add_handle failed with error: " + GetCurlMultiErrorString(resMulti));
            return E_FAIL;
        }

        return E_PENDING;
    }
    else
    {
    HttpManagerLog("HttpManager::SendHttpRequest: curl_easy_init failed");
    }

    return E_FAIL;
}

void HttpManager::Update()
{
    if (m_initialized == false)
    {
        return;
    }

    int runningHandles = 0;
    CURLMcode resMulti = curl_multi_perform(m_curlMultiHandle, &runningHandles);
    if (resMulti != CURLM_OK)
    {
        HttpManagerLog("HttpManager::Update: curl_multi_perform failed with error: " + GetCurlMultiErrorString(resMulti));
        return;
    }

    while (true)
    {
        int msgsInQueue = 0;
        CURLMsg* curlMsg = curl_multi_info_read(m_curlMultiHandle, &msgsInQueue);

        if (curlMsg == nullptr)
        {
            break;
        }

        if (curlMsg->msg == CURLMSG_DONE)
        {
            CURL* completedHandle = curlMsg->easy_handle;
            resMulti = curl_multi_remove_handle(m_curlMultiHandle, completedHandle);
            if (resMulti != CURLM_OK)
            {
                HttpManagerLog("HttpManager::Update: curl_multi_remove_handle failed with error: " + GetCurlMultiErrorString(resMulti));
                return;
            }

            // Context
            HttpRequestContext* context = nullptr;
            CURLcode resEasy = curl_easy_getinfo(completedHandle, CURLINFO_PRIVATE, &context);
            if (resEasy != CURLE_OK)
            {
                HttpManagerLog("HttpManager::Update: curl_easy_getinfo CURLINFO_PRIVATE failed with error: " + GetCurlErrorString(resEasy));
                return;
            }

            if(context)
            {
                resEasy = curlMsg->data.result;

                if (resEasy != CURLE_OK)
                {
                    HRESULT hr;
                    CURLcode getInfoResult = curl_easy_getinfo(completedHandle, CURLINFO_OS_ERRNO, &hr);
                    if (getInfoResult != CURLE_OK)
                    {
                        HttpManagerLog("HttpManager::Update: request failed with error: " + GetCurlErrorString(resEasy));
                    }
                    else
                    {
                        HttpManagerLog("HttpManager::Update: request failed with error: CURLcode= " + GetCurlErrorString(resEasy) + " HRESULT= " + std::to_string(hr));
                    }
                    
                    return;
                }

                // Response code
                resEasy = curl_easy_getinfo(completedHandle, CURLINFO_RESPONSE_CODE, &context->responseStatusCode);
                if (resEasy != CURLE_OK)
                {
                    HttpManagerLog("HttpManager::Update: curl_easy_getinfo CURLINFO_RESPONSE_CODE failed with error: " + GetCurlErrorString(resEasy));
                    return;
                }

                // Response Size
                resEasy = curl_easy_getinfo(completedHandle, CURLINFO_SIZE_DOWNLOAD_T, &context->responseBytesReceived);
                if (resEasy != CURLE_OK)
                {
                    HttpManagerLog("HttpManager::Update: curl_easy_getinfo CURLINFO_SIZE_DOWNLOAD_T failed with error: " + GetCurlErrorString(resEasy));
                    return;
                }

                CompleteHttpRequest(context);
            }
            else
            {
                HttpManagerLog("Could not find mapping for completed request");
            }
        }
    }
}

HRESULT HttpManager::CompleteHttpRequest(HttpRequestContext* context)
{
    HttpManagerLog("HttpManager::CompleteHttpRequest:");

    if (context == nullptr)
    {
        HttpManagerLog("HttpManager::CompleteHttpRequest: context was null");
        return E_FAIL;
    }

    // Check for failures due to an invalid token/signature and force a refresh on the retry
    if (context->responseStatusCode == 401 && context->authUser != nullptr)
    {
        HttpManagerLog("HttpManager::CompleteHttpRequest: 401 UNAUTHORIZED - Attempting to refresh user token");

        if (context->numRetries < m_maxRequestRetries)
        {
            context->numRetries++;
            context->shouldRetry = true;

            return BeginAuthorization(context);
        }
        else
        {
            // Let the error bubble up to the caller
            context->shouldRetry = false;
        }
    }

    if (context->onCompleted)
    {
        context->onCompleted(context);
    }

    delete context;
    context = nullptr;

    return S_OK;
}

HRESULT HttpManager::RetryHttpRequest(HttpRequestContext* context)
{
    HttpManagerLog("HttpManager::RetryHttpRequest:");

    if (context == nullptr)
    {
        HttpManagerLog("HttpManager::RetryHttpRequest: context was null");
        return E_FAIL;
    }

    context->responseBody.clear();
    context->responseBytesReceived = 0;
    context->responseStatusCode = 0;

    if (context->headersForSignature != nullptr)
    {
        delete[] context->headersForSignature;
        context->headersForSignature = nullptr;
    }

    return SendHttpRequest(context);
}
