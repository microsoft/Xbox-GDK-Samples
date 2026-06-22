//--------------------------------------------------------------------------------------
// File: UdpSocket.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <WinSock2.h>
#include <map>
#include <mutex>
#include <string>
#include <XTaskQueue.h>
#include "SocketPayload.h"

namespace ATG
{
    class UdpSocket;

    using SocketPayloadReceiverCallback = void (*)(UdpSocket* socket, const SOCKADDR* source, const SocketPayload* data, void* context);
    using UdpAcceptConnectionHandler = SocketPayloadReceiverCallback;

    struct ConnectionDataReceiver
    {
        SOCKADDR source{};
        XTaskQueueRegistrationToken token{};
        void* context{ nullptr };
        XTaskQueueHandle queue{ nullptr };
        SocketPayloadReceiverCallback callback{ nullptr };

        struct AddressComparison
        {
            bool operator()(const SOCKADDR& lhs, const SOCKADDR& rhs) const
            {
                return memcmp(&lhs, &rhs, sizeof(SOCKADDR)) < 0;
            }
        };
    };

    SOCKADDR AddressFromString(std::string_view address);
    std::string AddressToString(SOCKADDR& address);

    class UdpSocket
    {
    public:
        static HRESULT Create(uint16_t port, UdpSocket** socket) noexcept;

        ~UdpSocket() noexcept;

        void GetAddress(SOCKADDR* address) const noexcept;

        void AcceptConnections(UdpAcceptConnectionHandler callback, void* context, XTaskQueueHandle queue) noexcept;

        HRESULT SendTo(const SOCKADDR* destination, const SocketPayload* payload) noexcept;

        HRESULT RegisterDataReceiver(const SOCKADDR* source, void* context, SocketPayloadReceiverCallback callback, XTaskQueueHandle queue, XTaskQueueRegistrationToken* token) noexcept;

        HRESULT UnregisterDataReceiver(XTaskQueueRegistrationToken token) noexcept;

    private:
        explicit UdpSocket(SOCKET socket) noexcept;

        // Process the message from the overlapped handler
        HRESULT RecvMessage(LPOVERLAPPED overlapped, unsigned long result, size_t bytesTransfered) noexcept;

        // Queue the next overlapped receive to get the next message when it arrives
        HRESULT RecvNextMessage() noexcept;

        void SetError(HRESULT hr) noexcept;

        HRESULT m_internalError{ S_OK };

        SOCKADDR m_localAddress{};
        SOCKET m_socket{};
        PTP_IO m_threadPoolIo{ };

        struct RecvContext
        {
            OVERLAPPED overlapped{};
            SOCKADDR sourceAddress{};
            int addressSize{ sizeof(SOCKADDR) };
            unsigned long bytesWritten{};
            SocketPayload buffer{};
        };

        std::mutex m_receiversLock;
        uint64_t m_nextId{ 0 };
        std::map<uint64_t, ConnectionDataReceiver> m_tokenRegistationLookup;
        std::map<SOCKADDR, ConnectionDataReceiver*, ConnectionDataReceiver::AddressComparison> m_addressLookup;

        UdpAcceptConnectionHandler m_acceptCallback{ nullptr };
        void* m_acceptCallbackContext{ nullptr };
        XTaskQueueHandle m_acceptTaskQueue{ nullptr };

        RecvContext m_recvContext{};
    };
}
