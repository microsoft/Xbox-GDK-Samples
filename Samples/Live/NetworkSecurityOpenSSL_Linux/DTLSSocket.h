//--------------------------------------------------------------------------------------
// File: DTLSSocket.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <sys/socket.h>
#include "XAsync.h"
#include "XTaskQueue.h"
#include "SocketPayload.h"
#include <openssl/ssl.h>
#include <openssl/x509.h>

namespace ATG
{
    class UdpSocket;

    using DTLSConnectionHandle = struct DTLSConnection*;

    sockaddr AddressFromString(std::string_view address);
    std::string AddressToString(const sockaddr& address);

    std::string BytesToHexString(const uint8_t* bytes, size_t length);
    std::vector<uint8_t> HexStringToBytes(std::string_view string);
    bool SplitString(const std::string& source, const std::string& delim, std::string& left, std::string& right);

    using DTLSAcceptConnectionHandler = void (*)(class DTLSSocket*, const sockaddr*, const SocketPayload* data, void*);

    class DTLSSocket
    {
    public:
        static HRESULT Create(uint16_t port, XTaskQueueHandle queue, std::string_view certFile, std::string_view keyFile, DTLSSocket** socket) noexcept;

        ~DTLSSocket() noexcept;

        void GetAddress(sockaddr* address) const noexcept;

        uint32_t GetFingerprintSize() const noexcept;
        HRESULT GetFingerprint(uint8_t* buffer, uint32_t bufferSize, uint32_t* bufferUsed) const noexcept;

        uint32_t GetSubjectNameSize() const noexcept;
        HRESULT GetSubjectName(uint8_t* buffer, uint32_t bufferSize, uint32_t* bufferUsed) const noexcept;

        void AcceptConnections(DTLSAcceptConnectionHandler callback, void* context) noexcept;

        void AllowConnectionFrom(const sockaddr* destination) noexcept;

        HRESULT AcceptConnectionAsync(const sockaddr* inDestination, const std::string& expectedIdentityString, const SocketPayload* payload, XAsyncBlock* async) noexcept;

        static HRESULT AcceptConnectionAsyncResult(XAsyncBlock* async, DTLSConnectionHandle* connection) noexcept;

        HRESULT CreateConnectionAsync(const sockaddr* destination, const std::string& expectedIdentityString, XAsyncBlock* async) noexcept;

        static HRESULT CreateConnectionAsyncResult(XAsyncBlock* async, DTLSConnectionHandle* connection) noexcept;

        HRESULT GetConnectionMTU(const DTLSConnectionHandle connection, uint32_t* mtu) noexcept;

        HRESULT CloseConnection(DTLSConnectionHandle connection) noexcept;

        // Note: This is async because the encryption will happen in the background.
        HRESULT SendToAsync(DTLSConnectionHandle destination, const SocketPayload* payload, XAsyncBlock* async) noexcept;

        static HRESULT SendToAsyncResult(XAsyncBlock* async) noexcept;

        HRESULT RecvFrom(DTLSConnectionHandle* source, SocketPayload* payload) noexcept;

    private:
        std::shared_ptr<DTLSConnection> CreateNewConnection(const sockaddr* destination, bool asServer, const std::string& expectedIdentityString);

        static void DataReceiverStatic(UdpSocket* socket, const sockaddr* source, const SocketPayload* data, void* context);

        void DataReceiver(const SocketPayload* data, DTLSConnection* connection);

        HRESULT InternalSend(const DTLSConnection* destination, const SocketPayload* payload);

        void InternalAcceptConnectionHandler(const sockaddr* address, const SocketPayload* data) noexcept;

        HRESULT CreateSelfSignedCertificate() noexcept;

        void GenerateFingerprintAndSubjectNameFromContext() noexcept;

        struct DataReceived
        {
            HRESULT error{ S_OK };
            DTLSConnection* connection{ nullptr };
            SocketPayload payload;
        };

        XTaskQueueHandle m_queue;
        XTaskQueueHandle m_derivedWorkQueue;
        std::unique_ptr<UdpSocket> m_socket;

        DTLSAcceptConnectionHandler m_acceptCallback{ nullptr };
        void* m_acceptHandlerContext{ nullptr };

        std::mutex m_connectionLock;
        std::vector<std::shared_ptr<DTLSConnection>> m_connections;

        std::mutex m_packetLock;
        std::vector<std::unique_ptr<DataReceived>> m_decryptedPackets;

        SSL_CTX* m_sslContext{ nullptr };
        X509* m_cert{ nullptr };
        EVP_PKEY* m_key{ nullptr };

        std::vector<uint8_t> m_localFingerprint;
        std::vector<uint8_t> m_localSubjectName;

        friend struct DTLSSendAsyncOp;
        friend struct DTLSConnection;
    };
}
