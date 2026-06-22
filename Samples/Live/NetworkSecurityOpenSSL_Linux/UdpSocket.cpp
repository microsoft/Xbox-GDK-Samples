//--------------------------------------------------------------------------------------
// File: UdpSocket.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright(c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "UdpSocket.h"
#include "NetworkDebugHelpers.h"
#include <charconv>
#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>

namespace ATG
{
    bool ResolveAddressFromHost(std::string_view addr, sockaddr_in& outAddr)
    {
        addrinfo hints{}, * result;

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
                memcpy(&outAddr, result->ai_addr, result->ai_addrlen);
                found = true;
                break;
            }

            result = result->ai_next;
        }

        freeaddrinfo(orig);

        return found;
    }

    bool GetBestLocalAddress(sockaddr_in& outAddr)
    {
        struct ifaddrs *ifaddrs, *ifa;
        bool found = false;

        ZeroMemory(&outAddr, sizeof(outAddr));

        auto errcode = getifaddrs(&ifaddrs);
        if (errcode != 0)
        {
            return false;
        }

        ifa = ifaddrs;

        while(ifa != nullptr)
        {
            if (ifa->ifa_addr != nullptr)
            {
                if (ifa->ifa_addr->sa_family == AF_INET &&
                    (ifa->ifa_flags & IFF_UP) != 0 &&
                    (ifa->ifa_flags & IFF_LOOPBACK) == 0)
                {
                    memcpy(&outAddr, ifa->ifa_addr, sizeof(sockaddr_in));
                    found = true;
                    break;
                }
            }

            ifa = ifa->ifa_next;
        }

        freeifaddrs(ifaddrs);

        return found;
    }

    sockaddr AddressFromString(std::string_view address)
    {
        auto index = address.find_first_of(':');

        sockaddr addr;

        sockaddr_in* addrIn = reinterpret_cast<sockaddr_in*>(&addr);

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

    std::string AddressToString(const sockaddr& address)
    {
        auto addr = reinterpret_cast<const sockaddr_in*>(&address);
        char buffer[32];

        sprintf(buffer, "%u.%u.%u.%u:%u",
            (addr->sin_addr.s_addr & 0x000000FF),
            (addr->sin_addr.s_addr & 0x0000FF00) >> 8,
            (addr->sin_addr.s_addr & 0x00FF0000) >> 16,
            (addr->sin_addr.s_addr & 0xFF000000) >> 24,
            ntohs(addr->sin_port));

        return buffer;
    }

    HRESULT UdpSocket::Create(uint16_t port, UdpSocket** udpSocket) noexcept
    {
        DebugLog("UdpSocket::Create");

        uint64_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (sock == INVALID_SOCKET)
        {
            return HRESULT_FROM_WIN32(errno);
        }

        sockaddr localAddress{};
        sockaddr_in* address = reinterpret_cast<sockaddr_in*>(&localAddress);

        address->sin_port = htons(port);
        address->sin_family = AF_INET;

        auto result = bind(sock, &localAddress, sizeof(localAddress));

        if (result != 0)
        {
            return HRESULT_FROM_WIN32(errno);
        }

        auto socketPtr = new UdpSocket(sock);
        std::unique_ptr<UdpSocket> socketObj(socketPtr);

        *udpSocket = socketObj.get();
        socketObj.release();

        (*udpSocket)->StartReceiversThread();

        return S_OK;
    }

    UdpSocket::UdpSocket(int64_t socket) noexcept :
        m_socket(socket)
    {
        DebugLog("UdpSocket::UdpSocket");

        socklen_t size = sizeof(m_localAddress);
        getsockname(m_socket, &m_localAddress, &size);
    }

    UdpSocket::~UdpSocket() noexcept
    {
        DebugLog("UdpSocket::~UdpSocket");

        m_cleanUpReceiversThread = true;
        m_receiversThread.join();

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

        if (m_socket != INVALID_SOCKET)
        {
            close(m_socket);
            m_socket = INVALID_SOCKET;
        }
    }

    void UdpSocket::StartReceiversThread() noexcept
    {
        DebugLog("UdpSocket::StartReceiversThread");

        m_receiversThread = std::thread{ &UdpSocket::RecvNextMessage, this };
    }

    void UdpSocket::GetAddress(sockaddr* address) const noexcept
    {
        DebugLog("UdpSocket::GetAddress");

        memcpy(address, &m_localAddress, sizeof(sockaddr));
    }

    void UdpSocket::AcceptConnections(UdpAcceptConnectionHandler callback, void* context, XTaskQueueHandle queue) noexcept
    {
        DebugLog("UdpSocket::AcceptConnections");

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

    HRESULT UdpSocket::SendTo(const sockaddr* destination, const SocketPayload* payload) noexcept
    {
        DebugLog("UdpSocket::SendTo");

        if (FAILED(m_internalError))
        {
            return m_internalError;
        }

        SocketPayload empty{};
        if(!payload)
        {
            payload = &empty;
        }

        int result = sendto(m_socket,
            reinterpret_cast<const char*>(payload->payload),
            payload->size,
            0,
            destination,
            sizeof(sockaddr));

        if (result == SOCKET_ERROR)
        {
            return HRESULT_FROM_WIN32(errno);
        }
        else
        {
            DebugLog("UdpSocket::SendTo: Send %d bytes", result);
            return S_OK;
        }
    }

    HRESULT UdpSocket::RegisterDataReceiver(const sockaddr* source, void* context, SocketPayloadReceiverCallback callback, XTaskQueueHandle queue, XTaskQueueRegistrationToken* token) noexcept
    {
        DebugLog("UdpSocket::RegisterDataReceiver");

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

        auto&& [iter, result] = m_tokenRegistationLookup.insert({id, receiver});

        if (result)
        {
            m_addressLookup.insert({*source, &iter->second});
            return S_OK;
        }
        else
        {
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
    }

    HRESULT UdpSocket::UnregisterDataReceiver(XTaskQueueRegistrationToken token) noexcept
    {
        DebugLog("UdpSocket::UnregisterDataReceiver");

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

    void UdpSocket::RecvNextMessage() noexcept
    {
        while(m_cleanUpReceiversThread == false)
        {
            if (m_socket != INVALID_SOCKET)
            {
                struct timeval waitd = {10, 0};
                fd_set read_flags;
                fd_set write_flags;

                while(true) 
                {
                    FD_ZERO(&read_flags);
                    FD_ZERO(&write_flags);
                    FD_SET(m_socket, &read_flags);
                    FD_SET(m_socket, &write_flags);

                    int selectResult = select(m_socket + 1, &read_flags, &write_flags, (fd_set*)0, &waitd);

                    if(selectResult == SOCKET_ERROR)
                    {
                        break;
                    }

                    if (m_cleanUpReceiversThread)
                    {
                        break;
                    }
                    
                    if(FD_ISSET(m_socket, &read_flags)) 
                    {
                        FD_CLR(m_socket, &read_flags);

                        HRESULT hr = RecvMessage();
                        if(FAILED(hr))
                        {
                            DebugLog("Socket Failed to receive message: 0x%08X", hr);

                            SetError(hr);
                        }
                    }
                }
            }
        }
    }

    HRESULT UdpSocket::RecvMessage() noexcept
    {
        if (m_socket == INVALID_SOCKET)
        {
            return E_INVALIDARG;
        }

        SocketPayload buffer{};
        uint32_t flags = MSG_DONTWAIT;
        sockaddr sourceAddress{};
        socklen_t addressSize = sizeof(sockaddr);
    
        uint64_t recvfromResult = recvfrom(m_socket,
            reinterpret_cast<char*>(buffer.payload),
            sizeof(SocketPayload::payload),
            flags,
            &sourceAddress,
            &addressSize);

        if (recvfromResult == SOCKET_ERROR)
        {
            return HRESULT_FROM_WIN32(errno);
        }
        else
        {
            std::lock_guard lock(m_receiversLock);

            uint64_t bytesTransfered = recvfromResult;

            auto receiverIter = m_addressLookup.find(sourceAddress);

            if (receiverIter != m_addressLookup.end())
            {
                struct EventCallbackContext
                {
                    UdpSocket* socket{nullptr};
                    ConnectionDataReceiver *receiver{nullptr};
                    SocketPayload buffer{};
                };
                
                auto context = std::make_unique<EventCallbackContext>();

                context->socket = this;
                context->receiver = receiverIter->second;
                memcpy(&context->buffer, &buffer, sizeof(SocketPayload));
                context->buffer.size = static_cast<uint32_t>(bytesTransfered);

                auto& receiver = receiverIter->second;

                auto hr = XTaskQueueSubmitCallback(receiver->queue, XTaskQueuePort::Completion, context.get(), 
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
            }
            else if (m_acceptCallback != nullptr)
            {
                struct AcceptContext
                {
                    sockaddr address{};
                    UdpSocket* socket{nullptr};
                    SocketPayload payload{};
                };

                auto context = std::make_unique<AcceptContext>();
                context->address = sourceAddress;
                context->socket = this;

                memcpy(&context->payload.payload, &buffer.payload, bytesTransfered);
                context->payload.size = static_cast<uint32_t>(bytesTransfered);

                auto hr = XTaskQueueSubmitCallback(m_acceptTaskQueue, XTaskQueuePort::Completion, context.get(),
                    [](void* context, bool cancelled)
                    {
                        std::unique_ptr<AcceptContext> contextPtr{reinterpret_cast<AcceptContext*>(context)};

                        if (!cancelled)
                        {
                            auto socket = contextPtr->socket;
                            socket->m_acceptCallback(socket, &contextPtr->address, &contextPtr->payload, socket->m_acceptCallbackContext);
                        }
                    });

                if (SUCCEEDED(hr))
                {
                    context.release();
                }
            }
            
            return S_OK;
        }
    }

    void UdpSocket::SetError(HRESULT error) noexcept
    {
        DebugLog("UdpSocket::SetError");

        m_internalError = error;
    }
}
