//--------------------------------------------------------------------------------------
// File: DtlsConnection.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <WinSock2.h>
#include <mutex>
#include <string>
#include <XTaskQueue.h>
#include "DtlsSocket.h"
#include "DelayedOperation.h"
#include "SocketPayload.h"
#include "DtlsRecord.h"

namespace ATG
{
    class DtlsSocket;
    class TLSHandshakeMessage;
    class TLSRecord;

    struct DtlsConnection
    {
        enum class ConnectionState : uint32_t
        {
            None,
            Negotiating,
            Established,
            Renegotiating,
            Failed
        };

        std::string ConnectionStateToString(const ConnectionState& state)
        {
            switch (state)
            {
            case ConnectionState::Negotiating:   return "Negotiating";
            case ConnectionState::Established:   return "Established";
            case ConnectionState::Renegotiating: return "Renegotiating";
            case ConnectionState::Failed:        return "Failed";
            case ConnectionState::None:          return "None";
            }

            return "None";
        }

        enum class ProcessResult : uint32_t
        {
            Decrypted,
            Handshaking,
            WaitingForData,
            ConnectionClosed,
            ConnectionEstablished,
            Error
        };

        std::string ProcessResultToString(const ProcessResult& result)
        {
            switch (result)
            {
            case ProcessResult::Decrypted:             return "Decrypted";
            case ProcessResult::Handshaking:           return "Handshaking";
            case ProcessResult::WaitingForData:        return "WaitingForData";
            case ProcessResult::ConnectionClosed:      return "ConnectionClosed";
            case ProcessResult::ConnectionEstablished: return "ConnectionEstablished";
            case ProcessResult::Error:                 return "Error";
            }

            return "Error";
        }

        enum class HandshakeState : uint32_t
        {
            None,
            ParseClientHello,
            ParseServerHello,
            ParseClientKeyExchange,
            ParseClientFinished,
            ParseServerFinished
        };

        std::string HandshakeStateToString(const HandshakeState& state)
        {
            switch (state)
            {
            case HandshakeState::ParseClientHello:       return "ParseClientHello";
            case HandshakeState::ParseServerHello:       return "ParseServerHello";
            case HandshakeState::ParseClientKeyExchange: return "ParseClientKeyExchange";
            case HandshakeState::ParseClientFinished:    return "ParseClientFinished";
            case HandshakeState::ParseServerFinished:    return "ParseServerFinished";
            case HandshakeState::None:                   return "None";
            }

            return "None";
        }

        ~DtlsConnection();

        void InitSSL(bool isServer);
        bool IsConnected() const;
        void SendPayload(SocketPayload& payload);
        void SendApplicationData(SocketPayload& payload);
        ProcessResult ProcessData(const SocketPayload& inData, SocketPayload& outData);
        ProcessResult ProcessDtlsPayload(SocketPayload& inData, SocketPayload& outData);
        void EncryptData(SocketPayload& data);
        void DecryptData(SocketPayload& data);
        void Handshake(SocketPayload& packet);
        void ShutdownConnection();

        void ValidateCertificate(PCCERT_CONTEXT certContext);

        void GenerateMasterKey();
        void GenerateSessionKeys();
        void QueryBCryptObjectSizes();
        BCryptKeyHandle CreateEphemeralKey();
        void GenerateKeyMaterial(std::vector<uint8_t>& secret, const std::string_view label, std::vector<uint8_t>& seed, std::vector<uint8_t>& key);
        std::vector<uint8_t> ComputeFinishedHash(std::string_view label);

        // Client side handshake
        void StartClientHandshake();
        void SendClientHello();
        void ParseServerHello(std::shared_ptr<DtlsHandshakeMessage> tlsRecord);
        void SendClientKeyExchange();
        void ParseServerFinished(std::shared_ptr<DtlsHandshakeMessage> tlsRecord);

        // Server side handshake
        void StartServerHandshake(const SocketPayload& clientHello);
        void ParseClientHello(std::shared_ptr<DtlsHandshakeMessage> tlsRecord);
        std::vector<uint8_t> ComputeServerExchangeHashes();
        void SendServerHello();
        void ParseClientKeyExchange(std::shared_ptr<DtlsHandshakeMessage> tlsRecord);
        void ParseClientFinished(std::shared_ptr<DtlsHandshakeMessage> tlsRecord);
        void SendServerFinished();

        constexpr static std::string_view c_MasterSecret = "master secret";
        constexpr static std::string_view c_KeyExpansion = "key expansion";
        constexpr static std::string_view c_ServerFinished = "server finished";
        constexpr static std::string_view c_ClientFinished = "client finished";
        constexpr static size_t c_FinishHashLen = 12;
        constexpr static uint8_t c_MaxHandshakeRetries = 3;
        constexpr static uint32_t c_MasterKeyLength = 1024;

        PCCERT_CONTEXT m_clientCert{};
        PCCERT_CONTEXT m_serverCert{};

        BCryptHashHandle m_handshakeHash{};

        BCryptKeyHandle m_serverKey{};
        BCryptKeyHandle m_clientKey{};

        NCRYPT_KEY_HANDLE m_serverCertKey{};

        BCryptKeyHandle m_readKey{};
        BCryptKeyHandle m_writeKey{};

        std::vector<uint8_t> m_readKeyMaterial{};
        std::vector<uint8_t> m_writeKeyMaterial{};

        std::vector<uint8_t> m_writeMacBuffer{};
        std::vector<uint8_t> m_readMacBuffer{};

        std::vector<uint8_t> m_clientEphemeralKey{};
        std::vector<uint8_t> m_masterKeyMaterial{};

        std::vector<uint8_t> m_clientRandom{};
        std::vector<uint8_t> m_serverRandom{};
        std::vector<uint8_t> m_sessionId{};

        uint64_t m_readSequence{};
        uint64_t m_writeSequence{};
        uint16_t m_handshakeSequence{};
        uint8_t m_handshakeRetries{};
        uint8_t m_expectedHandshakeSequence{};
        uint16_t m_epoch{};

        DWORD m_hmacSha384ObjLen{};
        DWORD m_hmacSha384HashLen{};
        DWORD m_aesBlockLen{};
        DWORD m_aesObjLen{};
        DWORD m_md5ObjLen{};
        DWORD m_sha1ObjLen{};
        DWORD m_sha384ObjLen{};

        DtlsSocket* m_socket{ nullptr };
        SOCKADDR m_destinationAddress{};
        XTaskQueueRegistrationToken m_token{};
        DelayedOperation m_timeoutOp{};
        bool m_isServer{ false };
        bool m_encryptSend{ false };
        bool m_encryptRecv{ false };
        ConnectionState m_connectionState{ ConnectionState::None };
        HandshakeState m_HandshakeState{ HandshakeState::None };
        std::mutex m_sslMutex{};
        uint32_t m_maxMTU{ c_MaxPayloadSize };
        std::string m_expectedIdentityString{};
        std::shared_ptr<struct DtlsCreateConnectionAsyncOp> m_createConnectionOp{};
        std::vector<SocketPayload> m_retryPackets{};
        std::vector<SocketPayload> m_pendingPackets{};
    };
}
