//--------------------------------------------------------------------------------------
// File: DTLSSocket.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "DTLSSocket.h"
#include <charconv>
#include <XAsyncProvider.h>
#include "UdpSocket.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include "OpenSSLHelpers.h"
#include "DTLSConnection.h"
#include "DTLSCreateConnectionAsyncOp.h"

namespace
{
    std::string GetLocalHostName()
    {
        char hostNameBuffer[256]{};
        int result = gethostname(hostNameBuffer, sizeof(hostNameBuffer));
        if (result != 0)
        {
            return "";
        }

        return hostNameBuffer;
    }
}

namespace ATG
{
    constexpr char c_hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    std::string BytesToHexString(const uint8_t* bytes, size_t length)
    {
        // 2 characters per byte
        std::string out(length * 2, ' ');

        for (size_t i = 0; i < length; ++i)
        {
            out[2 * i] = c_hexmap[(bytes[i] & 0xF0) >> 4];
            out[2 * i + 1] = c_hexmap[bytes[i] & 0x0F];
        }

        return out;
    }

    std::vector<uint8_t> HexStringToBytes(std::string_view string)
    {
        std::vector<uint8_t> bytes;

        // 2 characters per byte
        bytes.reserve(string.length() / 2);

        for (size_t i = 0; i < string.length(); i += 2)
        {
            uint8_t byte = 0;
            std::from_chars(string.data() + i, string.data() + (i + 2), byte, 16);

            bytes.push_back(byte);
        }

        return bytes;
    }

    bool SplitString(const std::string& source, const std::string& delim, std::string& left, std::string& right)
    {
        if (delim.empty() == false)
        {
            std::string str = source;

            auto pos = str.find(delim);
            if (pos != std::string::npos)
            {
                left = str.substr(0, pos);
                str.erase(0, pos + delim.length());
                right = str;

                return true;
            }
        }

        return false;
    }

    struct DTLSSendAsyncOp
    {
        SocketPayload payload;
        DTLSConnection* destination;

        HRESULT DoWork() noexcept
        {
            return destination->SendPayload(payload);
        }

        HRESULT GetResult() noexcept
        {
            return S_OK;
        }

        static HRESULT Provider(_In_ XAsyncOp op, _In_ const XAsyncProviderData* data) noexcept
        {
            auto context = static_cast<DTLSSendAsyncOp*>(data->context);

            switch (op)
            {
            case XAsyncOp::Begin:
                return XAsyncSchedule(data->async, 0);

            case XAsyncOp::Cleanup:
            {
                delete context;
            }
            break;

            case XAsyncOp::GetResult:
            {
                auto hr = context->GetResult();

                if (FAILED(hr))
                {
                    return hr;
                }
            }
            break;

            case XAsyncOp::Cancel:
            {
                XAsyncComplete(data->async, E_ABORT, 0);
            }
            break;

            case XAsyncOp::DoWork:
            {
                HRESULT asyncHr = context->DoWork();
                XAsyncComplete(data->async, asyncHr, 0);
            }
            break;
            }

            return S_OK;
        }
    };

    HRESULT DTLSSocket::Create(uint16_t port, XTaskQueueHandle queue, DTLSSocket** socket) noexcept
    {
        DebugLog("DTLSSocket::Create");

        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_ssl_algorithms();

        auto dtlsSocket = std::make_unique<DTLSSocket>();

        dtlsSocket->m_sslContext = SSL_CTX_new(DTLS_method());

        // Disable DTLS1.0
        SSL_CTX_set_options(dtlsSocket->m_sslContext, SSL_OP_NO_DTLSv1);

        // We explicitly set this to avoid some fragmentation
        SSL_CTX_set_options(dtlsSocket->m_sslContext, SSL_OP_NO_QUERY_MTU);

        SSL_CTX_set_verify(dtlsSocket->m_sslContext, SSL_VERIFY_PEER, [](int, X509_STORE_CTX*) { return 1; });

        if (queue)
        {
            XTaskQueueDuplicateHandle(queue, &dtlsSocket->m_queue);
        }

        auto hr = dtlsSocket->CreateSelfSignedCertificate();

        SSL_CTX_use_certificate(dtlsSocket->m_sslContext, dtlsSocket->m_cert);
        SSL_CTX_use_PrivateKey(dtlsSocket->m_sslContext, dtlsSocket->m_key);

        if (FAILED(hr))
        {
            return hr;
        }

        dtlsSocket->GenerateFingerprintAndSubjectNameFromContext();

        UdpSocket* internalSocket;
        hr = UdpSocket::Create(port, &internalSocket);

        if (SUCCEEDED(hr))
        {
            dtlsSocket->m_socket.reset(internalSocket);
            *socket = dtlsSocket.get();
            dtlsSocket.release();
        }

        return hr;
    }

    void DTLSSocket::GenerateFingerprintAndSubjectNameFromContext() noexcept
    {
        DebugLog("DTLSSocket::GenerateFingerprintAndSubjectNameFromContext");

        X509* x509 = SSL_CTX_get0_certificate(m_sslContext);
        if (x509 != nullptr)
        {
            auto digest = EVP_get_digestbyname("sha256");

            // Fingerprint
            m_localFingerprint.resize(static_cast<uint32_t>(EVP_MD_size(digest)));
            uint32_t length = 0;
            X509_digest(x509, digest, m_localFingerprint.data(), &length);

            // Subject Name
            size_t subjectNameLen{};
            const uint8_t* subjectName{};
            if (X509_NAME_get0_der(X509_get_subject_name(x509), &subjectName, &subjectNameLen))
            {
                m_localSubjectName.clear();
                m_localSubjectName.insert(m_localSubjectName.end(), subjectName, subjectName + subjectNameLen);
            }
        }
    }

    DTLSSocket::~DTLSSocket() noexcept
    {
        DebugLog("DTLSSocket::~DTLSSocket");

        m_connections.clear();

        if (m_cert)
        {
            X509_free(m_cert);
        }

        if (m_key)
        {
            EVP_PKEY_free(m_key);
        }

        if (m_sslContext)
        {
            SSL_CTX_free(m_sslContext);
        }

        if (m_queue)
        {
            XTaskQueueCloseHandle(m_queue);
            m_queue = nullptr;
        }
    }

    void DTLSSocket::GetAddress(SOCKADDR* address) const noexcept
    {
        //DebugLog("DTLSSocket::GetAddress");

        m_socket->GetAddress(address);
    }

    uint32_t DTLSSocket::GetFingerprintSize() const noexcept
    {
        //DebugLog("DTLSSocket::GetFingerprintSize");

        return static_cast<uint32_t>(m_localFingerprint.size());
    }

    HRESULT DTLSSocket::GetFingerprint(uint8_t* buffer, uint32_t bufferSize, uint32_t* bufferUsed) const noexcept
    {
        //DebugLog("DTLSSocket::GetFingerprint");

        if (buffer == nullptr)
        {
            return E_INVALIDARG;
        }

        if (bufferSize < m_localFingerprint.size())
        {
            return E_NOT_SUFFICIENT_BUFFER;
        }

        memcpy(buffer, m_localFingerprint.data(), m_localFingerprint.size());

        if (bufferUsed)
        {
            *bufferUsed = static_cast<uint32_t>(m_localFingerprint.size());
        }

        return S_OK;
    }

    uint32_t DTLSSocket::GetSubjectNameSize() const noexcept
    {
        //DebugLog("DTLSSocket::GetSubjectNameSize");

        return static_cast<uint32_t>(m_localSubjectName.size());
    }

    HRESULT DTLSSocket::GetSubjectName(uint8_t* buffer, uint32_t bufferSize, uint32_t* bufferUsed) const noexcept
    {
        //DebugLog("DTLSSocket::GetSubjectName");

        if (buffer == nullptr)
        {
            return E_INVALIDARG;
        }

        if (bufferSize < m_localSubjectName.size())
        {
            return E_NOT_SUFFICIENT_BUFFER;
        }

        memcpy(buffer, m_localSubjectName.data(), m_localSubjectName.size());

        if (bufferUsed)
        {
            *bufferUsed = static_cast<uint32_t>(m_localSubjectName.size());
        }

        return S_OK;
    }

    void DTLSSocket::AcceptConnections(DTLSAcceptConnectionHandler callback, void* context) noexcept
    {
        DebugLog("DTLSSocket::AcceptConnections");

        m_acceptCallback = callback;
        m_acceptHandlerContext = context;

        if (callback != nullptr)
        {
            m_socket->AcceptConnections([](UdpSocket*, const SOCKADDR* address, const SocketPayload* data, void* context)
            {
                reinterpret_cast<DTLSSocket*>(context)->InternalAcceptConnectionHandler(address, data);
            }, this, m_queue);
        }
        else
        {
            m_socket->AcceptConnections(nullptr, nullptr, nullptr);
        }
    }

    void DTLSSocket::AllowConnectionFrom(const SOCKADDR* destination) noexcept
    {
        DebugLog("DTLSSocket::AllowConnectionFrom");

        // Send a packet to punch a hole in the firewall.
        m_socket->SendTo(destination, {});
    }

    HRESULT DTLSSocket::AcceptConnectionAsync(const SOCKADDR* inDestination, const std::string& expectedIdentityString, const SocketPayload* payload, XAsyncBlock* async) noexcept
    {
        DebugLog("DTLSSocket::AcceptConnectionAsync");

        if (inDestination == nullptr)
        {
            DebugLog("DTLSSocket::AcceptConnectionAsync: inDestination was null");
            return E_INVALIDARG;
        }

        if (expectedIdentityString.empty())
        {
            DebugLog("DTLSSocket::AcceptConnectionAsync: expectedIdentityString was empty");
            return E_INVALIDARG;
        }

        HRESULT hr = S_OK;
        auto connection = CreateNewConnection(inDestination, true, expectedIdentityString);

        // Create new connection will return null if there's already and existing connection with this address
        if (connection == nullptr)
        {
            DebugLog("DTLSSocket::CreateConnectionAsync: AcceptConnectionAsync was null");
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }

        auto context = std::make_shared<DTLSCreateConnectionAsyncOp>();
        context->connection = connection;
        connection->m_createConnectionOp = context;

        hr = connection->StartServerHandshake(*payload);

        if (SUCCEEDED(hr))
        {
            hr = XAsyncBegin(async, context.get(), nullptr, __FUNCTION__, DTLSCreateConnectionAsyncOp::Provider);
        }

        if (FAILED(hr))
        {
            context->connection = nullptr;
            connection->m_createConnectionOp = nullptr;
            CloseConnection(connection.get());
        }

        return hr;
    }

    HRESULT DTLSSocket::AcceptConnectionAsyncResult(XAsyncBlock* async, DTLSConnectionHandle* connection) noexcept
    {
        DebugLog("DTLSSocket::AcceptConnectionAsyncResult");

        return XAsyncGetResult(async, nullptr, sizeof(*connection), connection, nullptr);
    }

    HRESULT DTLSSocket::CreateConnectionAsync(const SOCKADDR* inDestination, const std::string& expectedIdentityString, XAsyncBlock* async) noexcept
    {
        DebugLog("DTLSSocket::CreateConnectionAsync");

        if (inDestination == nullptr)
        {
            DebugLog("DTLSSocket::CreateConnectionAsync: inDestination was null");
            return E_INVALIDARG;
        }

        if (expectedIdentityString.empty())
        {
            DebugLog("DTLSSocket::CreateConnectionAsync: expectedIdentityString was empty");
            return E_INVALIDARG;
        }

        auto connection = CreateNewConnection(inDestination, false, expectedIdentityString);

        // Create new connection will return null if there's already an existing connection with this address
        if (connection == nullptr)
        {
            DebugLog("DTLSSocket::CreateConnectionAsync: destination was null");
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }

        auto context = std::make_shared<DTLSCreateConnectionAsyncOp>();
        context->connection = connection;
        connection->m_createConnectionOp = context;

        // Start off the handshake process for the client
        auto hr = connection->StartClientHandshake();

        if (SUCCEEDED(hr))
        {
            hr = XAsyncBegin(async, context.get(), nullptr, __FUNCTION__, DTLSCreateConnectionAsyncOp::Provider);
        }

        if (FAILED(hr))
        {
            context->connection = nullptr;
            connection->m_createConnectionOp = nullptr;
            CloseConnection(connection.get());
        }

        return hr;
    }

    HRESULT DTLSSocket::CreateConnectionAsyncResult(XAsyncBlock* async, DTLSConnectionHandle* connection) noexcept
    {
        DebugLog("DTLSSocket::CreateConnectionAsyncResult");

        return XAsyncGetResult(async, nullptr, sizeof(*connection), connection, nullptr);
    }

    HRESULT DTLSSocket::GetConnectionMTU(const DTLSConnectionHandle inConnection, uint32_t* inMtu) noexcept
    {
        DebugLog("DTLSSocket::GetConnectionMTU");

        if (inConnection == nullptr || inMtu == nullptr)
        {
            DebugLog("DTLSSocket::GetConnectionMTU: inConnection was null");
            return E_INVALIDARG;
        }

        if (inMtu == nullptr)
        {
            DebugLog("DTLSSocket::GetConnectionMTU: inMtu was null");
            return E_INVALIDARG;
        }

        // Ensure that the connection has been established. There really shouldn't be a way to hit this.
        if (inConnection->m_connectionState != DTLSConnection::State::Established)
        {
            DebugLog("DTLSSocket::GetConnectionMTU: connection was not established");

            return HRESULT_FROM_WIN32(ERROR_CONNECTION_UNAVAIL);
        }

        *inMtu = inConnection->m_maxMTU;

        return S_OK;
    }

    HRESULT DTLSSocket::CloseConnection(DTLSConnectionHandle connection) noexcept
    {
        DebugLog("DTLSSocket::CloseConnection");

        std::scoped_lock lock(m_connectionLock);

        bool found = false;
        for (auto& con : m_connections)
        {
            if (con.get() == connection)
            {
                m_connections.erase(std::remove(m_connections.begin(), m_connections.end(), con));
                m_socket->UnregisterDataReceiver(connection->m_token);
                found = true;
                break;
            }
        }

        if (found == false)
        {
            return E_INVALIDARG;
        }

        return S_OK;
    }

    HRESULT DTLSSocket::SendToAsync(DTLSConnectionHandle inDestination, const SocketPayload* inPayload, XAsyncBlock* async) noexcept
    {
        DebugLog("DTLSSocket::SendToAsync");

        if (inDestination == nullptr)
        {
            DebugLog("DTLSSocket::SendToAsync: inDestination was null");
            return E_INVALIDARG;
        }

        if (inPayload->size > inDestination->m_maxMTU)
        {
            DebugLog("DTLSSocket::SendToAsync: payload was too large to send");
            return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        }

        // Ensure that the connection has been established.
        if (inDestination->m_connectionState != DTLSConnection::State::Established)
        {
            DebugLog("DTLSSocket::SendToAsync: connection was not established");
            return HRESULT_FROM_WIN32(ERROR_CONNECTION_UNAVAIL);
        }

        auto op = std::make_unique<DTLSSendAsyncOp>();
        op->destination = inDestination;
        memcpy(&op->payload, inPayload, sizeof(uint32_t) + inPayload->size);

        auto hr = XAsyncBegin(async, op.get(), nullptr, __FUNCTION__, DTLSSendAsyncOp::Provider);

        if (SUCCEEDED(hr))
        {
            op.release();
        }

        return hr;
    }

    HRESULT DTLSSocket::SendToAsyncResult(XAsyncBlock* async) noexcept
    {
        DebugLog("DTLSSocket::SendToAsyncResult");

        return XAsyncGetResult(async, nullptr, 0, nullptr, nullptr);
    }

    HRESULT DTLSSocket::RecvFrom(DTLSConnectionHandle* source, SocketPayload* payload) noexcept
    {
        std::scoped_lock lock(m_packetLock);

        if (source == nullptr)
        {
            return E_INVALIDARG;
        }

        if (m_decryptedPackets.empty())
        {
            return HRESULT_FROM_WIN32(ERROR_EMPTY);
        }

        auto& packet = m_decryptedPackets.front();

        HRESULT hr = packet->error;

        if (SUCCEEDED(hr))
        {
            *source = packet->connection;
            memcpy(payload, &packet->payload, sizeof(uint32_t) + packet->payload.size);
        }

        m_decryptedPackets.erase(m_decryptedPackets.begin());

        return hr;
    }

    std::shared_ptr<DTLSConnection> DTLSSocket::CreateNewConnection(const SOCKADDR* destination, bool asServer, const std::string& expectedIdentityString)
    {
        DebugLog("DTLSSocket::CreateNewConnection");

        auto connection = std::make_shared<DTLSConnection>();
        connection->m_socket = this;
        connection->m_destinationAddress = *destination;
        connection->m_expectedIdentityString = expectedIdentityString;

        {
            // Ensure that no other connections exists for the same address
            std::scoped_lock lock(m_connectionLock);
            auto existing = std::find_if(m_connections.begin(), m_connections.end(), [destination](const std::shared_ptr<DTLSConnection>& connections)
            {
                return memcmp(destination, &connections->m_destinationAddress, sizeof(SOCKADDR)) == 0;
            });

            if (existing == m_connections.end())
            {
                m_connections.push_back(connection);
            }
            else
            {
                return nullptr;
            }
        }

        // Initialize the OpenSSL structures for this connection
        connection->InitSSL(m_sslContext, asServer);

        // Start listening for data on this connection;
        m_socket->RegisterDataReceiver(destination, connection.get(), DTLSSocket::DataReceiverStatic, m_queue, &connection->m_token);

        return connection;
    }

    void DTLSSocket::DataReceiverStatic(UdpSocket*, const SOCKADDR*, const SocketPayload* data, void* context)
    {
        auto connection = reinterpret_cast<DTLSConnection*>(context);
        connection->m_socket->DataReceiver(data, connection);
    }

    void DTLSSocket::DataReceiver(const SocketPayload* data, DTLSConnection* connection)
    {
        std::scoped_lock sslLock(connection->m_sslMutex);

        auto packet = std::make_unique<DataReceived>();
        packet->connection = connection;

        bool dispatchPacket = true;
        auto result = connection->ProcessData(*data, packet->payload);
        if (result == DTLSConnection::ProcessResult::Decrypted)
        {
            packet->error = S_OK;
        }
        else if (result == DTLSConnection::ProcessResult::ConnectionClosed)
        {
            packet->error = E_ABORT;
        }
        else if (result == DTLSConnection::ProcessResult::Error)
        {
            packet->error = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
        else
        {
            // Only the above cases get reported back to the title.  The rest are for internal use.
            dispatchPacket = false;
        }


        if (dispatchPacket)
        {
            std::scoped_lock packetLock(m_packetLock);
            m_decryptedPackets.push_back(std::move(packet));
        }
    }

    HRESULT DTLSSocket::InternalSend(const DTLSConnection* destination, const SocketPayload* payload)
    {
        return m_socket->SendTo(&destination->m_destinationAddress, payload);
    }

    void DTLSSocket::InternalAcceptConnectionHandler(const SOCKADDR* address, const SocketPayload* data) noexcept
    {
        if (m_acceptCallback)
        {
            m_acceptCallback(this, address, data, m_acceptHandlerContext);
        }
    }

    HRESULT DTLSSocket::CreateSelfSignedCertificate() noexcept
    {
        HRESULT hr = S_OK;

        m_key = EVP_PKEY_new();

        if (!m_key)
        {
            hr = E_OUTOFMEMORY;
        }

        BIGNUM* number = nullptr;

        if (SUCCEEDED(hr))
        {
            number = BN_new();

            if (!number)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                BN_set_word(number, RSA_F4);
            }
        }

        RSA* rsa = nullptr;

        if (SUCCEEDED(hr))
        {
            rsa = RSA_new();

            if (!rsa)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                RSA_generate_key_ex(rsa, 1024, number, nullptr);

                EVP_PKEY_assign_RSA(m_key, rsa);
            }
        }

        if (SUCCEEDED(hr))
        {
            m_cert = X509_new();

            if (!m_cert)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                ASN1_INTEGER_set(X509_get_serialNumber(m_cert), 1);

                X509_gmtime_adj(X509_get_notBefore(m_cert), 0);
                // Certificate lifetime of 4 hours was picked to match the lifetime on an XSTS token
                X509_gmtime_adj(X509_get_notAfter(m_cert), 4 * 60 * 60);

                X509_set_pubkey(m_cert, m_key);

                X509_NAME* name = X509_get_subject_name(m_cert);

                auto hostname = GetLocalHostName();

                X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<unsigned char*>(hostname.data()), -1, -1, 0);

                X509_set_issuer_name(m_cert, name);

                X509_set_subject_name(m_cert, name);

                X509_sign(m_cert, m_key, EVP_sha256());

                auto digest = EVP_get_digestbyname("sha256");

                m_localFingerprint.resize(static_cast<uint32_t>(EVP_MD_size(digest)));

                uint32_t length = 0;
                X509_digest(m_cert, digest, m_localFingerprint.data(), &length);
            }
        }

        if (FAILED(hr))
        {
            if (m_cert)
            {
                X509_free(m_cert);
            }

            if (m_key)
            {
                EVP_PKEY_free(m_key);
            }

            if (m_sslContext)
            {
                SSL_CTX_free(m_sslContext);
            }
        }

        return hr;
    }
}
