//--------------------------------------------------------------------------------------
// File: UdpSocket.h
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright(c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <sys/socket.h>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <atomic>
#include "XTaskQueue.h"
#include "SocketPayload.h"

namespace ATG
{
    class UdpSocket;

    using SocketPayloadReceiverCallback = void (*)(UdpSocket* socket, const sockaddr* source, const SocketPayload* data, void* context);
    using UdpAcceptConnectionHandler = SocketPayloadReceiverCallback;

    struct ConnectionDataReceiver
    {
        sockaddr source{};
        XTaskQueueRegistrationToken token{};
        void* context{ nullptr };
        XTaskQueueHandle queue{ nullptr };
        SocketPayloadReceiverCallback callback{ nullptr };

        struct AddressComparison
        {
            bool operator()(const sockaddr& lhs, const sockaddr& rhs) const
            {
                return memcmp(&lhs, &rhs, sizeof(sockaddr)) < 0;
            }
        };
    };

    sockaddr AddressFromString(std::string_view address);
    std::string AddressToString(sockaddr& address);

    class UdpSocket
    {
    public:
        static HRESULT Create(uint16_t port, UdpSocket** socket) noexcept;

        ~UdpSocket() noexcept;

        void GetAddress(sockaddr* address) const noexcept;

        void AcceptConnections(UdpAcceptConnectionHandler callback, void* context, XTaskQueueHandle queue) noexcept;

        HRESULT SendTo(const sockaddr* destination, const SocketPayload* payload) noexcept;
    
        HRESULT RegisterDataReceiver(const sockaddr *source, void* context, SocketPayloadReceiverCallback callback, XTaskQueueHandle queue, XTaskQueueRegistrationToken* token) noexcept;

        HRESULT UnregisterDataReceiver(XTaskQueueRegistrationToken token) noexcept;

        void StartReceiversThread() noexcept;

    private:
        explicit UdpSocket(int64_t socket) noexcept;

        void RecvNextMessage() noexcept;
        HRESULT RecvMessage() noexcept;

        void SetError(HRESULT error) noexcept;

        HRESULT m_internalError{ S_OK };

        sockaddr m_localAddress{};
        int64_t m_socket{ INVALID_SOCKET };

        std::atomic<bool> m_cleanUpReceiversThread = false;
        std::thread m_receiversThread{};
        std::mutex m_receiversLock{};
        uint64_t m_nextId{ 0 };
        std::map<uint64_t, ConnectionDataReceiver> m_tokenRegistationLookup;
        std::map<sockaddr, ConnectionDataReceiver*, ConnectionDataReceiver::AddressComparison> m_addressLookup;

        UdpAcceptConnectionHandler m_acceptCallback{nullptr};
        void* m_acceptCallbackContext{nullptr};
        XTaskQueueHandle m_acceptTaskQueue{nullptr};
    };
}
