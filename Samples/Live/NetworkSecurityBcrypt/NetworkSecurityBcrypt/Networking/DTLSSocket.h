//--------------------------------------------------------------------------------------
// File: DtlsSocket.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <WinSock2.h>
#include "SocketPayload.h"

namespace ATG
{
    class UdpSocket;

    using DtlsConnectionHandle = struct DtlsConnection*;

    SOCKADDR AddressFromString(std::string_view address);
    std::string AddressToString(const SOCKADDR& address);

    std::string BytesToHexString(const uint8_t* bytes, size_t length);
    std::string BytesToHexStringDelim(const uint8_t* bytes, size_t length, char delim);
    std::vector<uint8_t> HexStringToBytes(std::string_view string);
    bool SplitString(const std::string& source, const std::string& delim, std::string& left, std::string& right);

    using DtlsAcceptConnectionHandler = void (*)(class DtlsSocket*, const SOCKADDR*, const SocketPayload* data, void*);

    class DtlsSocket
    {
    public:
        static const uint32_t c_maxServerNameLength = 255;
        static const uint32_t c_maxServerNameSize = c_maxServerNameLength + 1; // includes null terminator
        static const uint32_t c_maxCertificateSubjectNameSize = 3 + c_maxServerNameSize; // "CN=" + name (includes null terminator)
        static const uint32_t c_maxEncodedCertificateSubjectNameSize = c_maxCertificateSubjectNameSize * 2;
        static const uint32_t c_maxCertificateFingerprintSize = 32;

        static const int32_t c_maxClockSkewInSeconds = (5) * 60; // 5 minutes
        static const uint64_t c_maxClockSkewIn100ns = static_cast<uint64_t>(c_maxClockSkewInSeconds) * 1000 * 1000 * 10; // 5 minutes
        static const uint64_t c_certificateLifetimeInSeconds = static_cast<uint64_t>(4) * 60 * 60; // 4 hours
        static const uint64_t c_certificateLifetimeIn100ns = c_certificateLifetimeInSeconds * 1000 * 1000 * 10; // 4 hours

        inline static const wchar_t* c_privateKeyContainerName = L"Server Key";

        static HRESULT Create(uint16_t port, XTaskQueueHandle queue, DtlsSocket** socket) noexcept;
        static HRESULT Create(uint16_t port, XTaskQueueHandle queue, std::string_view certFile, std::string_view keyFile, DtlsSocket** socket) noexcept;

        ~DtlsSocket() noexcept;

        void GetAddress(SOCKADDR* address) const noexcept;

        uint32_t GetFingerprintSize() const noexcept;
        HRESULT GetFingerprint(uint8_t* buffer, uint32_t bufferSize, uint32_t* bufferUsed) const noexcept;

        uint32_t GetSubjectNameSize() const noexcept;
        HRESULT GetSubjectName(uint8_t* buffer, uint32_t bufferSize, uint32_t* bufferUsed) const noexcept;

        void AcceptConnections(DtlsAcceptConnectionHandler callback, void* context) noexcept;

        void AllowConnectionFrom(const SOCKADDR* destination) noexcept;

        HRESULT AcceptConnectionAsync(const SOCKADDR* destination, const std::string& expectedIdentityString, const SocketPayload* payload, XAsyncBlock* async) noexcept;

        static HRESULT AcceptConnectionAsyncResult(XAsyncBlock* async, DtlsConnectionHandle* connection) noexcept;

        HRESULT CreateConnectionAsync(const SOCKADDR* destination, const std::string& expectedIdentityString, XAsyncBlock* async) noexcept;

        static HRESULT CreateConnectionAsyncResult(XAsyncBlock* async, DtlsConnectionHandle* connection) noexcept;

        HRESULT GetConnectionMTU(const DtlsConnectionHandle connection, uint32_t* mtu) noexcept;

        HRESULT CloseConnection(DtlsConnectionHandle connection) noexcept;

        // Note: This is async because the encryption will happen in the background.
        HRESULT SendToAsync(DtlsConnectionHandle destination, const SocketPayload* payload, XAsyncBlock* async) noexcept;

        static HRESULT SendToAsyncResult(XAsyncBlock* async) noexcept;

        HRESULT RecvFrom(DtlsConnectionHandle* source, SocketPayload* payload) noexcept;

    private:
        std::shared_ptr<DtlsConnection> CreateNewConnection(const SOCKADDR* destination, bool asServer, const std::string& expectedIdentityString);

        static void DataReceiverStatic(UdpSocket* socket, const SOCKADDR* source, const SocketPayload* data, void* context);

        void DataReceiver(const SocketPayload* data, DtlsConnection* connection);

        HRESULT InternalSend(const DtlsConnection* destination, const SocketPayload* payload);

        void InternalAcceptConnectionHandler(const SOCKADDR* address, const SocketPayload* data) noexcept;

        HRESULT GenerateSubjectNameFromContext();
        HRESULT GenerateFingerprintFromContext();
        HRESULT EncodeSubjectCommonName(std::string_view subjectCommonName, uint32_t maxOutputBufferSize, void* outputBuffer, uint32_t* outputBufferSize);
        HRESULT CreateSelfSignedCertificate() noexcept;
        HRESULT CreateCertficateContextFromFile(std::string_view filePath);
        HRESULT CreatePrivateKeyHandleFromFile(std::string_view filePath);
        HRESULT LoadAndDecodeBlobFromFile(std::string_view filePath, std::vector<uint8_t>& fileBlob);

        struct DataReceived
        {
            HRESULT error{ S_OK };
            DtlsConnection* connection{ nullptr };
            SocketPayload payload;
        };

        XTaskQueueHandle m_queue = nullptr;
        XTaskQueueHandle m_derivedWorkQueue = nullptr;
        std::unique_ptr<UdpSocket> m_socket;

        DtlsAcceptConnectionHandler m_acceptCallback{ nullptr };
        void* m_acceptHandlerContext{ nullptr };

        std::mutex m_connectionLock;
        std::vector<std::shared_ptr<DtlsConnection>> m_connections;

        std::mutex m_packetLock;
        std::vector<std::unique_ptr<DataReceived>> m_decryptedPackets;

        PCCERT_CONTEXT m_localCertContext{};
        NCRYPT_KEY_HANDLE m_privateKeyHandle{};

        std::vector<uint8_t> m_localFingerprint;
        std::vector<uint8_t> m_localSubjectName;

        friend struct DtlsSendAsyncOp;
        friend struct DtlsConnection;
    };
}
