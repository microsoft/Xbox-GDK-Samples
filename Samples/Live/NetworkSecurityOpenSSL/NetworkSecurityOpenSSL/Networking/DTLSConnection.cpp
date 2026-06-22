//--------------------------------------------------------------------------------------
// File: DTLSConnection.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "DTLSConnection.h"
#include "DTLSSocket.h"
#include "DTLSCreateConnectionAsyncOp.h"
#include "NetworkDebugHelpers.h"
#include <openssl/err.h>

namespace
{
    constexpr uint32_t c_handshakeTimeoutMS = 1000;

    int32_t g_sslIndex = -1;

    int32_t GetSSLDataIndex()
    {
        auto index = g_sslIndex;

        if (index == -1)
        {
            g_sslIndex = SSL_get_ex_new_index(0, &g_sslIndex, nullptr, nullptr, nullptr);

            if (g_sslIndex == -1)
            {
                DebugLog("Unable to allocate new index");
            }
            else
            {
                index = g_sslIndex;
            }
        }

        return index;
    }
}

namespace ATG
{
    struct DTLSCreateConnectionAsyncOp;

    DTLSConnection::~DTLSConnection()
    {
        DebugLog("DTLSConnection::~DTLSConnection");
    }

    void DTLSConnection::InitSSL(SSL_CTX* context, bool isServer)
    {
        DebugLog("DTLSConnection::InitSSL");

        m_isServer = isServer;

        m_input = BIO_new(BIO_s_mem());
        BIO_set_mem_eof_return(m_input, -1);
        m_output = BIO_new(BIO_s_mem());
        BIO_set_mem_eof_return(m_output, -1);

        m_connection.reset(SSL_new(context));

        SSL_set_ciphersuites(m_connection.get(), SSL_TXT_AES_GCM);

        SSL_set_mtu(m_connection.get(), c_MaxPayloadSize);

        SSL_set_bio(m_connection.get(), m_input, m_output);

        if (m_isServer)
        {
            SSL_set_accept_state(m_connection.get());
        }
        else
        {
            SSL_set_connect_state(m_connection.get());
        }

        SSL_set_ex_data(m_connection.get(), GetSSLDataIndex(), this);

        m_connectionState = State::Negotiating;

        m_timeoutOp.Init([this]()
        {
            if (m_connectionState == State::Negotiating || m_connectionState == State::Failed)
            {
                ShutdownConnection();
                m_connectionState = State::Failed;
                m_socket->CloseConnection(this);

                if (m_createConnectionOp != nullptr)
                {
                    DebugLog("DTLSConnection timed out");
                    m_createConnectionOp->Complete(HRESULT_FROM_WIN32(ERROR_CONNECTION_UNAVAIL));
                }
            }

        }, m_socket->m_derivedWorkQueue);
    }

    bool DTLSConnection::IsConnected() const
    {
        DebugLog("DTLSConnection::IsConnected");

        return SSL_is_init_finished(m_connection.get()) == 1;
    }

    void DTLSConnection::Handshake()
    {
        DebugLog("DTLSConnection::Handshake");

        int result = SSL_do_handshake(m_connection.get());

        if (result <= 0)
        {
            auto error = SSL_get_error(m_connection.get(), result);

            if (error != SSL_ERROR_WANT_READ)
            {
                char errorStringBuffer[256];
                ERR_error_string_n(ERR_get_error(), errorStringBuffer, ARRAYSIZE(errorStringBuffer));
                DebugLog("DTLSConnection::Handshake: Handshake Failed with %d,\n  %s", error, errorStringBuffer);
            }
        }
    }

    HRESULT DTLSConnection::StartClientHandshake()
    {
        DebugLog("DTLSConnection::StartClientHandshake: Starting client handshake");

        Handshake();

        auto hr = m_timeoutOp.Reset(c_handshakeTimeoutMS);

        DebugLog("DTLSConnection::StartClientHandshakeTimer reset: 0x%08X", hr);

        SocketPayload data{};
        if (CheckAndReadData(data))
        {
            return m_socket->InternalSend(this, &data);
        }

        return S_OK;
    }

    HRESULT DTLSConnection::StartServerHandshake(const SocketPayload& clientHello)
    {
        DebugLog("DTLSConnection::StartServerHandshake: Starting server handshake");

        SocketPayload data{};

        auto hr = m_timeoutOp.Reset(c_handshakeTimeoutMS);

        DebugLog("DTLSConnection::StartServerHandshake: Timer reset: 0x%08X", hr);

        auto result = ProcessData(clientHello, data);

        Assert(result == ProcessResult::Handshaking);

        return S_OK;
    }

    HRESULT DTLSConnection::SendPayload(const SocketPayload& data)
    {
        DebugLog("DTLSConnection::SendPayload: Sending %d bytes", data.size);

        std::scoped_lock lock(m_sslMutex);
        EncryptData(data);

        HRESULT hr = S_OK;

        SocketPayload encryptedData;
        while (CheckAndReadData(encryptedData) && SUCCEEDED(hr))
        {
            hr = m_socket->InternalSend(this, &encryptedData);
        }

        return hr;
    }

    bool DTLSConnection::CheckAndReadData(SocketPayload& data)
    {
        DebugLog("DTLSConnection::CheckAndReadData");

        auto pending = BIO_ctrl_pending(m_output);

        if (pending > 0)
        {
            auto read = BIO_read(m_output, data.payload, sizeof(SocketPayload::payload));

            if (read > 0)
            {
                DebugLog("DTLSConnection::CheckAndReadData: Read %d encrypted bytes", read);
                data.size = static_cast<uint32_t>(read);
                return true;
            }
            else
            {
                auto error = SSL_get_error(m_connection.get(), read);

                DebugLog("DTLSConnection::CheckAndReadData: BIO_read failed with %d", error);
            }
        }
        return false;
    }

    DTLSConnection::ProcessResult DTLSConnection::ProcessData(const SocketPayload& inData, SocketPayload& outData)
    {
        DebugLog("DTLSConnection::ProcessData: Proccessing %d bytes", inData.size);

        auto result = ProcessResult::Error;

        auto written = BIO_write(m_input, inData.payload, static_cast<int32_t>(inData.size));
        if (written > 0)
        {
            if (SSL_get_shutdown(m_connection.get()) & SSL_RECEIVED_SHUTDOWN)
            {
                DebugLog("DTLSConnection::ProcessData: Connection closed");
                result = ProcessResult::ConnectionClosed;
            }
            else if (!IsConnected())
            {
                Assert(m_connectionState == State::Negotiating);

                Handshake();

                if (IsConnected())
                {
                    DebugLog("DTLSConnection::ProcessData: Connected");

                    auto hr = ValidateCertificate();
                    if (SUCCEEDED(hr))
                    {
                        m_maxMTU = static_cast<uint32_t>(DTLS_get_data_mtu(m_connection.get()));
                        DebugLog("DTLSConnection::ProcessData: MTU: %u", m_maxMTU);

                        m_connectionState = State::Established;
                        result = ProcessResult::ConnectionEstablished;
                    }
                    else
                    {
                        DebugLog("DTLSConnection::ProcessData: Certificate Validation Failed: 0x%08x", hr);
                        m_timeoutOp.Cancel();
                        ShutdownConnection();
                        m_connectionState = State::Failed;
                        m_socket->CloseConnection(this);
                        result = ProcessResult::ConnectionClosed;
                    }

                    if (m_createConnectionOp != nullptr)
                    {
                        m_createConnectionOp->Complete(hr);
                    }

                    // The handshake timeout is no longer needed now that we are connected
                    m_timeoutOp.Cancel();
                }
                else
                {
                    // Restart the timer
                    auto hr = m_timeoutOp.Reset(c_handshakeTimeoutMS);

                    DebugLog("DTLSConnection::ProcessData: Timer reset: 0x%08X", hr);

                    DebugLog("DTLSConnection::ProcessData: Continue handshake sequence");
                    result = ProcessResult::Handshaking;
                }

                // Send pending handshake or shutdown data.
                SocketPayload data;
                while (CheckAndReadData(data))
                {
                    m_socket->InternalSend(this, &data);
                }
            }
            else
            {
                int read = SSL_read(m_connection.get(), outData.payload, written);

                if (read < 0)
                {
                    auto error = SSL_get_error(m_connection.get(), read);

                    if (error == SSL_ERROR_WANT_WRITE || error == SSL_ERROR_WANT_READ)
                    {
                        result = ProcessResult::WaitingForData;
                    }
                    else
                    {
                        DebugLog("DTLSConnection::ProcessData: SSL_read failed with %d", error);
                        result = ProcessResult::Error;
                    }
                }
                else
                {
                    DebugLog("DTLSConnection::ProcessData: Decrypted %d bytes", read);
                    outData.size = static_cast<uint32_t>(read);
                    result = ProcessResult::Decrypted;
                }
            }
        }

        return result;
    }

    bool DTLSConnection::EncryptData(const SocketPayload& data)
    {
        DebugLog("DTLSConnection::EncryptData");

        int result = 0;
        int attempts = 0;

        do
        {
            result = SSL_write(m_connection.get(), data.payload, static_cast<int32_t>(data.size));

            if (result <= 0)
            {
                auto error = SSL_get_error(m_connection.get(), result);

                // Retry a couple of times
                if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
                {
                    attempts++;
                    continue;
                }
            }
            else
            {
                break;
            }

        } while (result < 0 && attempts < 3);

        return attempts < 3;
    }

    void DTLSConnection::ShutdownConnection()
    {
        DebugLog("DTLSConnection::ShutdownConnection");

        SSL_shutdown(m_connection.get());
    }

    HRESULT DTLSConnection::ValidateCertificate()
    {
        if (m_expectedIdentityString.empty())
        {
            DebugLog("DTLSConnection::ValidateCertificate: m_expectedIdentityString was empty");

            return E_INVALIDARG;
        }

        DebugLog("DTLSConnection::ValidateCertificate: Validating Certificate");

        HRESULT result = S_OK;
        X509* remoteCert = SSL_get_peer_certificate(m_connection.get());
        if (remoteCert != nullptr)
        {
            if (X509_cmp_current_time(X509_get0_notBefore(remoteCert)) >= 0)
            {
                DebugLog("DTLSConnection::ValidateCertificate: Certificate not yet valid");
                result = CERT_E_EXPIRED;
            }
            else if (X509_cmp_current_time(X509_get0_notAfter(remoteCert)) <= 0)
            {
                DebugLog("DTLSConnection::ValidateCertificate: Certificate expired");
                result = CERT_E_EXPIRED;
            }
        }
        else
        {
            DebugLog("DTLSConnection::ValidateCertificate: Certificate was not found");
            result = HRESULT_FROM_WIN32(ERROR_NO_MATCH);
        }

        std::string expectedFingerprintString, expectedSubjectNameString;
        ATG::SplitString(m_expectedIdentityString, ":", expectedFingerprintString, expectedSubjectNameString);

        if (SUCCEEDED(result))
        {
            if (!expectedSubjectNameString.empty())
            {
                size_t subjectNameLen{};
                const uint8_t* subjectNameBytes{};

                if (X509_NAME_get0_der(X509_get_subject_name(remoteCert), &subjectNameBytes, &subjectNameLen))
                {
                    // For debugging purposes
                    std::string subjectNameString = BytesToHexString(subjectNameBytes, subjectNameLen);

                    DebugLog("DTLSConnection::ValidateCertificate: expectedSubjectName: %s", expectedFingerprintString.c_str());
                    DebugLog("DTLSConnection::ValidateCertificate: subjectName: %s", subjectNameString.c_str());

                    if (subjectNameString != expectedSubjectNameString)
                    {
                        DebugLog("DTLSConnection::ValidateCertificate: Expected subject name did not match certificate.");
                        result = TRUST_E_SUBJECT_NOT_TRUSTED;
                    }
                }
            }
            else
            {
                DebugLog("DTLSConnection::ValidateCertificate: Subject name not set");
                result = TRUST_E_SUBJECT_NOT_TRUSTED;
            }
        }

        if (SUCCEEDED(result))
        {
            if (!expectedFingerprintString.empty())
            {
                auto digest = EVP_get_digestbyname("sha256");

                uint8_t fingerprintBytes[256]{};
                uint32_t fingerprintLen = 0;

                X509_digest(remoteCert, digest, fingerprintBytes, &fingerprintLen);

                // For debugging purposes
                std::string fingerprintString = BytesToHexString(fingerprintBytes, fingerprintLen);

                DebugLog("DTLSConnection::ValidateCertificate: expectedFingerprint: %s", expectedFingerprintString.c_str());
                DebugLog("DTLSConnection::ValidateCertificate: fingerprint: %s", fingerprintString.c_str());

                if (fingerprintString != expectedFingerprintString)
                {
                    DebugLog("DTLSConnection::ValidateCertificate: Certificate did not match expected fingerprint");
                    result = HRESULT_FROM_WIN32(ERROR_NO_MATCH);
                }
            }
            else
            {
                DebugLog("DTLSConnection::ValidateCertificate: Fingerprint not set.");
                result = HRESULT_FROM_WIN32(ERROR_NO_MATCH);
            }
        }

        X509_free(remoteCert);
        return result;
    }
}
