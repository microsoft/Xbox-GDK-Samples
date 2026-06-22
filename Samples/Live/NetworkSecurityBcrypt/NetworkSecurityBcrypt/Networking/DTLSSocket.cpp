//--------------------------------------------------------------------------------------
// File: DtlsSocket.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "SocketPayload.h"
#include "DtlsRecord.h"
#include "DtlsSocket.h"
#include <charconv>
#include <XAsyncProvider.h>
#include "UdpSocket.h"
#include "DtlsConnection.h"
#include "DtlsCreateConnectionAsyncOp.h"
#include "strsafe.h"
#include <intsafe.h>

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

    std::string BytesToHexStringDelim(const uint8_t* bytes, size_t length, char delim)
    {
        // 3 characters per byte
        std::string out(length * 3, ' ');

        for (size_t i = 0; i < length; ++i)
        {
            out[3 * i] = c_hexmap[(bytes[i] & 0xF0) >> 4];
            out[3 * i + 1] = c_hexmap[bytes[i] & 0x0F];
            out[3 * i + 2] = delim;
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

    struct DtlsSendAsyncOp
    {
        SocketPayload payload{};
        DtlsConnection* destination = nullptr;

        HRESULT DoWork() noexcept
        {
            try
            {
                destination->SendApplicationData(payload);
            }
            catch (DtlsException& ex)
            {
                DEBUGLOG("DtlsSendAsyncOp:DoWork:SendApplicationData Failed: %s", ex.what());
                return ex.hr();
            }

            return S_OK;
        }

        HRESULT GetResult() noexcept
        {
            return S_OK;
        }

        static HRESULT Provider(_In_ XAsyncOp op, _In_ const XAsyncProviderData* data) noexcept
        {
            auto context = static_cast<DtlsSendAsyncOp*>(data->context);

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
                HRESULT hr = context->GetResult();
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
                HRESULT hr = context->DoWork();
                XAsyncComplete(data->async, hr, 0);
            }
            break;
            }

            return S_OK;
        }
    };

    HRESULT DtlsSocket::Create(uint16_t port, XTaskQueueHandle queue, DtlsSocket** socket) noexcept
    {
        DEBUGLOG("DtlsSocket::Create");

        auto dtlsSocket = std::make_unique<DtlsSocket>();

        if (queue)
        {
            XTaskQueueDuplicateHandle(queue, &dtlsSocket->m_queue);
        }

        auto hr = dtlsSocket->CreateSelfSignedCertificate();
        if (FAILED(hr))
        {
            return hr;
        }

        UdpSocket* internalSocket{ nullptr };
        hr = UdpSocket::Create(port, &internalSocket);

        if (SUCCEEDED(hr))
        {
            dtlsSocket->m_socket.reset(internalSocket);
            *socket = dtlsSocket.get();
            dtlsSocket.release();
        }

        return hr;
    }

    HRESULT DtlsSocket::Create(uint16_t port, XTaskQueueHandle queue, std::string_view certFile, std::string_view keyFile, DtlsSocket** socket) noexcept
    {
        DEBUGLOG("DtlsSocket::Create");

        auto dtlsSocket = std::make_unique<DtlsSocket>();

        if (queue)
        {
            XTaskQueueDuplicateHandle(queue, &dtlsSocket->m_queue);
        }

        // Specify the certificate file
        auto hr = dtlsSocket->CreateCertficateContextFromFile(certFile);
        if (FAILED(hr))
        {
            DEBUGLOG("DtlsSocket::Create: Unable to load certificate: %s, error: 0x%x", certFile.data(), hr);
            return hr;
        }

        hr = dtlsSocket->GenerateFingerprintFromContext();

        if (FAILED(hr))
        {
            DEBUGLOG("DtlsSocket::Create: Unable to generate certificate fingerprint: 0x%x", hr);
            return hr;
        }

        hr = dtlsSocket->GenerateSubjectNameFromContext();

        if (FAILED(hr))
        {
            DEBUGLOG("DtlsSocket::Create: Unable to retrieve subject name from certificate: 0x%x", hr);
            return hr;
        }

        // Load and verify private key file
        hr = dtlsSocket->CreatePrivateKeyHandleFromFile(keyFile);

        if (FAILED(hr))
        {
            DEBUGLOG("DtlsSocket::Create: Unable to load private key: %s, error: 0x%x", certFile.data(), hr);
            return hr;
        }

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

    DtlsSocket::~DtlsSocket() noexcept
    {
        DEBUGLOG("DtlsSocket::~DtlsSocket");

        m_connections.clear();

        if (m_localCertContext)
        {
            CertFreeCertificateContext(m_localCertContext);
            m_localCertContext = NULL;
        }

        if (m_privateKeyHandle)
        {
            NCryptDeleteKey(m_privateKeyHandle, 0);
            m_privateKeyHandle = NULL;
        }
        if (m_queue)
        {
            XTaskQueueCloseHandle(m_queue);
            m_queue = nullptr;
        }
    }

    void DtlsSocket::GetAddress(SOCKADDR* address) const noexcept
    {
        m_socket->GetAddress(address);
    }

    uint32_t DtlsSocket::GetFingerprintSize() const noexcept
    {
        return static_cast<uint32_t>(m_localFingerprint.size());
    }

    HRESULT DtlsSocket::GetFingerprint(uint8_t* buffer, uint32_t bufferSize, uint32_t* bufferUsed) const noexcept
    {
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

    uint32_t DtlsSocket::GetSubjectNameSize() const noexcept
    {
        return static_cast<uint32_t>(m_localSubjectName.size());
    }

    HRESULT DtlsSocket::GetSubjectName(uint8_t* buffer, uint32_t bufferSize, uint32_t* bufferUsed) const noexcept
    {
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

    void DtlsSocket::AcceptConnections(DtlsAcceptConnectionHandler callback, void* context) noexcept
    {
        DEBUGLOG("DtlsSocket::AcceptConnections");

        m_acceptCallback = callback;
        m_acceptHandlerContext = context;

        if (callback != nullptr)
        {
            m_socket->AcceptConnections([](UdpSocket*, const SOCKADDR* address, const SocketPayload* data, void* context)
            {
                reinterpret_cast<DtlsSocket*>(context)->InternalAcceptConnectionHandler(address, data);
            }, this, m_queue);
        }
        else
        {
            m_socket->AcceptConnections(nullptr, nullptr, nullptr);
        }
    }

    void DtlsSocket::AllowConnectionFrom(const SOCKADDR* destination) noexcept
    {
        DEBUGLOG("DtlsSocket::AllowConnectionFrom");

        // Send a packet to punch a hole in the firewall.
        m_socket->SendTo(destination, {});
    }

    HRESULT DtlsSocket::AcceptConnectionAsync(const SOCKADDR* inDestination, const std::string& expectedIdentityString, const SocketPayload* payload, XAsyncBlock* async) noexcept
    {
        DEBUGLOG("DtlsSocket::AcceptConnectionAsync");

        if (inDestination == nullptr)
        {
            DEBUGLOG("DtlsSocket::AcceptConnectionAsync: inDestination was null");
            return E_INVALIDARG;
        }

        if (expectedIdentityString.empty())
        {
            DEBUGLOG("DtlsSocket::AcceptConnectionAsync: expectedIdentityString was empty");
            return E_INVALIDARG;
        }

        HRESULT hr = S_OK;
        auto connection = CreateNewConnection(inDestination, true, expectedIdentityString);

        // Create new connection will return null if there's already and existing connection with this address
        if (connection == nullptr)
        {
            DEBUGLOG("DtlsSocket::CreateConnectionAsync: AcceptConnectionAsync was null");
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }

        auto context = std::make_shared<DtlsCreateConnectionAsyncOp>();
        context->connection = connection;
        connection->m_createConnectionOp = context;

        try
        {
            connection->StartServerHandshake(*payload);
        }
        catch (DtlsException& ex)
        {
            hr = ex.hr();
            DEBUGLOG("DtlsSendAsyncOp:DoWork:StartServerHandshake DtlsException: (0x%x) %s: %s", hr, ex.what(), GetErrorMessage(hr).c_str());
        }

        if (SUCCEEDED(hr))
        {
            hr = XAsyncBegin(async, context.get(), nullptr, __FUNCTION__, DtlsCreateConnectionAsyncOp::Provider);
        }

        if (FAILED(hr))
        {
            context->connection = nullptr;
            connection->m_createConnectionOp = nullptr;
            CloseConnection(connection.get());
        }

        return hr;
    }

    HRESULT DtlsSocket::AcceptConnectionAsyncResult(XAsyncBlock* async, DtlsConnectionHandle* connection) noexcept
    {
        DEBUGLOG("DtlsSocket::AcceptConnectionAsyncResult");

        return XAsyncGetResult(async, nullptr, sizeof(*connection), connection, nullptr);
    }

    HRESULT DtlsSocket::CreateConnectionAsync(const SOCKADDR* inDestination, const std::string& expectedIdentityString, XAsyncBlock* async) noexcept
    {
        DEBUGLOG("DtlsSocket::CreateConnectionAsync");

        if (inDestination == nullptr)
        {
            DEBUGLOG("DtlsSocket::CreateConnectionAsync: inDestination was null");
            return E_INVALIDARG;
        }

        if (expectedIdentityString.empty())
        {
            DEBUGLOG("DtlsSocket::CreateConnectionAsync: expectedIdentityString was empty");
            return E_INVALIDARG;
        }

        auto connection = CreateNewConnection(inDestination, false, expectedIdentityString);

        // Create new connection will return null if there's already an existing connection with this address
        if (connection == nullptr)
        {
            DEBUGLOG("DtlsSocket::CreateConnectionAsync: destination was null");
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }

        auto context = std::make_shared<DtlsCreateConnectionAsyncOp>();
        context->connection = connection;
        connection->m_createConnectionOp = context;

        // Start off the handshake process for the client
        HRESULT hr = S_OK;

        try
        {
            connection->StartClientHandshake();
        }
        catch (DtlsException& ex)
        {
            hr = ex.hr();
            DEBUGLOG("DtlsSendAsyncOp:DoWork:StartClientHandshake DtlsException: (0x%x) %s: %s", hr, ex.what(), GetErrorMessage(hr).c_str());
        }

        if (SUCCEEDED(hr))
        {
            hr = XAsyncBegin(async, context.get(), nullptr, __FUNCTION__, DtlsCreateConnectionAsyncOp::Provider);
        }

        if (FAILED(hr))
        {
            context->connection = nullptr;
            connection->m_createConnectionOp = nullptr;
            CloseConnection(connection.get());
        }

        return hr;
    }

    HRESULT DtlsSocket::CreateConnectionAsyncResult(XAsyncBlock* async, DtlsConnectionHandle* connection) noexcept
    {
        DEBUGLOG("DtlsSocket::CreateConnectionAsyncResult");

        return XAsyncGetResult(async, nullptr, sizeof(*connection), connection, nullptr);
    }

    HRESULT DtlsSocket::GetConnectionMTU(const DtlsConnectionHandle inConnection, uint32_t* inMtu) noexcept
    {
        DEBUGLOG("DtlsSocket::GetConnectionMTU");

        if (inConnection == nullptr || inMtu == nullptr)
        {
            DEBUGLOG("DtlsSocket::GetConnectionMTU: inConnection was null");
            return E_INVALIDARG;
        }

        if (inMtu == nullptr)
        {
            DEBUGLOG("DtlsSocket::GetConnectionMTU: inMtu was null");
            return E_INVALIDARG;
        }

        // Ensure that the connection has been established. There really shouldn't be a way to hit this.
        if (inConnection->m_connectionState != DtlsConnection::ConnectionState::Established)
        {
            DEBUGLOG("DtlsSocket::GetConnectionMTU: connection was not established");

            return HRESULT_FROM_WIN32(ERROR_CONNECTION_UNAVAIL);
        }

        *inMtu = inConnection->m_maxMTU;

        return S_OK;
    }

    HRESULT DtlsSocket::CloseConnection(DtlsConnectionHandle connection) noexcept
    {
        DEBUGLOG("DtlsSocket::CloseConnection");

        std::scoped_lock lock(m_connectionLock);

        bool found = false;
        for (auto& con : m_connections)
        {
            if (con.get() == connection)
            {
                connection->ShutdownConnection();
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

    HRESULT DtlsSocket::SendToAsync(DtlsConnectionHandle inDestination, const SocketPayload* inPayload, XAsyncBlock* async) noexcept
    {
        DEBUGLOG("DtlsSocket::SendToAsync");

        if (inDestination == nullptr)
        {
            DEBUGLOG("DtlsSocket::SendToAsync: inDestination was null");
            return E_INVALIDARG;
        }

        if (inPayload->Size() > inDestination->m_maxMTU)
        {
            DEBUGLOG("DtlsSocket::SendToAsync: payload was too large to send");
            return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        }

        // Ensure that the connection has been established.
        if (inDestination->m_connectionState != DtlsConnection::ConnectionState::Established)
        {
            DEBUGLOG("DtlsSocket::SendToAsync: connection was not established");
            return HRESULT_FROM_WIN32(ERROR_CONNECTION_UNAVAIL);
        }

        DEBUGLOG("Sending: %s", ATG::BytesToHexStringDelim(inPayload->payload.data(), inPayload->payload.size(), ':').c_str());

        auto op = std::make_unique<DtlsSendAsyncOp>();
        op->destination = inDestination;
        op->payload.payload = inPayload->payload;

        HRESULT hr = XAsyncBegin(async, op.get(), nullptr, __FUNCTION__, DtlsSendAsyncOp::Provider);
        if (SUCCEEDED(hr))
        {
            op.release();
        }

        return hr;
    }

    HRESULT DtlsSocket::SendToAsyncResult(XAsyncBlock* async) noexcept
    {
        DEBUGLOG("DtlsSocket::SendToAsyncResult");

        return XAsyncGetResult(async, nullptr, 0, nullptr, nullptr);
    }

    HRESULT DtlsSocket::RecvFrom(DtlsConnectionHandle* source, SocketPayload* payload) noexcept
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
            (*payload) = packet->payload;
        }

        m_decryptedPackets.erase(m_decryptedPackets.begin());

        return hr;
    }

    std::shared_ptr<DtlsConnection> DtlsSocket::CreateNewConnection(const SOCKADDR* destination, bool asServer, const std::string& expectedIdentityString)
    {
        DEBUGLOG("DtlsSocket::CreateNewConnection");

        auto connection = std::make_shared<DtlsConnection>();
        connection->m_socket = this;
        connection->m_destinationAddress = *destination;
        connection->m_expectedIdentityString = expectedIdentityString;

        {
            // Ensure that no other connections exists for the same address
            std::scoped_lock lock(m_connectionLock);
            auto existing = std::find_if(m_connections.begin(), m_connections.end(), [destination](const std::shared_ptr<DtlsConnection>& connections)
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

        // Initialize this connection
        connection->InitSSL(asServer);

        // Start listening for data on this connection;
        m_socket->RegisterDataReceiver(destination, connection.get(), DtlsSocket::DataReceiverStatic, m_queue, &connection->m_token);

        return connection;
    }

    void DtlsSocket::DataReceiverStatic(UdpSocket*, const SOCKADDR*, const SocketPayload* data, void* context)
    {
        auto connection = reinterpret_cast<DtlsConnection*>(context);
        connection->m_socket->DataReceiver(data, connection);
    }

    void DtlsSocket::DataReceiver(const SocketPayload* data, DtlsConnection* connection)
    {
        std::scoped_lock sslLock(connection->m_sslMutex);

        auto packet = std::make_unique<DataReceived>();
        packet->connection = connection;

        bool dispatchPacket = true;
        auto result = connection->ProcessData(*data, packet->payload);
        if (result == DtlsConnection::ProcessResult::Decrypted)
        {
            packet->error = S_OK;
        }
        else if (result == DtlsConnection::ProcessResult::ConnectionClosed)
        {
            packet->error = E_ABORT;
        }
        else if (result == DtlsConnection::ProcessResult::Error)
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

    HRESULT DtlsSocket::InternalSend(const DtlsConnection* destination, const SocketPayload* payload)
    {
        return m_socket->SendTo(&destination->m_destinationAddress, payload);
    }

    void DtlsSocket::InternalAcceptConnectionHandler(const SOCKADDR* address, const SocketPayload* data) noexcept
    {
        if (m_acceptCallback)
        {
            m_acceptCallback(this, address, data, m_acceptHandlerContext);
        }
    }

    HRESULT DtlsSocket::GenerateSubjectNameFromContext()
    {
        DEBUGLOG("DtlsSocket::GenerateSubjectNameFromContext");

        if (m_localCertContext == nullptr)
        {
            DEBUGLOG("DtlsSocket::CreateCertificateFingerprint: m_certContext was null");
            return E_FAIL;
        }

        HRESULT hr = S_OK;

        DWORD nameSize = CertGetNameStringA(m_localCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, nullptr, 0);
        if (nameSize > 1)
        {
            m_localSubjectName.resize(nameSize);

            CertGetNameStringA(m_localCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, reinterpret_cast<LPSTR>(m_localSubjectName.data()), static_cast<DWORD>(m_localSubjectName.size()));

            m_localSubjectName.pop_back(); // remove null terminator
        }
        else
        {
            DEBUGLOG("DtlsSocket::GenerateSubjectNameFromContext: CertGetNameStringA unable to retrieve CERT_NAME_SIMPLE_DISPLAY_TYPE");
            m_localSubjectName.clear();

            hr = E_FAIL;
        }

        return hr;
    }

    HRESULT DtlsSocket::GenerateFingerprintFromContext()
    {
        DEBUGLOG("DtlsSocket::CreateCertificateFingerprint");

        if (m_localCertContext == nullptr)
        {
            DEBUGLOG("DtlsSocket::CreateCertificateFingerprint: m_certContext was null");
            return E_FAIL;
        }

        HRESULT hr = S_OK;

        DWORD computedHashSize = c_maxCertificateFingerprintSize;
        BYTE computedHashBuffer[c_maxCertificateFingerprintSize];

        if (CryptHashCertificate2(BCRYPT_SHA256_ALGORITHM, 0, nullptr, m_localCertContext->pbCertEncoded, m_localCertContext->cbCertEncoded, computedHashBuffer, &computedHashSize))
        {
            m_localFingerprint.resize(computedHashSize);
            memcpy(m_localFingerprint.data(), computedHashBuffer, computedHashSize);
        }
        else
        {
            m_localFingerprint.clear();

            DWORD error = GetLastError();
            DEBUGLOG("DtlsSocket::GenerateFingerprintFromContext: CryptHashCertificate2 failed with error %d", error);
            hr = HRESULT_FROM_WIN32(error);
        }

        return hr;
    }

    HRESULT DtlsSocket::EncodeSubjectCommonName(std::string_view subjectCommonName, uint32_t maxOutputBufferSize, void* outputBuffer, uint32_t* outputBufferSize)
    {
        DEBUGLOG("DtlsSocket::EncodeSubjectCommonName");

        if (subjectCommonName.empty())
        {
            DEBUGLOG("DtlsSocket::EncodeSubjectCommonName: subjectCommonName was empty");
            return E_INVALIDARG;
        }

        HRESULT hr = S_OK;

        assert(((maxOutputBufferSize == 0) && (outputBuffer == nullptr)) || ((maxOutputBufferSize > 0) && (outputBuffer != nullptr)));
        assert(outputBufferSize != nullptr);

        assert(sizeof(*outputBufferSize) == sizeof(DWORD));
        *outputBufferSize = maxOutputBufferSize;
        LPCSTR err;

        if (!CertStrToNameA(X509_ASN_ENCODING, subjectCommonName.data(), CERT_X500_NAME_STR, nullptr, (BYTE*)outputBuffer, (DWORD*)outputBufferSize, &err))
        {
            DWORD error = GetLastError();
            DEBUGLOG("DtlsSocket::EncodeSubjectCommonName: Error %u encoding subject common name \"%s\"!", error, subjectCommonName.data());
            hr = HRESULT_FROM_WIN32(error);
            if (SUCCEEDED(hr))
            {
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            if (*outputBufferSize == 0)
            {
                DEBUGLOG("DtlsSocket::EncodeSubjectCommonName: Encoded subject name size is empty!");
                hr = E_INVALIDARG;
            }
            else
            {
                DEBUGLOG("DtlsSocket::EncodeSubjectCommonName: Encoded subject common name size is %u, max output buffer size %u.", *outputBufferSize, maxOutputBufferSize);
                if (*outputBufferSize > maxOutputBufferSize)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                }
                else
                {
                    hr = S_OK;
                }
            }
        }

        return hr;
    }

    HRESULT DtlsSocket::CreateSelfSignedCertificate() noexcept
    {
        DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate");

        // Free any previous certificate we might have.
        if (m_localCertContext != nullptr)
        {
            CertFreeCertificateContext(m_localCertContext);
            m_localCertContext = nullptr;
        }

        // Generate a random subject name string.
        HRESULT hr = S_OK;
        std::string name;
        name.resize(c_maxCertificateSubjectNameSize);

        // Try to retrieve a random 64 bit value using the default system random number generator.
        uint64_t randomId = 0;
        NTSTATUS status = BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&randomId), sizeof(randomId), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (status >= 0)
        {
            // Format the 64-bit number as a hexadecimal string into the temporary buffer.
            hr = StringCchPrintfA(name.data(), name.size(), "CN=R%016llx", randomId);
            if (FAILED(hr))
            {
                // This case really shouldn't happen, but fail regardless.
                DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate: Couldn't format subject name buffer!");
            }

            name.resize(strlen(name.c_str()));
        }
        else
        {
            hr = HRESULT_FROM_NT(status);
            DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate: Couldn't generate random ID: (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
            assert(FAILED(hr));
        }

        if (SUCCEEDED(hr))
        {
            DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate: Encoding subject name \"%s\".", name.c_str());

            // Now encode the certificate subject common name into a stack buffer.
            BYTE encodedCertNameBuffer[c_maxEncodedCertificateSubjectNameSize];
            uint32_t encodedCertNameSize;

            hr = EncodeSubjectCommonName(name, sizeof(encodedCertNameBuffer), encodedCertNameBuffer, &encodedCertNameSize);
            if (FAILED(hr))
            {
                DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate: Failed encoding subject common name \"%s\"!", name.c_str());
            }
            else
            {
                // Begin with the current filetime.  Subtract the maximum allowed clock skew from now to determine when the
                // certificate should have been considered valid.  Add the maximum certificate lifetime to now to
                // determine when the certificate should expire.
                assert(sizeof(FILETIME) == sizeof(LARGE_INTEGER));

                union
                {
                    FILETIME asFileTime;
                    LARGE_INTEGER asLargeInteger;
                } validityStartTime;

                union
                {
                    FILETIME asFileTime;
                    LARGE_INTEGER asLargeInteger;
                } validityEndTime;

                GetSystemTimeAsFileTime(&validityStartTime.asFileTime);
                validityEndTime.asFileTime = validityStartTime.asFileTime;
                validityStartTime.asLargeInteger.QuadPart -= c_maxClockSkewIn100ns;
                validityEndTime.asLargeInteger.QuadPart += c_certificateLifetimeIn100ns;

                // Convert both to system times as needed.  If either fail somehow, just fallback to using the defaults values (start == now, end == 1 year).
                SYSTEMTIME validityStartSystemTime;
                SYSTEMTIME validityEndSystemTime;
                SYSTEMTIME* validityStartSystemTimeToUse;
                SYSTEMTIME* validityEndSystemTimeToUse;

                if (!FileTimeToSystemTime(&validityStartTime.asFileTime, &validityStartSystemTime))
                {
                    DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate: Couldn't convert validity start file time 0x%016I64x to system time!  Using default (current time) instead.", validityStartTime.asLargeInteger.QuadPart);
                    validityStartSystemTimeToUse = nullptr;
                }
                else
                {
                    validityStartSystemTimeToUse = &validityStartSystemTime;
                }

                if (!FileTimeToSystemTime(&validityEndTime.asFileTime, &validityEndSystemTime))
                {
                    DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate: Couldn't convert validity end file time 0x%016I64x to system time!  Using default (1 year) instead.", validityEndTime.asLargeInteger.QuadPart);
                    validityEndSystemTimeToUse = nullptr;
                }
                else
                {
                    validityEndSystemTimeToUse = &validityEndSystemTime;
                }

                // Create a new certificate using the specified certificate name blob, the determined validity period, and default settings for everything else.
                CERT_NAME_BLOB certNameBlob;
                certNameBlob.cbData = encodedCertNameSize;
                certNameBlob.pbData = encodedCertNameBuffer;

                m_localCertContext = CertCreateSelfSignCertificate(NULL, &certNameBlob, 0, nullptr, nullptr, validityStartSystemTimeToUse, validityEndSystemTimeToUse, nullptr);
                if (m_localCertContext == nullptr)
                {
                    DWORD error = GetLastError();
                    DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate: Couldn't have Cert API create self-signed certificate (err %u)!", error);

                    hr = E_FAIL;
                }
                else
                {
                    // Now generate a hash of the certificate bytes to serve as a fingerprint and store it in our member variable.
                    hr = GenerateFingerprintFromContext();
                    if (FAILED(hr))
                    {
                        DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate: Failed hashing newly created certificate context 0x%p!", m_localCertContext);

                        // Delete the certificate, we can't use it.
                        CertFreeCertificateContext(m_localCertContext);

                        m_localCertContext = nullptr;
                    }

                    hr = GenerateSubjectNameFromContext();
                    if (FAILED(hr))
                    {
                        DEBUGLOG("DtlsSocket::CreateSelfSignedCertificate: Failed to generate subject name context 0x%p!", m_localCertContext);
                    }
                }
            }
        }

        return hr;
    }

    HRESULT DtlsSocket::CreateCertficateContextFromFile(std::string_view filePath)
    {
        DEBUGLOG("DtlsSocket::CreateCertficateContextFromFile");

        HRESULT hr = S_OK;
        auto fileBlob = std::vector<uint8_t>();

        // Load and decode the cert file
        hr = LoadAndDecodeBlobFromFile(filePath, fileBlob);

        // Create a certificate context from the decrypted cert binary
        if (SUCCEEDED(hr))
        {
            m_localCertContext = CertCreateCertificateContext(
                X509_ASN_ENCODING,                          // Accepted certificate encodings
                fileBlob.data(),                            // Pointer to binary encrypted certificate
                static_cast<DWORD>(fileBlob.size()));       // The size of the cert buffer

            if (m_localCertContext == NULL)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("CertCreateCertificateContext failed with 0x%x.", hr);
            }
        }
        else
        {
            DEBUGLOG("CreateCertficateContextFromFile failed with 0x%x.", hr);
        }

        return hr;
    }

    HRESULT DtlsSocket::CreatePrivateKeyHandleFromFile(std::string_view filePath)
    {
        DEBUGLOG("DtlsSocket::CreatePrivateKeyHandleFromFile");

        HRESULT hr = S_OK;
        auto fileBlob = std::vector<uint8_t>();

        // Load and base64 decode the key file
        hr = LoadAndDecodeBlobFromFile(filePath, fileBlob);

        DWORD bytesNeeded = 0;

        // Determine the size of buffer required to hold the decoded key info
        if (SUCCEEDED(hr))
        {
            auto status = CryptDecodeObjectEx(
                X509_ASN_ENCODING,                          // Encoding type
                PKCS_PRIVATE_KEY_INFO,                      // Struct type of the data
                fileBlob.data(),                            // Input data buffer
                static_cast<DWORD>(fileBlob.size()),        // Input data buffer size
                0,                                          // No flags specified
                NULL,                                       // No callback used
                NULL,                                       // Pointer to output buffer, NULL to return needed size
                &bytesNeeded);                              // Returns the size needed for the decode buffer

            if (status == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("CryptDecodeObjectEx failed with 0x%x.", hr);
            }
        }

        auto keyBlob = std::vector<uint8_t>();

        // Decode the key info
        if (SUCCEEDED(hr))
        {
            keyBlob.resize(bytesNeeded);

            auto status = CryptDecodeObjectEx(
                X509_ASN_ENCODING,                          // Encoding type
                PKCS_PRIVATE_KEY_INFO,                      // Struct type of the data
                fileBlob.data(),                            // Input data buffer
                static_cast<DWORD>(fileBlob.size()),        // Input data buffer size
                0,                                          // No flags specified
                NULL,                                       // No callback used
                keyBlob.data(),                             // Pointer to output buffer
                &bytesNeeded);                              // Size of the output buffer

            if (status == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("CryptDecodeObjectEx failed with 0x%x.", hr);
            }
        }

        auto pPriKeyInfo = (CRYPT_PRIVATE_KEY_INFO*)keyBlob.data();

        // Get the size of buffer needed to decode info object to key blob
        if (SUCCEEDED(hr))
        {
            auto status = CryptDecodeObjectEx(
                X509_ASN_ENCODING,                  // Encoding type
                CNG_RSA_PRIVATE_KEY_BLOB,           // Struct type of the data
                pPriKeyInfo->PrivateKey.pbData,     // Input data buffer
                pPriKeyInfo->PrivateKey.cbData,     // Input data buffer size
                0,                                  // No flags specified
                NULL,                               // No callback used
                NULL,                               // Pointer to output buffer, NULL to return needed size
                &bytesNeeded);                      // Returns the size needed for the decode buffer

            if (status == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("CryptDecodeObjectEx failed with 0x%x.", hr);
            }
        }

        auto cngKeyBlob = std::vector<uint8_t>();

        // Decode info object to key blob
        if (SUCCEEDED(hr))
        {
            cngKeyBlob.resize(bytesNeeded);

            auto status = CryptDecodeObjectEx(
                X509_ASN_ENCODING,                  // Encoding type
                CNG_RSA_PRIVATE_KEY_BLOB,           // Struct type of the data
                pPriKeyInfo->PrivateKey.pbData,     // Input data buffer
                pPriKeyInfo->PrivateKey.cbData,     // Input data buffer size
                0,                                  // No flags specified
                NULL,                               // No callback used
                cngKeyBlob.data(),                  // Pointer to output buffer that receives data
                &bytesNeeded);                      // Size of the output buffer

            if (status == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("CryptDecodeObjectEx failed with 0x%x.", hr);
            }
        }

        NCRYPT_PROV_HANDLE keyProvider = NULL;

        // Open a key storage provider
        if (SUCCEEDED(hr))
        {
            auto status = NCryptOpenStorageProvider(
                &keyProvider,               // Receives the provider handle
                MS_KEY_STORAGE_PROVIDER,    // Built-in 'software' key storage provider
                0);                         // No flags specified

            if (status != ERROR_SUCCESS)
            {
                hr = status;
                DEBUGLOG("NCryptOpenStorageProvider failed with 0x%x.", hr);
            }
        }

        NCRYPT_KEY_HANDLE keyHandle = NULL;

        // Create a new key
        if (SUCCEEDED(hr))
        {
            auto status = NCryptCreatePersistedKey(
                keyProvider,                        // Key provider handle
                &keyHandle,                         // Returned key handle
                NCRYPT_RSA_ALGORITHM,               // Use RSA
                c_privateKeyContainerName,          // Reference name for the key
                0,                                  // No legacy key spec
                NCRYPT_OVERWRITE_KEY_FLAG);         // Overwrite any key of the same name

            if (status != ERROR_SUCCESS)
            {
                hr = status;
                DEBUGLOG("NCryptCreatePersistedKey failed with 0x%x.", hr);
            }
        }

        // Import the key material
        if (SUCCEEDED(hr))
        {
            auto status = NCryptSetProperty(
                keyHandle,                                  // Key handle to use
                BCRYPT_RSAPRIVATE_BLOB,                     // Setting the RSA private key material
                reinterpret_cast<PBYTE>(cngKeyBlob.data()), // Key blob pointer
                static_cast<DWORD>(cngKeyBlob.size()),      // Key blob size
                0);                                         // No flags

            if (status != ERROR_SUCCESS)
            {
                hr = status;
                DEBUGLOG("NCryptSetProperty failed with 0x%x.", hr);
            }
        }

        // Finalize the key
        if (SUCCEEDED(hr))
        {
            auto status = NCryptFinalizeKey(keyHandle, 0);  // Finalze key handle

            if (status != ERROR_SUCCESS)
            {
                hr = status;
                DEBUGLOG("NCryptFinalizeKey failed with 0x%x.", hr);
            }
        }

        // Attach key info to certificate
        if (SUCCEEDED(hr))
        {
            CRYPT_KEY_PROV_INFO keyProvInfo{};

            keyProvInfo.pwszContainerName = const_cast<wchar_t*>(c_privateKeyContainerName);
            keyProvInfo.pwszProvName = const_cast<LPWSTR>(MS_KEY_STORAGE_PROVIDER);

            auto status = CertSetCertificateContextProperty(
                m_localCertContext,             // The certificate context
                CERT_KEY_PROV_INFO_PROP_ID,     // Key provider info property
                0,                              // No flags
                &keyProvInfo);                  // Pointer to key info structure

            if (!status)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("CertSetCertificateContextProperty failed with 0x%x.", hr);
            }
        }

        // Open the private key handle
        if (SUCCEEDED(hr))
        {
            auto status = NCryptOpenKey(
                keyProvider,                    // The NCrypt key provider
                &m_privateKeyHandle,            // The received key handle
                c_privateKeyContainerName,      // The certificate context
                0, 0);                          // No flags

            if (status != ERROR_SUCCESS)
            {
                hr = status;
                DEBUGLOG("NCryptOpenKey failed with 0x%x.", hr);
            }
        }

        if (keyHandle)
        {
            NCryptFreeObject(keyHandle);
        }

        if (keyProvider)
        {
            NCryptFreeObject(keyProvider);
        }

        if (FAILED(hr))
        {
            DEBUGLOG("CreatePrivateKeyHandleFromFile failed with 0x%x.", hr);
        }

        return hr;
    }

    HRESULT DtlsSocket::LoadAndDecodeBlobFromFile(std::string_view filePath, std::vector<uint8_t>& fileBlob)
    {
        DEBUGLOG("DtlsSocket::LoadAndDecodeBlobFromFile");

        HRESULT hr = S_OK;
        auto fileHandle = INVALID_HANDLE_VALUE;

        // Open the blob file
        fileHandle = CreateFileA(
            filePath.data(),        // File name
            GENERIC_READ,           // Read only access
            FILE_SHARE_READ,        // Other processes can also read the file
            NULL,                   // No security descriptor
            OPEN_EXISTING,          // Only open if file exists
            FILE_ATTRIBUTE_NORMAL,  // Don't open hidden/system files
            NULL);                  // No template file used

        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUGLOG("CreateFileA failed with 0x%x.", hr);
        }

        auto fileSize = INVALID_FILE_SIZE;

        // Get the size of the file
        if (SUCCEEDED(hr))
        {
            fileSize = GetFileSize(fileHandle, NULL);

            if (fileSize == INVALID_FILE_SIZE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("GetFileSize failed with 0x%x.", hr);
            }
        }

        auto fileContent = std::vector<uint8_t>();

        // Read the file contents
        if (SUCCEEDED(hr))
        {
            DWORD bytesRead = 0;

            fileContent.resize(fileSize);

            auto success = ReadFile(
                fileHandle,             // Handle to the file
                fileContent.data(),     // Pointer to data buffer to receive contents
                fileSize,               // Number of bytes to read (the whole thing)
                &bytesRead,             // Receives the number of bytes read
                NULL);                  // No overlapped (async) I/O

            if (success == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("ReadFile failed with 0x%x.", hr);
            }
        }

        // We're done with the file
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(fileHandle);
            fileHandle = NULL;
        }

        DWORD decryptedSize = 0;

        // Get the size of the decoded blob
        if (SUCCEEDED(hr))
        {
            auto success = CryptStringToBinaryA(
                reinterpret_cast<LPCSTR>(fileContent.data()),   // Encoded text buffer
                fileSize,                                       // Size of buffer
                CRYPT_STRING_BASE64_ANY,                        // Decode BASE64 text
                NULL,                                           // Buffer pointer, use NULL to get the needed size
                &decryptedSize,                                 // Buffer size needed is returned here
                NULL,                                           // Skip not used
                NULL);                                          // Conversion flags not used

            if (success == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("CryptStringToBinaryA failed with 0x%x.", hr);
            }
        }

        // Decrypt the encoded blob
        if (SUCCEEDED(hr))
        {
            fileBlob.resize(decryptedSize);

            auto success = CryptStringToBinaryA(
                reinterpret_cast<LPCSTR>(fileContent.data()),   // Encoded text buffer
                fileSize,                                       // Size of buffer
                CRYPT_STRING_BASE64_ANY,                        // Decode BASE64 text
                fileBlob.data(),                                // Buffer pointer to recieve data
                &decryptedSize,                                 // Buffer size
                NULL,                                           // Skip not used
                NULL);                                          // Conversion flags not used

            if (success == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DEBUGLOG("CryptStringToBinaryA failed with 0x%x.", hr);
            }
        }

        return hr;
    }
}
