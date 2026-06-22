//--------------------------------------------------------------------------------------
// File: UdpSocket.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "UdpSocket.h"

#include "Debug.h"

#include <charconv>
#include <memory>
#include <string>

#include <WinSock2.h>
#include <WS2tcpip.h>

namespace ATG
{
    bool ResolveAddressFromHost(std::string_view addr, SOCKADDR_IN& outAddr)
    {
        ADDRINFO hints{}, * result;

        ZeroMemory(&outAddr, sizeof(outAddr));

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_ALL;

        std::string address(addr.begin(), addr.end());

        auto errcode = getaddrinfo(address.c_str(), NULL, &hints, &result);
        if (errcode != 0)
        {
            return false;
        }

        bool found = false;
        auto orig = result;

        while (result)
        {
            if (result->ai_family == AF_INET)
            {
                CopyMemory(&outAddr, result->ai_addr, result->ai_addrlen);
                found = true;
                break;
            }

            result = result->ai_next;
        }

        freeaddrinfo(orig);

        return found;
    }

    bool GetLocalNetworkAddress(SOCKADDR_IN& outAddr)
    {
        char host[255] = {};
        auto err = gethostname(host, 255);

        if (err == SOCKET_ERROR)
        {
            return false;
        }

        auto result = ResolveAddressFromHost(host, outAddr);

        return result;
    }

    SOCKADDR AddressFromString(std::string_view address)
    {
        auto index = address.find_first_of(':');

        SOCKADDR addr;

        SOCKADDR_IN* addrIn = reinterpret_cast<SOCKADDR_IN*>(&addr);

        if (index == std::string_view::npos)
        {
            ResolveAddressFromHost(address, *addrIn);
        }
        else
        {
            auto s = std::string_view(&*address.begin(), index);
            ResolveAddressFromHost(s, *addrIn);
            std::from_chars(address.data() + index + 1, address.data() + address.size(), addrIn->sin_port);
            addrIn->sin_port = htons(addrIn->sin_port);
        }

        return addr;
    }

    std::string AddressToString(const SOCKADDR& address)
    {
        auto addr = reinterpret_cast<const SOCKADDR_IN*>(&address);
        char buffer[32];
        sprintf_s(buffer, "%u.%u.%u.%u:%u",
            addr->sin_addr.S_un.S_un_b.s_b1,
            addr->sin_addr.S_un.S_un_b.s_b2,
            addr->sin_addr.S_un.S_un_b.s_b3,
            addr->sin_addr.S_un.S_un_b.s_b4,
            ntohs(addr->sin_port));

        return buffer;
    }

    HRESULT UdpSocket::Create(uint16_t port, UdpSocket** socket) noexcept
    {
        DEBUGLOG("UdpSocket::Create");

        auto wsaSocket = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);

        if (wsaSocket == INVALID_SOCKET)
        {
            return HRESULT_FROM_WIN32(static_cast<uint32_t>(WSAGetLastError()));
        }
        SOCKADDR localAddress{};
        SOCKADDR_IN* address = reinterpret_cast<SOCKADDR_IN*>(&localAddress);

        address->sin_port = htons(port);
        address->sin_family = AF_INET;

        int result = bind(wsaSocket, &localAddress, sizeof(m_localAddress));

        if (result != 0)
        {
            return HRESULT_FROM_WIN32(static_cast<uint32_t>(WSAGetLastError()));
        }

        auto socketPtr = new UdpSocket(wsaSocket);
        std::unique_ptr<UdpSocket> socketObj(socketPtr);

        socketObj->m_threadPoolIo = CreateThreadpoolIo(
            reinterpret_cast<HANDLE>(wsaSocket),
            [](PTP_CALLBACK_INSTANCE, void* context, void* overlapped, ULONG result, ULONG_PTR bytesTransfered, PTP_IO)
            {
                if (result != ERROR_OPERATION_ABORTED)
                {
                    auto socket = reinterpret_cast<UdpSocket*>(context);

                    HRESULT hr = socket->RecvMessage(reinterpret_cast<LPOVERLAPPED>(overlapped), result, bytesTransfered);
                    if (FAILED(hr))
                    {
                        DEBUGLOG("RecvMessage returned HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());

                        socket->SetError(hr);
                    }
                }
            },
            socketObj.get(),
            nullptr);

        if (socketObj->m_threadPoolIo == nullptr)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        HRESULT hr = socketObj->RecvNextMessage();
        if (FAILED(hr))
        {
            return hr;
        }

        *socket = socketObj.get();
        socketObj.release();
        return S_OK;
    }

    UdpSocket::UdpSocket(SOCKET socket) noexcept :
        m_socket(socket)
    {
        DEBUGLOG("UdpSocket::UdpSocket");

        int size = sizeof(m_localAddress);
        getsockname(m_socket, &m_localAddress, &size);
    }

    UdpSocket::~UdpSocket() noexcept
    {
        DEBUGLOG("UdpSocket::~UdpSocket");

        if (m_threadPoolIo)
        {
            CloseThreadpoolIo(m_threadPoolIo);
            m_threadPoolIo = nullptr;
        }

        {
            // Clear these out to prevent any look ups from happening during shutdown
            std::lock_guard lock(m_receiversLock);
            m_tokenRegistationLookup.clear();
            m_addressLookup.clear();
        }

        if (m_acceptTaskQueue)
        {
            XTaskQueueCloseHandle(m_acceptTaskQueue);
        }

        if (m_socket)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
    }

    void UdpSocket::GetAddress(SOCKADDR* address) const noexcept
    {
        DEBUGLOG("UdpSocket::GetAddress");

        memcpy(address, &m_localAddress, sizeof(SOCKADDR));
    }

    void UdpSocket::AcceptConnections(UdpAcceptConnectionHandler callback, void* context, XTaskQueueHandle queue) noexcept
    {
        DEBUGLOG("UdpSocket::AcceptConnections");

        m_acceptCallback = callback;
        m_acceptCallbackContext = context;

        if (m_acceptTaskQueue)
        {
            XTaskQueueCloseHandle(m_acceptTaskQueue);
            m_acceptTaskQueue = nullptr;
        }

        if (queue)
        {
            XTaskQueueDuplicateHandle(queue, &m_acceptTaskQueue);
        }
    }

    HRESULT UdpSocket::SendTo(const SOCKADDR* destination, const SocketPayload* payload) noexcept
    {
        DEBUGLOG("UdpSocket::SendTo");

        if (FAILED(m_internalError))
        {
            return m_internalError;
        }

        SocketPayload empty{};

        if (!payload)
        {
            payload = &empty;
        }

        WSABUF wsaBuf{ static_cast<ULONG>(payload->Size()), const_cast<char*>(reinterpret_cast<const char*>(payload->payload.data())) };

        DWORD bytesSent{ 0 };

        int result = WSASendTo(m_socket, &wsaBuf, 1, &bytesSent, 0, destination, sizeof(SOCKADDR), nullptr, nullptr);
        if (result == SOCKET_ERROR)
        {
            return HRESULT_FROM_WIN32(static_cast<uint32_t>(WSAGetLastError()));
        }
        else
        {
            DEBUGLOG("UdpSocket::SendTo: Send %d bytes", bytesSent);
            return S_OK;
        }
    }

    HRESULT UdpSocket::RegisterDataReceiver(const SOCKADDR* source, void* context, SocketPayloadReceiverCallback callback, XTaskQueueHandle queue, XTaskQueueRegistrationToken* token) noexcept
    {
        DEBUGLOG("UdpSocket::RegisterDataReceiver");

        if (FAILED(m_internalError))
        {
            return m_internalError;
        }

        std::lock_guard lock(m_receiversLock);

        auto id = m_nextId++;

        token->token = id;

        ConnectionDataReceiver receiver{
            *source,
            *token,
            context,
            queue,
            callback
        };

        auto&& [iter, result] = m_tokenRegistationLookup.insert({ id, receiver });

        if (result)
        {
            m_addressLookup.insert({ *source, &iter->second });
            return S_OK;
        }
        else
        {
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
    }

    HRESULT UdpSocket::UnregisterDataReceiver(XTaskQueueRegistrationToken token) noexcept
    {
        DEBUGLOG("UdpSocket::UnregisterDataReceiver");

        std::lock_guard lock(m_receiversLock);
        auto iter = m_tokenRegistationLookup.find(token.token);

        if (iter != m_tokenRegistationLookup.end())
        {
            m_addressLookup.erase(iter->second.source);
            m_tokenRegistationLookup.erase(iter);
            return S_OK;
        }
        else
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }
    }

    HRESULT UdpSocket::RecvMessage(LPOVERLAPPED, unsigned long result, size_t bytesTransfered) noexcept
    {
        if (result == NO_ERROR)
        {
            std::lock_guard lock(m_receiversLock);

            auto receiverIter = m_addressLookup.find(m_recvContext.sourceAddress);

            if (receiverIter != m_addressLookup.end())
            {
                struct EventCallbackContext
                {
                    UdpSocket* socket{ nullptr };
                    ConnectionDataReceiver* receiver{ nullptr };
                    SocketPayload buffer{};
                };

                auto context = std::make_unique<EventCallbackContext>();
                context->socket = this;
                context->receiver = receiverIter->second;
                context->buffer = m_recvContext.buffer;
                context->buffer.payload.resize(bytesTransfered);

                auto& receiver = receiverIter->second;

                HRESULT hr = XTaskQueueSubmitCallback(receiver->queue, XTaskQueuePort::Completion, context.get(),
                    [](void* context, bool cancelled)
                {
                    std::unique_ptr<EventCallbackContext> eventContext(reinterpret_cast<EventCallbackContext*>(context));

                    if (!cancelled)
                    {
                        auto receiver = eventContext->receiver;
                        receiver->callback(eventContext->socket, &receiver->source, &eventContext->buffer, receiver->context);
                    }
                });

                if (SUCCEEDED(hr))
                {
                    context.release();
                }
                else
                {
                    DEBUGLOG("UdpSocket::RecvMessage: XTaskQueueSubmitCallback failed with HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
                }
            }
            else if (m_acceptCallback != nullptr)
            {
                struct AcceptContext
                {
                    SOCKADDR address{};
                    UdpSocket* socket{ nullptr };
                    SocketPayload buffer{};
                };

                auto context = std::make_unique<AcceptContext>();
                context->address = m_recvContext.sourceAddress;
                context->socket = this;
                context->buffer = m_recvContext.buffer;
                context->buffer.payload.resize(bytesTransfered);

                HRESULT hr = XTaskQueueSubmitCallback(m_acceptTaskQueue, XTaskQueuePort::Completion, context.get(),
                    [](void* context, bool cancelled)
                {
                    std::unique_ptr<AcceptContext> contextPtr{ reinterpret_cast<AcceptContext*>(context) };

                    if (!cancelled)
                    {
                        auto socket = contextPtr->socket;
                        socket->m_acceptCallback(socket, &contextPtr->address, &contextPtr->buffer, socket->m_acceptCallbackContext);
                    }
                });

                if (SUCCEEDED(hr))
                {
                    context.release();
                }
                else
                {
                    DEBUGLOG("UdpSocket::RecvMessage: XTaskQueueSubmitCallback failed with HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
                }
            }

            // Start the next receive
            if (m_socket && m_threadPoolIo != nullptr)
            {
                return RecvNextMessage();
            }
            else
            {
                return S_OK;
            }
        }
        else
        {
            return HRESULT_FROM_WIN32(result);
        }
    }

    HRESULT UdpSocket::RecvNextMessage() noexcept
    {
        m_recvContext.buffer.payload.resize(c_MaxPayloadSize);

        DWORD flags = 0;
        WSABUF buf{ m_recvContext.buffer.Size(), reinterpret_cast<CHAR*>(m_recvContext.buffer.Data()) };

        if (!m_socket)
        {
            return E_INVALIDARG;
        }

        // Let the threadpool know that a task is being queued
        StartThreadpoolIo(m_threadPoolIo);

        auto result = WSARecvFrom(m_socket,
            &buf,
            1,
            &m_recvContext.bytesWritten,
            &flags,
            reinterpret_cast<SOCKADDR*>(&m_recvContext.sourceAddress),
            &m_recvContext.addressSize,
            &m_recvContext.overlapped,
            nullptr);

        if (!result)
        {
            result = WSA_IO_PENDING;
        }
        else
        {
            result = static_cast<int32_t>(GetLastError());
        }

        // If there was an error other than Pending then we need to cancel the threadpool task
        if (result != WSA_IO_PENDING && m_threadPoolIo)
        {
            CancelThreadpoolIo(m_threadPoolIo);
        }

        if (result == WSA_IO_PENDING)
        {
            return S_OK;
        }
        else
        {
            return HRESULT_FROM_WIN32(static_cast<uint32_t>(result));
        }
    }

    void UdpSocket::SetError(HRESULT hr) noexcept
    {
        DEBUGLOG("UdpSocket::SetError: HRESULT = 0x%08x", hr);

        m_internalError = hr;
    }
}
