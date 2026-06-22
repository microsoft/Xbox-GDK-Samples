//--------------------------------------------------------------------------------------
// WinHttpManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleWinHttp_Desktop.h"
#include "WinHttpManager.h"
#include "XAsync.h"
#include <Wincrypt.h>

using namespace ATG;

// Static members
std::map<uint32_t, HINTERNET> WinHttpManager::m_httpSessions;
std::mutex WinHttpManager::m_httpHandleQueueLock;
std::mutex WinHttpManager::m_httpSessionMapLock;
std::map<HINTERNET, WinHttpManager::ContextObject*> WinHttpManager::m_httpHandleQueue;

// Plumbing to show status in the sample UI
extern std::unique_ptr<Sample> g_sample;

void ATG::DisplayMessage(std::string message)
{
    if (g_sample)
    {
        g_sample->SendMessageToScreen(message);
    }
}

void WinHttpManager::Reset()
{
    // Calling reset will close held handles returning the manager to an initial state.
    // This is used after a Suspend/Resume.
    {
        std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);

        for (const auto& handle : m_httpHandleQueue)
        {
            WinHttpCloseHandle(handle.first);
        }

        m_httpHandleQueue.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_httpSessionMapLock);

        for (const auto& handle : m_httpSessions)
        {
            WinHttpCloseHandle(handle.second);
        }

        m_httpSessions.clear();
    }
}

HRESULT WinHttpManager::GetSessionBySecurityInformation(XNetworkingSecurityInformation* si, HINTERNET* session)
{
    std::lock_guard<std::mutex> lock(m_httpSessionMapLock);

    // Check the cache first
    auto it = m_httpSessions.find(si->enabledHttpSecurityProtocolFlags);
    if (it != m_httpSessions.end())
    {
        *session = it->second;
        return S_OK;
    }

    // Create the WinHttp Session.
    // NOTE: This session handle should be used for all further WinHttp
    //       operations with the same flags and not be closed/re-created for each query

    DWORD flags = WINHTTP_FLAG_ASYNC;

#if 0
    // Note, on console WINHTTP_FLAG_SECURE_DEFAULTS works
    // but does not on the currently released version of
    // Windows.  Use WINHTTP_FLAG_ASYNC on PC until the
    // update to Windows is released later in 2020

    if (XGameRuntimeIsFeatureAvailable(XGameRuntimeFeature::XNetworking))
    {
        // Require async and secure operating in XNetworking
        flags = WINHTTP_FLAG_SECURE_DEFAULTS;
    }
#endif

    auto httpSession = WinHttpOpen(
        L"SimpleWinHttp Sample/1.0",            // HTTP UserAgent value
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,    // Use System and per-User proxy settings
        WINHTTP_NO_PROXY_NAME,                  // Empty value
        WINHTTP_NO_PROXY_BYPASS,                // Empty value
        flags                                   // Use HTTPS and asynchronous handling
        );

    if (httpSession == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Set the security protocol flags on the session
    auto result = WinHttpSetOption(
        httpSession,                                // HTTP Session to configure
        WINHTTP_OPTION_SECURE_PROTOCOLS,            // Set the secure protocols option
        &si->enabledHttpSecurityProtocolFlags,      // Use the flags from the security information
        sizeof(si->enabledHttpSecurityProtocolFlags)
        );

    if (result == false)
    {
        WinHttpCloseHandle(httpSession);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Set the timeouts to be 20 seconds
    result = WinHttpSetTimeouts(
        httpSession,
        0,              // Resolve timeout, use default
        20000,          // Connect timeout
        20000,          // Send timeout
        20000           // Receive timeout
        );

    if (result == false)
    {
        WinHttpCloseHandle(httpSession);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Setup the status callback on the Session handle so all Requests created from this
    // Session will also use the same callback automagically.
    auto previous = WinHttpSetStatusCallback(
        httpSession,                                // The Http Session for which we want notifications
        WinHttpStatusCallback,                      // The callback function
        WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,    // Call us back for every event
        NULL                                        // Reserved
        );

    if (previous == WINHTTP_INVALID_STATUS_CALLBACK)
    {
        WinHttpCloseHandle(httpSession);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Cache it for later
    m_httpSessions[si->enabledHttpSecurityProtocolFlags] = httpSession;

    // Return it to the caller
    *session = httpSession;

    return S_OK;
}

bool WinHttpManager::NetworkAvailable()
{
    if (XGameRuntimeIsFeatureAvailable(XGameRuntimeFeature::XNetworking))
    {
        XNetworkingConnectivityHint connectivityHint;

        auto hr = XNetworkingGetConnectivityHint(&connectivityHint);

        if (SUCCEEDED(hr))
        {
            return connectivityHint.networkInitialized;
        }

        return false;
    }

    return true;
}

void WinHttpManager::WinHttpStatusCallback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID statusInfo, DWORD statusInfoLength)
{
    switch (status)
    {
        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            // A request has completed and the headers are available so now
            // see if there is data available to be read as well
            DisplayMessage("WinHttp Status: HEADERS_AVAILABLE");

            auto httpContext = reinterpret_cast<WinHttpRequestContext*>(context);
            if (httpContext != nullptr)
            {
                DWORD headerSize = 0;

                // Request the size needed for the header query
                auto result = WinHttpQueryHeaders(
                    httpContext->HttpRequest,
                    WINHTTP_QUERY_RAW_HEADERS_CRLF,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    NULL,
                    &headerSize,
                    WINHTTP_NO_HEADER_INDEX
                    );

                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {
                    DWORD bufferSize = headerSize / sizeof(wchar_t);
                    auto buffer = new wchar_t[bufferSize];

                    // Now call again with the allocated buffer
                    result = WinHttpQueryHeaders(
                        httpContext->HttpRequest,
                        WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        buffer,
                        &headerSize,
                        WINHTTP_NO_HEADER_INDEX
                        );

                    if (result)
                    {
                        httpContext->ResponseHeaders = buffer;
                        httpContext->HeaderLength = bufferSize;
                    }
                    else
                    {
                        delete[] buffer;
                    }
                }

                if (!result)
                {
                    DisplayMessage("Unable to retrieve headers: " + std::to_string(GetLastError()));

                    XAsyncComplete(
                        httpContext->AsyncBlock,
                        HRESULT_FROM_WIN32(GetLastError()),
                        0
                        );

                    break;
                }

                // Try to get the total buffer size from the headers
                DWORD contentLength = 0;
                DWORD contentLengthSize = sizeof(contentLength);

                httpContext->Response.clear();

                // Get the size of the response body
                result = WinHttpQueryHeaders(
                    httpContext->HttpRequest,
                    WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                    nullptr,
                    &contentLength,
                    &contentLengthSize,
                    nullptr
                    );

                if (result)
                {
                    // The Content-Length header was found
                    httpContext->HaveContentLengthHeader = true;
                    httpContext->ContentLength = contentLength;

                    // Size the read buffer to the required length
                    httpContext->Response.resize(contentLength);
                }

                // We can now query if there is a response body to read
                result = WinHttpQueryDataAvailable(
                    httpContext->HttpRequest,
                    nullptr
                    );

                if (result == false)
                {
                    XAsyncComplete(
                        httpContext->AsyncBlock,
                        HRESULT_FROM_WIN32(GetLastError()),
                        0
                        );
                }
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
        {
            DisplayMessage("WinHttp Status: DATA_AVAILABLE");

            auto httpContext = reinterpret_cast<WinHttpRequestContext*>(context);
            if (httpContext != nullptr)
            {
                auto bytesAvailable = *reinterpret_cast<DWORD*>(statusInfo);

                if (bytesAvailable > 0)
                {
                    DWORD bytesToRead = std::max(bytesAvailable, static_cast<DWORD>(s_readBufferSize));

                    if (httpContext->HaveContentLengthHeader)
                    {
                        bytesToRead = static_cast<DWORD>(httpContext->Response.size() - httpContext->ResponseBytesReceived);

                        if (bytesAvailable > bytesToRead)
                        {
                            // There's more data than we were told there should be in the Content-Length header
                            XAsyncComplete(
                                httpContext->AsyncBlock,
                                HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                                0
                                );

                            break;
                        }
                    }
                    else
                    {
                        // See if we need to grow the buffer
                        auto neededSize = httpContext->ResponseBytesReceived + bytesToRead;
                        if (httpContext->Response.size() < neededSize)
                        {
                            httpContext->Response.resize(neededSize);
                        }
                    }

                    // Start the read operation
                    auto result = WinHttpReadData(
                        httpContext->HttpRequest,
                        httpContext->Response.data() + httpContext->ResponseBytesReceived,
                        bytesToRead,
                        nullptr
                        );

                    if (result == false)
                    {
                        XAsyncComplete(
                            httpContext->AsyncBlock,
                            HRESULT_FROM_WIN32(GetLastError()),
                            0
                            );
                    }
                }
                else
                {
                    // No more data avilable

                    if (httpContext->HaveContentLengthHeader && httpContext->ResponseBytesReceived != httpContext->ContentLength)
                    {
                        // It's an error to send more/less data than specified in the Content-Length header
                        XAsyncComplete(
                            httpContext->AsyncBlock,
                            HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                            0
                            );
                    }
                    else
                    {
                        httpContext->CompleteRequest();
                    }
                }
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            DisplayMessage("WinHttp Status: READ_COMPLETE");

            auto httpContext = reinterpret_cast<WinHttpRequestContext*>(context);
            if (httpContext != nullptr)
            {
                httpContext->ResponseBytesReceived += statusInfoLength;

                // Check if more data is available
                auto result = WinHttpQueryDataAvailable(
                    httpContext->HttpRequest,
                    nullptr
                    );

                if (result == false)
                {
                    XAsyncComplete(
                        httpContext->AsyncBlock,
                        HRESULT_FROM_WIN32(GetLastError()),
                        0
                        );
                }
            }
            else
            {
                // If there's no context object, it's from a WebSocket read operation
                WebSocketDataContext* wsContext = nullptr;

                {
                    std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);

                    auto it = m_httpHandleQueue.find(handle);
                    if (it != m_httpHandleQueue.end())
                    {
                        wsContext = reinterpret_cast<WebSocketDataContext*>(it->second);
                    }
                }

                if (wsContext != nullptr)
                {
                    auto wsStatus = *reinterpret_cast<WINHTTP_WEB_SOCKET_STATUS*>(statusInfo);

                    if (wsStatus.eBufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE)
                    {
                        // We've received all data now
                        wsContext->BytesTransferred += wsStatus.dwBytesTransferred;

                        XAsyncComplete(
                            wsContext->AsyncBlock,
                            S_OK,
                            wsContext->BytesTransferred
                            );
                    }
                    else if (wsStatus.eBufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE)
                    {
                        // There's more data available.  Grow the buffer and read some more.
                        auto currentSize = wsContext->DataBuffer.size();

                        wsContext->DataBuffer.resize(currentSize + s_readBufferSize);

                        DWORD err = WinHttpWebSocketReceive(
                            wsContext->WebSocket->m_webSocket,
                            wsContext->DataBuffer.data() + currentSize,
                            static_cast<DWORD>(wsContext->DataBuffer.size()),
                            nullptr,
                            nullptr
                            );

                        if (err != ERROR_SUCCESS)
                        {
                            DisplayMessage("WinHttp Error during WinHttpWebSocketReceive: " + std::to_string(err));

                            wsContext->WebSocket->SetSocketStatus(WebSocketStatus::Error);

                            XAsyncComplete(
                                wsContext->AsyncBlock,
                                HRESULT_FROM_WIN32(err),
                                0
                                );
                        }
                    }
                    else
                    {
                        DisplayMessage("Unexpected WINHTTP_WEB_SOCKET_BUFFER_TYPE: " + std::to_string(wsStatus.eBufferType));

                        wsContext->WebSocket->SetSocketStatus(WebSocketStatus::Error);

                        XAsyncComplete(
                            wsContext->AsyncBlock,
                            E_UNEXPECTED,
                            0
                            );
                    }
                }
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            // A WinHttp request failed
            DisplayMessage("WinHttp Status: REQUEST_ERROR");

            auto httpContext = reinterpret_cast<WinHttpRequestContext*>(context);
            if (httpContext != nullptr)
            {
                auto result = reinterpret_cast<WINHTTP_ASYNC_RESULT*>(statusInfo);
                if (result != nullptr)
                {
                    // If we've stored an HR for the task, return that
                    auto hr = FAILED(httpContext->TaskResult)
                        ? httpContext->TaskResult
                        : HRESULT_FROM_WIN32(result->dwError);

                    XAsyncComplete(
                        httpContext->AsyncBlock,
                        hr,
                        0
                        );
                }
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            // A WinHttp request has completed successfully
            DisplayMessage("WinHttp Status: SENDREQUEST_COMPLETE");

            auto httpContext = reinterpret_cast<WinHttpRequestContext*>(context);
            if (httpContext != nullptr)
            {
                // Tell the context to complete the request and retrieve the response
                httpContext->ProcessRequest();
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
        {
            DisplayMessage("WinHttp Status: WRITE_COMPLETE");

            std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);

            auto it = m_httpHandleQueue.find(handle);
            if (it != m_httpHandleQueue.end())
            {
                auto httpContext = reinterpret_cast<WebSocketDataContext*>(it->second);
                auto wsstatus = *reinterpret_cast<WINHTTP_WEB_SOCKET_STATUS*>(statusInfo);

                if (wsstatus.eBufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE)
                {
                    httpContext->BytesTransferred = wsstatus.dwBytesTransferred;

                    // The write operation has completed successfully
                    XAsyncComplete(
                        httpContext->AsyncBlock,
                        S_OK,
                        sizeof(DWORD)
                        );
                }
                else
                {
                    DisplayMessage("Unexpected buffer type received: " + std::to_string(wsstatus.eBufferType));

                    XAsyncComplete(
                        httpContext->AsyncBlock,
                        E_UNEXPECTED,
                        0
                        );
                }
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE:
        {
            DisplayMessage("WinHttp Status: CLOSE_COMPLETE");

            std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);

            auto it = m_httpHandleQueue.find(handle);
            if (it != m_httpHandleQueue.end())
            {
                auto httpContext = reinterpret_cast<WebSocketCloseContext*>(it->second);

                BYTE reasonBuffer[WINHTTP_WEB_SOCKET_MAX_CLOSE_REASON_LENGTH];
                DWORD reasonLength = 0;
                USHORT closestatus = 0;

                // The WebSocket close is complete, now query the result
                auto err = WinHttpWebSocketQueryCloseStatus(
                    httpContext->WebSocket->m_webSocket,
                    &closestatus,
                    reasonBuffer,
                    ARRAYSIZE(reasonBuffer),
                    &reasonLength
                    );

                if (err != ERROR_SUCCESS)
                {
                    DisplayMessage("WinHttp Error during WinHttpWebSocketQueryCloseStatus: " + std::to_string(err));
                }
                else
                {
                    DisplayMessage("WebSocket close status: " + std::to_string(closestatus));

                    if (reasonLength > 0)
                    {
                        std::string reason(reasonBuffer, reasonBuffer + reasonLength);
                        DisplayMessage("WebSocket close reason: " + reason);
                    }
                }

                // Mark the WebSocket* object as closed
                httpContext->WebSocket->SetSocketStatus(WebSocketStatus::Closed);

                // The operation is always completed successfully even if we couldn't get the close status
                XAsyncComplete(
                    httpContext->AsyncBlock,
                    S_OK,
                    0
                    );
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
        {
            DisplayMessage("WinHttp Status: SENDING_REQUEST");

            auto httpContext = reinterpret_cast<WinHttpRequestContext*>(context);
            if (httpContext != nullptr && httpContext->SecurityInformation != nullptr)
            {
                auto hr = XNetworkingVerifyServerCertificate(
                    httpContext->HttpRequest,
                    httpContext->SecurityInformation
                    );

                if (FAILED(hr))
                {
                    DisplayMessage("XNetworkingVerifyServerCertificate failed: " + std::to_string(hr));

                    // Place the HRESULT in the context so we can return the correct value
                    httpContext->TaskResult = hr;

                    WinHttpCloseHandle(httpContext->HttpRequest);
                }
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:
        {
            DisplayMessage("WinHttp Status: RESPONSE_RECEIVED");

            auto httpContext = reinterpret_cast<WinHttpRequestContext*>(context);
            if (httpContext != nullptr)
            {
                uint32_t statusCode;
                DWORD size = sizeof(statusCode);

                auto result = WinHttpQueryHeaders(
                    httpContext->HttpRequest,
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    nullptr,
                    &statusCode,
                    &size,
                    nullptr
                    );

                if (result)
                {
                    httpContext->ResponseStatusCode = statusCode;
                }
                else
                {
                    DisplayMessage("Unable to retrieve HTTP Status Code: " + std::to_string(GetLastError()));
                }
            }
            break;
        }
#pragma region Unused Callback Status Cases
        case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        {
            DisplayMessage("WinHttp Status: HANDLE_CLOSING");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_RESOLVING_NAME:
        {
            DisplayMessage("WinHttp Status: RESOLVING_NAME");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_NAME_RESOLVED:
        {
            DisplayMessage("WinHttp Status: NAME_RESOLVED");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER:
        {
            DisplayMessage("WinHttp Status: CONNECTING_TO_SERVER");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER:
        {
            DisplayMessage("WinHttp Status: CONNECTED_TO_SERVER");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
        {
            DisplayMessage("WinHttp Status: REQUEST_SENT");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE:
        {
            DisplayMessage("WinHttp Status: RECEIVING_RESPONSE");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION:
        {
            DisplayMessage("WinHttp Status: CLOSING_CONNECTION");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED:
        {
            DisplayMessage("WinHttp Status: CONNECTION_CLOSED");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_HANDLE_CREATED:
        {
            DisplayMessage("WinHttp Status: HANDLE_CREATED");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_DETECTING_PROXY:
        {
            DisplayMessage("WinHttp Status: DETECTING_PROXY");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_REDIRECT:
        {
            DisplayMessage("WinHttp Status: REDIRECT");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE:
        {
            DisplayMessage("WinHttp Status: INTERMEDIATE_RESPONSE");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
        {
            DisplayMessage("WinHttp Status: SECURE_FAILURE");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE:
        {
            DisplayMessage("WinHttp Status: GETPROXYFORURL_COMPLETE");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE:
        {
            DisplayMessage("WinHttp Status: SHUTDOWN_COMPLETE");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_SETTINGS_WRITE_COMPLETE:
        {
            DisplayMessage("WinHttp Status: SETTINGS_WRITE_COMPLETE");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_SETTINGS_READ_COMPLETE:
        {
            DisplayMessage("WinHttp Status: SETTINGS_READ_COMPLETE");
            break;
        }
#pragma endregion
        default:
        {
            DisplayMessage("Unhandled WinHttp Callback Status value!");
            break;
        }
    }
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmicrosoft-cast"
#endif

HRESULT WinHttpManager::OpenWebSocketAsync(XAsyncBlock* asyncBlock, XUserHandle user, const wchar_t* url, WINHTTP_EXTENDED_HEADER* headers, DWORD headerCount)
{
    // Initiate an asynchronous opration to connect to a WebSocket endpoint

    if (!NetworkAvailable())
    {
        return HRESULT_FROM_WIN32(ERROR_NO_NETWORK);
    }

    auto context = new WebSocketRequestContext(
        user,
        L"GET",
        url,
        headers,
        headerCount,
        asyncBlock
        );

    auto hr = XAsyncBegin(
        asyncBlock,
        context,
        XAsyncOpenWebSocketProvider,
        __FUNCTION__,
        XAsyncOpenWebSocketProvider
        );

    if (SUCCEEDED(hr))
    {
        hr = XAsyncSchedule(
            asyncBlock,
            0
            );
    }

    if (FAILED(hr))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT WinHttpManager::OpenWebSocketAsyncResult(XAsyncBlock* asyncBlock, WebSocket** webSocket)
{
    // Retrieve the results of an OpenSocketAsync operation
    // Returns a WebSocket* object

    if (asyncBlock == nullptr || webSocket == nullptr)
    {
        return E_INVALIDARG;
    }

    WebSocketRequestContext* socketContext = nullptr;

    auto hr = XAsyncGetResult(
        asyncBlock,
        XAsyncOpenWebSocketProvider,
        sizeof(socketContext),
        &socketContext,
        nullptr
        );

    if (FAILED(hr))
    {
        return hr;
    }

    if (socketContext == nullptr)
    {
        return E_UNEXPECTED;
    }

    // Return an object that represents the opened WebSocket
    *webSocket = new WebSocket(
        socketContext->ResultHandle,
        socketContext->HttpConnection
        );

    return S_OK;
}

HRESULT WinHttpManager::WriteToWebSocketAsync(XAsyncBlock* asyncBlock, WebSocket* webSocket, const uint8_t* data, DWORD length)
{
    // Begin an asynchronous write operation on a WebSocket

    if (!NetworkAvailable())
    {
        return HRESULT_FROM_WIN32(ERROR_NO_NETWORK);
    }

    auto context = new WebSocketDataContext(
        asyncBlock,
        webSocket,
        data,
        length
        );

    auto hr = XAsyncBegin(
        asyncBlock,
        context,
        XAsyncWebSocketWriteProvider,
        __FUNCTION__,
        XAsyncWebSocketWriteProvider
        );

    if (SUCCEEDED(hr))
    {
        hr = XAsyncSchedule(
            asyncBlock,
            0
            );
    }

    if (FAILED(hr))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT WinHttpManager::WriteToWebSocketAsyncResult(XAsyncBlock* asyncBlock, DWORD* bytesWritten)
{
    // Retreieve the results of a WriteToWebSockAsync operation
    // Returns the number of bytes successfully written

    if (asyncBlock == nullptr || bytesWritten == nullptr)
    {
        return E_INVALIDARG;
    }

    *bytesWritten = 0;

    auto hr = XAsyncGetResult(
        asyncBlock,
        XAsyncWebSocketWriteProvider,
        sizeof(DWORD),
        bytesWritten,
        nullptr
        );

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

HRESULT WinHttpManager::ReadFromWebSocketAsync(XAsyncBlock* asyncBlock, WebSocket* webSocket)
{
    // Begin an asynchronous read operation on a WebSocket

    if (!NetworkAvailable())
    {
        return HRESULT_FROM_WIN32(ERROR_NO_NETWORK);
    }

    if (asyncBlock == nullptr || webSocket == nullptr)
    {
        return E_INVALIDARG;
    }

    auto context = new WebSocketDataContext(
        asyncBlock,
        webSocket,
        nullptr,
        0
        );

    auto hr = XAsyncBegin(
        asyncBlock,
        context,
        XAsyncWebSocketReadProvider,
        __FUNCTION__,
        XAsyncWebSocketReadProvider
        );

    if (SUCCEEDED(hr))
    {
        hr = XAsyncSchedule(
            asyncBlock,
            0
            );
    }

    if (FAILED(hr))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;

}

HRESULT WinHttpManager::ReadFromWebSocketAsyncResultSize(XAsyncBlock* asyncBlock, DWORD* length)
{
    // Retrieve the size of the data buffer needed to complete the ReadFromWebSocketAsync operation

    if (asyncBlock == nullptr || length == nullptr)
    {
        return E_INVALIDARG;
    }

    size_t bufferSize = 0;

    auto hr = XAsyncGetResultSize(
        asyncBlock,
        &bufferSize
        );

    if (FAILED(hr))
    {
        return hr;
    }

    // Return the needed buffer size
    *length = static_cast<DWORD>(bufferSize);

    return S_OK;
}

HRESULT WinHttpManager::ReadFromWebSocketAsyncResult(XAsyncBlock* asyncBlock, uint8_t* buffer, size_t size)
{
    // Retrieve the data buffer returned from the ReadFromWebSocketAsync operation

    if (asyncBlock == nullptr || buffer == nullptr)
    {
        return E_INVALIDARG;
    }

    auto hr = XAsyncGetResult(
        asyncBlock,
        XAsyncWebSocketReadProvider,
        size,
        buffer,
        nullptr
        );

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

HRESULT WinHttpManager::CloseWebSocketAsync(XAsyncBlock* asyncBlock, WebSocket* webSocket)
{
    // Begin an asynchronous operation to gracefully close a WebSocket

    if (!NetworkAvailable())
    {
        return HRESULT_FROM_WIN32(ERROR_NO_NETWORK);
    }

    auto context = new WebSocketCloseContext(
        asyncBlock,
        webSocket
        );

    auto hr = XAsyncBegin(
        asyncBlock,
        context,
        XAsyncCloseWebSocketProvider,
        __FUNCTION__,
        XAsyncCloseWebSocketProvider
        );

    if (SUCCEEDED(hr))
    {
        hr = XAsyncSchedule(
            asyncBlock,
            0
            );
    }

    if (FAILED(hr))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT WinHttpManager::CloseWebSocketAsyncResult(XAsyncBlock* asyncBlock, USHORT* result)
{
    // Retrieve the results of a CloseWebSocketAsync operation

    if (asyncBlock == nullptr || result == nullptr)
    {
        return E_INVALIDARG;
    }

    WebSocketCloseContext* context = nullptr;

    auto hr = XAsyncGetResult(
        asyncBlock,
        XAsyncCloseWebSocketProvider,
        sizeof(context),
        &context,
        nullptr
        );

    if (FAILED(hr))
    {
        return hr;
    }

    // Return the WINHTTP_WEB_SOCKET_CLOSE_STATUS value
    *result = context->Result;

    return S_OK;
}

HRESULT WinHttpManager::MakeHttpRequestAsync(XAsyncBlock* asyncBlock, XUserHandle user, const wchar_t* verb, const wchar_t* uri, WINHTTP_EXTENDED_HEADER* headers, DWORD headerCount, uint8_t* bodyContent, DWORD bodyLength)
{
    // Begin an asynchronous opration for a simple HTTP request

    if (!NetworkAvailable())
    {
        return HRESULT_FROM_WIN32(ERROR_NO_NETWORK);
    }

    auto context = new WinHttpRequestContext(
        user,
        verb,
        uri,
        headers,
        headerCount,
        bodyContent,
        bodyLength,
        asyncBlock
        );

    auto hr = XAsyncBegin(
        asyncBlock,
        context,
        XAsyncHttpRequestProvider,
        __FUNCTION__,
        XAsyncHttpRequestProvider
        );

    if (SUCCEEDED(hr))
    {
        hr = XAsyncSchedule(
            asyncBlock,
            0
            );
    }

    if (FAILED(hr))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT WinHttpManager::MakeHttpRequestAsyncResult(XAsyncBlock* asyncBlock, WinHttpRequest** request)
{
    // Retrieve the results from a MakeHttpRequestAsync operation
    // Returns a WinHttpRequest* object

    if (asyncBlock == nullptr || request == nullptr)
    {
        return E_INVALIDARG;
    }

    WinHttpRequestContext* context = nullptr;

    auto hr = XAsyncGetResult(
        asyncBlock,
        XAsyncHttpRequestProvider,
        sizeof(context),
        &context,
        nullptr
        );

    if (FAILED(hr))
    {
        return hr;
    }

    if (context == nullptr)
    {
        return E_UNEXPECTED;
    }

    *request = new WinHttpRequest(
        context->HttpRequest,
        context->HttpConnection,
        context->ResponseStatusCode,
        context->Response.data(),
        context->Response.size(),
        context->ResponseHeaders,
        context->HeaderLength
        );

    return S_OK;
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

HRESULT WinHttpManager::XAsyncCloseWebSocketProvider(XAsyncOp op, const XAsyncProviderData* data)
{
    // XAsync provider for CloseWebSocketAsync operations
    switch (op)
    {
        case XAsyncOp::Cleanup:
        {
            auto context = reinterpret_cast<WebSocketCloseContext*>(data->context);

            {
                std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);
                m_httpHandleQueue.erase(context->WebSocket->m_webSocket);
            }

            delete context;

            return S_OK;
        }

        case XAsyncOp::GetResult:
            *reinterpret_cast<WebSocketCloseContext**>(data->buffer) = reinterpret_cast<WebSocketCloseContext*>(data->context);
            return S_OK;
        
        case XAsyncOp::DoWork:
        {
            auto httpContext = static_cast<WebSocketCloseContext*>(data->context);

            if (httpContext->WebSocket->GetConnectionStatus() != WebSocketStatus::Connected)
            {
                // The WebSocket isn't in a connected state

                XAsyncComplete(
                    httpContext->AsyncBlock,
                    E_INVALIDARG,
                    0
                    );

                return E_INVALIDARG;
            }

            {
                // Store the context by handle for lookup during status callbacks
                std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);
                m_httpHandleQueue[httpContext->WebSocket->m_webSocket] = httpContext;
            }

            // Initiate a graceful shutdown of the websocket
            DWORD err = WinHttpWebSocketClose(
                httpContext->WebSocket->m_webSocket,
                WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS,
                nullptr,
                0
                );

            if (err != ERROR_SUCCESS)
            {
                DisplayMessage("WinHttp Error during WinHttpWebSocketClose: " + std::to_string(err));
    
                XAsyncComplete(
                    httpContext->AsyncBlock,
                    HRESULT_FROM_WIN32(err),
                    0
                    );

                return HRESULT_FROM_WIN32(err);
            }

            return E_PENDING;
        }

        default:
            break;
    }

    return S_OK;
}

HRESULT WinHttpManager::XAsyncHttpRequestProvider(XAsyncOp op, const XAsyncProviderData* data)
{
    // Async provider for MakeHttpRequestAsync
    switch (op)
    {
        case XAsyncOp::Cleanup:
        {
            auto context = reinterpret_cast<WinHttpRequestContext*>(data->context);

            if (context->ResponseHeaders)
            {
                delete[] context->ResponseHeaders;
            }

            delete context;

            return S_OK;
        }

        case XAsyncOp::GetResult:
            *reinterpret_cast<WinHttpRequestContext**>(data->buffer) = reinterpret_cast<WinHttpRequestContext*>(data->context);
            return S_OK;

        case XAsyncOp::DoWork:
        {
            auto context = reinterpret_cast<WinHttpRequestContext*>(data->context);

            return context->QuerySecurityInformation();
        }

        default:
            break;
    }

    return S_OK;
}

HRESULT WinHttpManager::XAsyncOpenWebSocketProvider(XAsyncOp op, const XAsyncProviderData* data)
{
    // Async provider for OpenWebSocketAsync
    auto context = reinterpret_cast<WebSocketRequestContext*>(data->context);

    switch (op)
    {
        case XAsyncOp::Cleanup:
            delete context;
            return S_OK;

        case XAsyncOp::GetResult:
            *reinterpret_cast<WebSocketRequestContext**>(data->buffer) = context;
            return S_OK;

        case XAsyncOp::DoWork:
            return context->QuerySecurityInformation();

        default:
            break;
    }

    return S_OK;
}

HRESULT WinHttpManager::XAsyncWebSocketWriteProvider(XAsyncOp op, const XAsyncProviderData* data)
{
    // Async provider for WriteToWebSocketAsync
    switch (op)
    {
        case XAsyncOp::Cleanup:
        {
            auto context = reinterpret_cast<WebSocketDataContext*>(data->context);

            {
                std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);
                m_httpHandleQueue.erase(context->WebSocket->m_webSocket);
            }

            delete context;

            return S_OK;
        }

        case XAsyncOp::GetResult:
            *reinterpret_cast<DWORD*>(data->buffer) = reinterpret_cast<WebSocketDataContext*>(data->context)->BytesTransferred;
            return S_OK;

        case XAsyncOp::DoWork:
        {
            auto context = reinterpret_cast<WebSocketDataContext*>(data->context);

            if (context->WebSocket->GetConnectionStatus() != WebSocketStatus::Connected)
            {
                // WebSocket isn't connected
                XAsyncComplete(
                    context->AsyncBlock,
                    E_INVALIDARG,
                    0
                    );

                return E_INVALIDARG;
            }

            {
                std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);
                m_httpHandleQueue[context->WebSocket->m_webSocket] = context;
            }

            // Start the WinHttp send
            DWORD err = WinHttpWebSocketSend(
                context->WebSocket->m_webSocket,
                WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
                context->DataBuffer.data(),
                static_cast<DWORD>(context->DataBuffer.size())
                );

            if (err != ERROR_SUCCESS)
            {
                DisplayMessage("WinHttp Error during WinHttpWebSocketSend: " + std::to_string(err));
                context->WebSocket->SetSocketStatus(WebSocketStatus::Error);

                XAsyncComplete(
                    context->AsyncBlock,
                    HRESULT_FROM_WIN32(GetLastError()),
                    0
                    );

                return HRESULT_FROM_WIN32(GetLastError());
            }

            return E_PENDING;
        }

        default:
            break;
    }

    return S_OK;
}

HRESULT WinHttpManager::XAsyncWebSocketReadProvider(XAsyncOp op, const XAsyncProviderData* data)
{
    // Async provider for ReadFromWebSocketAsync
    switch (op)
    {
        case XAsyncOp::Cleanup:
        {
            auto context = reinterpret_cast<WebSocketDataContext*>(data->context);

            {
                std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);
                m_httpHandleQueue.erase(context->WebSocket->m_webSocket);
            }

            delete context;

            return S_OK;
        }

        case XAsyncOp::GetResult:
        {
            auto context = reinterpret_cast<WebSocketDataContext*>(data->context);

            // Copy the data buffer to the caller's buffer
            memcpy_s(
                data->buffer,
                data->bufferSize,
                context->DataBuffer.data(),
                context->BytesTransferred
                );

            return S_OK;
        }

        case XAsyncOp::DoWork:
        {
            auto context = reinterpret_cast<WebSocketDataContext*>(data->context);

            context->DataBuffer.clear();
            context->DataBuffer.resize(s_readBufferSize);

            {
                std::lock_guard<std::mutex> lock(m_httpHandleQueueLock);
                m_httpHandleQueue[context->WebSocket->m_webSocket] = context;
            }

            // Start the WinHttp read
            DWORD err = WinHttpWebSocketReceive(
                context->WebSocket->m_webSocket,
                context->DataBuffer.data(),
                static_cast<DWORD>(context->DataBuffer.size()),
                nullptr,
                nullptr
                );

            if (err != ERROR_SUCCESS)
            {
                DisplayMessage("WinHttp Error during WinHttpWebSocketReceive: " + std::to_string(err));
                context->WebSocket->SetSocketStatus(WebSocketStatus::Error);

                XAsyncComplete(
                    context->AsyncBlock,
                    HRESULT_FROM_WIN32(GetLastError()),
                    0
                    );

                return HRESULT_FROM_WIN32(GetLastError());
            }

            return E_PENDING;
        }

        default:
            break;
    }

    return S_OK;
}

HRESULT WinHttpManager::QuerySecurityInformation(WinHttpRequestContext* context)
{
    auto async = new XAsyncBlock{};

    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        auto context = reinterpret_cast<WinHttpRequestContext*>(async->context);

        size_t securityInformationBufferByteCount = 0;

        // Get the size of the security info
        auto hr = XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize(
            async,
            &securityInformationBufferByteCount
            );

        if (FAILED(hr))
        {
            DisplayMessage("Error during XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize: " + std::to_string(hr));

            XAsyncComplete(
                context->AsyncBlock,
                hr,
                0
                );

            delete async;

            return;
        }

        size_t bytesUsed = 0;

        context->SecurityInformationBuffer.resize(securityInformationBufferByteCount);

        // Read in the results
        // SecurityInformation is a typed pointer to the data in the SecurityInformationBuffer which is the memory that should
        // be free'd when no longer needed
        hr = XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult(
            async,
            securityInformationBufferByteCount,
            &bytesUsed,
            context->SecurityInformationBuffer.data(),
            &context->SecurityInformation
            );

        if (SUCCEEDED(hr))
        {
            if (context->AuthUser != nullptr)
            {
                // If a user object is provided, use it to authorization headers to the request
                context->BeginAuthorization();
            }
            else
            {
                // Perform the request without user auth
                context->BeginRequest();
            }
        }
        else
        {
            DisplayMessage("Error during XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult: " + std::to_string(hr));

            XAsyncComplete(
                context->AsyncBlock,
                hr,
                0
                );
        }

        delete async;
    };

    auto hr = XNetworkingQuerySecurityInformationForUrlUtf16Async(
        context->Url.c_str(),
        async
        );

    if (FAILED(hr))
    {
        DisplayMessage("Error during XNetworkingQuerySecurityInformationForUrlUtf16Async: " + std::to_string(hr));

        XAsyncComplete(
            context->AsyncBlock,
            hr,
            0
            );

        delete async;

        return hr;
    }

    return E_PENDING;
}

HRESULT WinHttpManager::BeginAuthorization(WinHttpRequestContext* context)
{
    auto async = new XAsyncBlock{};

    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {

        auto context = reinterpret_cast<WinHttpRequestContext*>(async->context);

        size_t bufferSize = 0;

        // Get the size of the buffer, waiting for the operation to complete
        auto hr = XUserGetTokenAndSignatureUtf16ResultSize(
            async,
            &bufferSize
            );

        if (FAILED(hr))
        {
            DisplayMessage("Error during XUserGetTokenAndSignatureUtf16ResultSize: " + std::to_string(hr));

            XAsyncComplete(
                context->AsyncBlock,
                hr,
                0
                );

            if (context->HeadersForSignature != nullptr)
            {
                delete context->HeadersForSignature;
            }

            delete async;

            return;
        }

        size_t resultBufferUsed = 0;

        context->TokenAndSignatureBuffer.resize(bufferSize);

        // Read the token and signature
        hr = XUserGetTokenAndSignatureUtf16Result(
            async,
            bufferSize,
            context->TokenAndSignatureBuffer.data(),
            &context->TokenAndSignature,
            &resultBufferUsed
            );

        if (SUCCEEDED(hr))
        {
            // Now we have the token and/or signature so we're ready to make the request
            // or retry the request if it failed authorization previously
            if (context->Retry)
            {
                context->RetryRequest();
            }
            else
            {
                context->BeginRequest();
            }
        }
        else
        {
            DisplayMessage("Error during XUserGetTokenAndSignatureUtf16Result: " + std::to_string(hr));

            XAsyncComplete(
                context->AsyncBlock,
                hr,
                0
                );
        }

        if (context->HeadersForSignature != nullptr)
        {
            delete context->HeadersForSignature;
        }

        delete async;
    };

    // Get the URL components for the connection
    URL_COMPONENTS parts = {};

    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = static_cast<DWORD>(-1L);
    parts.dwSchemeLength = static_cast<DWORD>(-1L);

    auto result = WinHttpCrackUrl(
        context->Url.c_str(),
        0,
        0,
        &parts
        );

    if (result == false)
    {
        XAsyncComplete(
            context->AsyncBlock,
            HRESULT_FROM_WIN32(GetLastError()),
            0
            );

        if (context->HeadersForSignature != nullptr)
        {
            delete context->HeadersForSignature;
        }

        delete async;

        return HRESULT_FROM_WIN32(GetLastError());
    }

    std::wstring scheme(parts.lpszScheme, parts.lpszScheme + parts.dwSchemeLength);
    std::wstring host(parts.lpszHostName, parts.lpszHostName + parts.dwHostNameLength);
    std::wstring authUrl = scheme + L"://" + host;

    //  We need to convert the headers to GetTokenAndSignatureHeaders so that they can be included in
    //  the signature generation if the signature policy requires those headers
    size_t headerCount = context->RequestHeaders.size();

    context->HeadersForSignature = new XUserGetTokenAndSignatureUtf16HttpHeader[headerCount];

    for (uint64_t i = 0; i < headerCount; i++)
    {
        context->HeadersForSignature[i].name = context->RequestHeaders[i].pwszName;
        context->HeadersForSignature[i].value = context->RequestHeaders[i].pwszValue;
    }
    
    // Request the token and signature to add to the request headers
    // Force a token refresh if we've already received a 401 unauthorized response to this query
    auto hr = XUserGetTokenAndSignatureUtf16Async(
        context->AuthUser,
        context->ResponseStatusCode == 401 ? XUserGetTokenAndSignatureOptions::ForceRefresh : XUserGetTokenAndSignatureOptions::None,
        context->Verb.c_str(),
        authUrl.c_str(),
        headerCount,
        context->HeadersForSignature,
        context->BodySize,
        context->Body,
        async
        );

    if (FAILED(hr))
    {
        DisplayMessage("Error during XUserGetTokenAndSignatureUtf16Async: " + std::to_string(hr));

        XAsyncComplete(
            context->AsyncBlock,
            hr,
            0
            );

        if(context->HeadersForSignature != nullptr)
        {
            delete context->HeadersForSignature;
        }

        delete async;

        return hr;
    }

    return E_PENDING;
}

HRESULT WinHttpManager::BeginWebSocketRequest(WinHttpRequestContext* context)
{
    // The process for WebSockets starts the same as a basic Http request but
    // set the flag to add the websocket headers

    context->UpdgradeToWebSocket = true;

    return BeginHttpRequest(context);
}

HRESULT WinHttpManager::BeginHttpRequest(WinHttpRequestContext* context)
{
    // Get the URL components for the connection
    URL_COMPONENTS parts = {};

    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = static_cast<DWORD>(-1L);
    parts.dwUrlPathLength = static_cast<DWORD>(-1L);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1L);

    auto result = WinHttpCrackUrl(
        context->Url.c_str(),
        0,
        0,
        &parts
        );

    if (result == false)
    {
        XAsyncComplete(
            context->AsyncBlock,
            HRESULT_FROM_WIN32(GetLastError()),
            0
            );

        return HRESULT_FROM_WIN32(GetLastError());
    }

    std::wstring host(parts.lpszHostName, parts.lpszHostName + parts.dwHostNameLength);
    std::wstring path(parts.lpszUrlPath, parts.lpszUrlPath + parts.dwUrlPathLength);
    std::wstring extra(parts.lpszExtraInfo, parts.lpszExtraInfo + parts.dwExtraInfoLength);

    context->UriPath = path + extra;

    // Get the HttpSession to use
    HINTERNET httpSession = nullptr;

    auto hr = GetSessionBySecurityInformation(
        context->SecurityInformation,
        &httpSession
        );

    if (FAILED(hr))
    {
        XAsyncComplete(
            context->AsyncBlock,
            hr,
            0
            );

        return hr;
    }

    // Begin the WinHttp connection to the desired endpoint
    auto httpConnection = WinHttpConnect(
        httpSession,
        host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0
        );

    if (httpConnection == nullptr)
    {
        XAsyncComplete(
            context->AsyncBlock,
            HRESULT_FROM_WIN32(GetLastError()),
            0
            );

        return HRESULT_FROM_WIN32(GetLastError());
    }

    context->HttpConnection = httpConnection;

    return SendHttpRequest(context);
}

HRESULT WinHttpManager::RetryHttpRequest(WinHttpRequestContext* context)
{
    // Reset and reuse the context for the retry
    if (context->HttpRequest)
    {
        WinHttpCloseHandle(context->HttpRequest);
        context->HttpRequest = nullptr;
    }

    context->HaveContentLengthHeader = false;
    context->ContentLength = 0;
    context->ResponseBytesReceived = 0;
    context->ResponseStatusCode = 0;

    return SendHttpRequest(context);
}

HRESULT WinHttpManager::SendHttpRequest(WinHttpRequestContext* context)
{
    // Open the WinHttp request on the connection
    auto httpRequest = WinHttpOpenRequest(
        context->HttpConnection,
        context->Verb.c_str(),
        context->UriPath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
        );

    if (httpRequest == nullptr)
    {
        XAsyncComplete(
            context->AsyncBlock,
            HRESULT_FROM_WIN32(GetLastError()),
            0
            );

        return HRESULT_FROM_WIN32(GetLastError());
    }

    context->HttpRequest = httpRequest;

    if (context->UpdgradeToWebSocket)
    {
        // Tell the Request we want to become a websocket
        auto bResult = WinHttpSetOption(
            httpRequest,
            WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
            nullptr,
            0
            );

        if (bResult == false)
        {
            XAsyncComplete(
                context->AsyncBlock,
                HRESULT_FROM_WIN32(GetLastError()),
                0
                );

            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    // Add the token and signature if present
    if (context->TokenAndSignature != nullptr)
    {
        context->RequestHeaders.emplace_back(
            WINHTTP_EXTENDED_HEADER{ L"Authorization", context->TokenAndSignature->token }
            );

        //  If there is no signature policy this value will be null
        if (context->TokenAndSignature->signature != nullptr)
        {
            context->RequestHeaders.emplace_back(
                WINHTTP_EXTENDED_HEADER{ L"Signature", context->TokenAndSignature->signature }
                );
        }
    }

    // Add optional headers
    if (context->RequestHeaders.size() > 0)
    {
        //  WinHttpAddRequestHeadersEx is available on console, but not PC yet
        //  it will be released along with WINHTTP_SECURE_DEFAULTS in an upcoming
        //  GDK for desktop / PC.
        //  Until then we need to format the headers into a string to pass into
        //  the existing WinHttpAddRequestHeaders API
        auto formattedHeaders = FormatHeaderString(context->RequestHeaders);

        auto bResult = WinHttpAddRequestHeaders(
            httpRequest,
            formattedHeaders.data(),
            static_cast<DWORD>(formattedHeaders.size()),
            WINHTTP_ADDREQ_FLAG_ADD
            );

        if (bResult == false)
        {
            DisplayMessage("Error during WinHttpAddRequestHeaders: " + std::to_string(GetLastError()));

            XAsyncComplete(
                context->AsyncBlock,
                HRESULT_FROM_WIN32(GetLastError()),
                0
                );

            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    // Send the WinHttp request to the server
    auto bResult = WinHttpSendRequest(
        httpRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        reinterpret_cast<void*>(context->Body),
        context->BodySize,
        0,
        reinterpret_cast<DWORD_PTR>(context)
        );

    if (bResult == false)
    {
        XAsyncComplete(
            context->AsyncBlock,
            HRESULT_FROM_WIN32(GetLastError()),
            0
            );

        return HRESULT_FROM_WIN32(GetLastError());
    }

    // At this point the request will continue asynchronously based on
    // the callbacks in WinHttpStatusCallback()

    return E_PENDING;
}

HRESULT WinHttpManager::ProcessWebSocketRequest(WinHttpRequestContext* context)
{
    // This stage is the same for WebSocket and Http requests
    return ProcessHttpRequest(context);
}

HRESULT WinHttpManager::ProcessHttpRequest(WinHttpRequestContext* context)
{
    // Retrieve the server response
    auto bResult = WinHttpReceiveResponse(
        context->HttpRequest,
        nullptr
        );

    if (bResult == false)
    {
        XAsyncComplete(
            context->AsyncBlock,
            HRESULT_FROM_WIN32(GetLastError()),
            0
            );

        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Wait for the callback that the data is available
    return E_PENDING;
}

HRESULT WinHttpManager::CompleteHttpRequest(WinHttpRequestContext* context)
{
    // Check for failures due to an invalid token/signature and
    // force a refresh on the retry
    if (context->ResponseStatusCode == 401 && context->AuthUser != nullptr)
    {
        DisplayMessage("401 UNAUTHORIZED - Attempting to refresh user token");

        if (context->Retries < s_maxRequestRetries)
        {
            context->Retries++;
            context->Retry = true;

            return BeginAuthorization(context);
        }
        else
        {
            // Let the error bubble up to the caller
            context->Retry = false;
        }
    }

    // Complete the asynchronous operation
    XAsyncComplete(
        context->AsyncBlock,
        S_OK,
        sizeof(DWORD_PTR)
        );

    return S_OK;
}

HRESULT WinHttpManager::CompleteWebSocketRequest(WinHttpRequestContext* context)
{
    // Perform the conversion to a websocket
    auto webSocket = WinHttpWebSocketCompleteUpgrade(
        context->HttpRequest,
        0
        );

    if (webSocket == nullptr)
    {
        XAsyncComplete(
            context->AsyncBlock,
            HRESULT_FROM_WIN32(GetLastError()),
            0
            );

        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Store the WebSocket handle
    context->ResultHandle = webSocket;

    // The request can be closed now
    WinHttpCloseHandle(context->HttpRequest);
    context->HttpRequest = nullptr;

    // Complete the asynchronous operation
    XAsyncComplete(
        context->AsyncBlock,
        S_OK,
        sizeof(DWORD_PTR)
        );

    return S_OK;
}

//  This is a helper function to format the headers into a string to be used
//  with WinHttpAddRequestHeaders
std::wstring WinHttpManager::FormatHeaderString(std::vector<WINHTTP_EXTENDED_HEADER> headers)
{
    std::wstring headersString = L"";

    for (size_t i = 0; i < headers.size(); i++)
    {
        headersString.append(headers[i].pwszName);
        headersString.append(L": ");

        if (headers[i].pszValue == nullptr)
        {
            headersString.append(L"");
        }
        else
        {
            headersString.append(headers[i].pwszValue);
        }

        //  We don't need the newline on the last one, but
        //  do on the others.
        if (i <= headers.size())
        {
            headersString.append(L"\r\n");
        }
    }

    return headersString;
}
