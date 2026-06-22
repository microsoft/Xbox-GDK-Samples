//--------------------------------------------------------------------------------------
// WinHttpManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

class WebSocket;
class WinHttpRequest;

/////////////////////////////////////////////////////////////////////////////////////////
class WinHttpManager
{
public:
    // WinHttpManager is a static class
    WinHttpManager() = delete;
    WinHttpManager(const WinHttpManager&) = delete;
    WinHttpManager(WinHttpManager&&) = delete;
    WinHttpManager& operator=(const WinHttpManager&) = delete;
    WinHttpManager& operator=(WinHttpManager&&) = delete;
    ~WinHttpManager() = delete;

    // The read buffer for WinHttp should be 8k at a minimum
    static constexpr int s_readBufferSize = 8192;
    static constexpr int s_maxRequestRetries = 4;

    static bool NetworkAvailable();

    static void Reset();

    static HRESULT OpenWebSocketAsync(XAsyncBlock* asyncBlock, XUserHandle user, const wchar_t* url, WINHTTP_EXTENDED_HEADER* headers, DWORD headerCount);
    static HRESULT OpenWebSocketAsyncResult(XAsyncBlock* asyncBlock, WebSocket** webSocket);
    
    static HRESULT WriteToWebSocketAsync(XAsyncBlock* asyncBlock, WebSocket* webSocket, const uint8_t* data, DWORD length);
    static HRESULT WriteToWebSocketAsyncResult(XAsyncBlock* asyncBlock, DWORD* written);

    static HRESULT ReadFromWebSocketAsync(XAsyncBlock* asyncBlock, WebSocket* webSocket);
    static HRESULT ReadFromWebSocketAsyncResultSize(XAsyncBlock* asyncBlock, DWORD* length);
    static HRESULT ReadFromWebSocketAsyncResult(XAsyncBlock* asyncBlock, uint8_t* buffer, size_t size);

    static HRESULT CloseWebSocketAsync(XAsyncBlock* asyncBlock, WebSocket* webSocket);
    static HRESULT CloseWebSocketAsyncResult(XAsyncBlock* asyncBlock, USHORT* result);

    static HRESULT MakeHttpRequestAsync(XAsyncBlock* asyncBlock, XUserHandle user, const wchar_t* verb, const wchar_t* uri, WINHTTP_EXTENDED_HEADER* headers, DWORD headerCount, uint8_t* bodyContent, DWORD bodyLength);
    static HRESULT MakeHttpRequestAsyncResult(XAsyncBlock* asyncBlock, WinHttpRequest** buffer);

    static void CALLBACK WinHttpStatusCallback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID statusInfo, DWORD statusInfoLength);

    static HRESULT CALLBACK XAsyncOpenWebSocketProvider(XAsyncOp op, const XAsyncProviderData* data);
    static HRESULT CALLBACK XAsyncCloseWebSocketProvider(XAsyncOp op, const XAsyncProviderData* data);
    static HRESULT CALLBACK XAsyncHttpRequestProvider(XAsyncOp op, const XAsyncProviderData* data);
    static HRESULT CALLBACK XAsyncWebSocketWriteProvider(XAsyncOp op, const XAsyncProviderData* data);
    static HRESULT CALLBACK XAsyncWebSocketReadProvider(XAsyncOp op, const XAsyncProviderData* data);

private:
#pragma region Async Contexts
    /////////////////////////////////////////////////////////////////////////////////////
    struct ContextObject
    {
        virtual ~ContextObject() = default;
    };

    /////////////////////////////////////////////////////////////////////////////////////
    struct WinHttpRequestContext : ContextObject
    {
        WinHttpRequestContext(XUserHandle user, const wchar_t* verb, const wchar_t* url, WINHTTP_EXTENDED_HEADER* headers, DWORD headerCount, uint8_t* body, DWORD bodyLen, XAsyncBlock* asyncBlock) :
            Verb(verb),
            Url(url),
            ResponseBytesReceived(0),
            UpdgradeToWebSocket(false),
            ContentLength(0),
            HaveContentLengthHeader(false),
            Retry(false),
            TaskResult(S_OK),
            ResponseHeaders(nullptr),
            HeaderLength(0),
            ResponseStatusCode(0),
            Body(body),
            BodySize(bodyLen),
            Retries(0),
            AsyncBlock(asyncBlock),
            AuthUser(user),
            HttpConnection(nullptr),
            HttpRequest(nullptr),
            ResultHandle(nullptr),
            TokenAndSignature(nullptr),
            HeadersForSignature(nullptr),
            SecurityInformation(nullptr)
        {
            if (headers != nullptr && headerCount > 0)
            {
                RequestHeaders = std::vector<WINHTTP_EXTENDED_HEADER>(headers, headers + headerCount);
            }
        }

        virtual HRESULT QuerySecurityInformation() { return WinHttpManager::QuerySecurityInformation(this); }
        virtual HRESULT BeginAuthorization() { return WinHttpManager::BeginAuthorization(this); }
        virtual HRESULT BeginRequest() { return WinHttpManager::BeginHttpRequest(this); }
        virtual HRESULT RetryRequest() { return WinHttpManager::RetryHttpRequest(this); }
        virtual HRESULT ProcessRequest() { return WinHttpManager::ProcessHttpRequest(this); }
        virtual HRESULT CompleteRequest() { return WinHttpManager::CompleteHttpRequest(this); }

        std::wstring Verb;
        std::wstring Url;
        std::wstring UriPath;
        std::vector<WINHTTP_EXTENDED_HEADER> RequestHeaders;
        std::vector<uint8_t> Response;
        DWORD ResponseBytesReceived;
        bool UpdgradeToWebSocket;
        DWORD ContentLength;
        bool HaveContentLengthHeader;
        bool Retry;
        HRESULT TaskResult;
        wchar_t* ResponseHeaders;
        size_t HeaderLength;
        uint32_t ResponseStatusCode;
        uint8_t* Body;
        DWORD BodySize;
        DWORD Retries;
        XAsyncBlock* AsyncBlock;
        XUserHandle AuthUser;
        HINTERNET HttpConnection;
        HINTERNET HttpRequest;
        HINTERNET ResultHandle;
        XUserGetTokenAndSignatureUtf16Data* TokenAndSignature;
        XUserGetTokenAndSignatureUtf16HttpHeader* HeadersForSignature;
        XNetworkingSecurityInformation* SecurityInformation;
        std::vector<uint8_t> SecurityInformationBuffer;
        std::vector<uint8_t> TokenAndSignatureBuffer;
    };

    /////////////////////////////////////////////////////////////////////////////////////
    struct WebSocketRequestContext : public WinHttpRequestContext
    {
        WebSocketRequestContext(XUserHandle user, const wchar_t* verb, const wchar_t* url, WINHTTP_EXTENDED_HEADER* headers, DWORD headerCount, XAsyncBlock* asyncBlock) :
            WinHttpRequestContext(user, verb, url, headers, headerCount, nullptr, 0,  asyncBlock)
        {
        }

        HRESULT BeginRequest() override { return WinHttpManager::BeginWebSocketRequest(this); }
        HRESULT ProcessRequest() override { return WinHttpManager::ProcessWebSocketRequest(this); }
        HRESULT CompleteRequest() override { return WinHttpManager::CompleteWebSocketRequest(this); }
    };

    /////////////////////////////////////////////////////////////////////////////////////
    struct WebSocketDataContext : ContextObject
    {
        WebSocketDataContext(XAsyncBlock* asyncBlock, WebSocket* webSocket, const uint8_t* data, DWORD length) :
            AsyncBlock(asyncBlock),
            WebSocket(webSocket),
            DataLength(length),
            BytesTransferred(0)
        {
            if (data != nullptr) { DataBuffer.assign(data, data + length); }
        }

        XAsyncBlock* AsyncBlock;
        WebSocket* WebSocket;
        std::vector<uint8_t> DataBuffer;
        DWORD DataLength;
        DWORD BytesTransferred;
    };

    /////////////////////////////////////////////////////////////////////////////////////
    struct WebSocketCloseContext : ContextObject
    {
        WebSocketCloseContext(XAsyncBlock* asyncBlock, WebSocket* webSocket) :
            AsyncBlock(asyncBlock),
            WebSocket(webSocket)
        {
            Result = WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS;
        }

        XAsyncBlock* AsyncBlock;
        WebSocket* WebSocket;
        USHORT Result;
    };
#pragma endregion

    /////////////////////////////////////////////////////////////////////////////////////
    static HRESULT QuerySecurityInformation(WinHttpRequestContext* context);
    static HRESULT GetSessionBySecurityInformation(XNetworkingSecurityInformation* si, HINTERNET* session);

    static HRESULT BeginAuthorization(WinHttpRequestContext* context);

    static HRESULT BeginHttpRequest(WinHttpRequestContext* context);
    static HRESULT RetryHttpRequest(WinHttpRequestContext* context);
    static HRESULT SendHttpRequest(WinHttpRequestContext* context);
    static HRESULT ProcessHttpRequest(WinHttpRequestContext* context);
    static HRESULT CompleteHttpRequest(WinHttpRequestContext* context);

    static HRESULT BeginWebSocketRequest(WinHttpRequestContext* context);
    static HRESULT ProcessWebSocketRequest(WinHttpRequestContext* context);
    static HRESULT CompleteWebSocketRequest(WinHttpRequestContext* context);

    /////////////////////////////////////////////////////////////////////////////////////
    static std::mutex m_httpHandleQueueLock;
    static std::mutex m_httpSessionMapLock;
    static std::map<HINTERNET, ContextObject*> m_httpHandleQueue;
    static std::map<uint32_t, HINTERNET> m_httpSessions;
};

/////////////////////////////////////////////////////////////////////////////////////////
enum class WebSocketStatus
{
    Uninitialized,
    Connected,
    Closed,
    Error
};

/////////////////////////////////////////////////////////////////////////////////////////
class WebSocket
{
public:
    ~WebSocket()
    {
        if (m_webSocket) WinHttpCloseHandle(m_webSocket);
        if (m_httpConnection) WinHttpCloseHandle(m_httpConnection);
    }

    WebSocketStatus GetConnectionStatus() const { return m_connectionStatus; }

private:
    WebSocket(HINTERNET webSocket, HINTERNET httpConnection) :
        m_webSocket(webSocket),
        m_httpConnection(httpConnection),
        m_connectionStatus(WebSocketStatus::Connected)
    {
    }

    void SetSocketStatus(WebSocketStatus status) { m_connectionStatus = status; }

    HINTERNET m_webSocket;
    HINTERNET m_httpConnection;
    WebSocketStatus m_connectionStatus;

    friend class WinHttpManager;
};

/////////////////////////////////////////////////////////////////////////////////////////
class WinHttpRequest
{
public:
    ~WinHttpRequest()
    {
        if (m_httpRequest) WinHttpCloseHandle(m_httpRequest);
        if (m_httpConnection) WinHttpCloseHandle(m_httpConnection);
    }

    wchar_t* GetHeaders() { return m_headers; }
    size_t GetHeaderLength() { return m_headerLength; }

    uint8_t* GetBody() { return m_responseBody; }
    size_t GetBodyLength() { return m_responseLength; }

    uint32_t GetStatusCode() { return m_httpStatusCode; }

private:
    WinHttpRequest(HINTERNET request, HINTERNET connection, uint32_t status, uint8_t* body, size_t bodyLen, wchar_t* headers, size_t headerLen) :
        m_httpRequest(request),
        m_httpConnection(connection),
        m_httpStatusCode(status),
        m_responseBody(body),
        m_responseLength(bodyLen),
        m_headers(headers),
        m_headerLength(headerLen)
    {
    }

    HINTERNET m_httpRequest;
    HINTERNET m_httpConnection;
    uint32_t m_httpStatusCode;
    uint8_t* m_responseBody;
    size_t m_responseLength;
    wchar_t* m_headers;
    size_t m_headerLength;

    friend class WinHttpManager;
};

/////////////////////////////////////////////////////////////////////////////////////////
void DisplayMessage(std::string message);

}
