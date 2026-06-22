//--------------------------------------------------------------------------------------
// File: DTLSConnection.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <mutex>
#include <string>
#include "XTaskQueue.h"
#include "DTLSSocket.h"
#include "DelayedOperation.h"
#include "OpenSSLHelpers.h"
#include "SocketPayload.h"

namespace ATG
{
    class DTLSSocket;

    struct DTLSConnection
    {
        enum class State : uint32_t
        {
            None,
            Negotiating,
            Established,
            Renegotiating,
            Failed
        };

        enum class ProcessResult : uint32_t
        {
            Decrypted,
            Handshaking,
            WaitingForData,
            ConnectionClosed,
            ConnectionEstablished,

            Error
        };

        DTLSSocket* m_socket{ nullptr };
        sockaddr m_destinationAddress{};
        XTaskQueueRegistrationToken m_token{};

        DelayedOperation m_timeoutOp;

        bool     m_isServer{ false };
        State    m_connectionState{ State::None };

        std::mutex m_sslMutex;

        BIO* m_input{ nullptr };
        BIO* m_output{ nullptr };
        SSL_PTR     m_connection{ nullptr };

        uint32_t    m_maxMTU{ 0 };

        std::string m_expectedIdentityString;

        std::shared_ptr<struct DTLSCreateConnectionAsyncOp> m_createConnectionOp;

        ~DTLSConnection();

        void InitSSL(SSL_CTX* context, bool isServer);

        bool IsConnected() const;

        void Handshake();

        HRESULT StartClientHandshake();

        HRESULT StartServerHandshake(const SocketPayload& clientHello);

        HRESULT SendPayload(const SocketPayload& payload);

        bool CheckAndReadData(SocketPayload& data);

        ProcessResult ProcessData(const SocketPayload& inData, SocketPayload& outData);

        bool EncryptData(const SocketPayload& data);
        void ShutdownConnection();

        HRESULT ValidateCertificate();
    };
}
