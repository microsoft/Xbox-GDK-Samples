//--------------------------------------------------------------------------------------
// File: DtlsConnection.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SocketPayload.h"
#include "DtlsRecord.h"
#include "DtlsConnection.h"
#include "DtlsSocket.h"
#include "DtlsCreateConnectionAsyncOp.h"
#include "StringUtil.h"

#pragma warning(disable:4061)

namespace
{
    constexpr uint32_t c_handshakeTimeoutMS = 1000;
}

using namespace ATG;

struct DtlsCreateConnectionAsyncOp;

DtlsConnection::~DtlsConnection()
{
    DEBUGLOG("DtlsConnection::~DtlsConnection");
}

void DtlsConnection::InitSSL(bool isServer)
{
    DEBUGLOG("DtlsConnection::InitSSL");

    m_isServer = isServer;
    m_connectionState = ConnectionState::Negotiating;

    if (isServer)
    {
        m_serverCert = m_socket->m_localCertContext;
        m_serverCertKey = m_socket->m_privateKeyHandle;
        m_HandshakeState = HandshakeState::ParseClientHello;
    }
    else
    {
        m_clientCert = m_socket->m_localCertContext;
        m_HandshakeState = HandshakeState::ParseServerHello;
    }

    // Initialize BCrypt constants
    QueryBCryptObjectSizes();

    // Setup the handshake timeout operation
    m_timeoutOp.Init([this]() -> uint32_t
    {
        DEBUGLOG("TimeoutOp fired!");

        // Still trying to connect
        if (m_connectionState == ConnectionState::Negotiating)
        {
            // Retry sending the last packet
            if (m_handshakeRetries < c_MaxHandshakeRetries)
            {
                std::scoped_lock sslLock(m_sslMutex);

                DEBUGLOG("Resending last packet(s)");

                for (auto packet : m_retryPackets)
                {
                    auto hr = m_socket->InternalSend(this, &packet);
                    if (FAILED(hr))
                    {
                        throw DtlsException(hr, "InternalSend");
                    }
                }

                m_handshakeRetries++;
                return c_handshakeTimeoutMS;
            }
            else
            {
                DEBUGLOG("Too many retries");
                m_connectionState = ConnectionState::Failed;
            }
        }

        // If the connection is marked as failed, clean it up and abort
        if(m_connectionState == ConnectionState::Failed)
        {
            m_socket->CloseConnection(this);

            if (m_createConnectionOp != nullptr)
            {
                DEBUGLOG("DtlsConnection timed out");
                m_createConnectionOp->Complete(HRESULT_FROM_WIN32(ERROR_CONNECTION_UNAVAIL));
            }
        }

        return 0;
    }, m_socket->m_derivedWorkQueue);
}

bool DtlsConnection::IsConnected() const
{
    auto connected = (m_connectionState == ConnectionState::Established);

    DEBUGLOG("DtlsConnection::IsConnected: %s", connected ? "TRUE" : "FALSE");

    return connected;
}

void DtlsConnection::SendPayload(SocketPayload& data)
{
    DEBUGLOG("DtlsConnection::SendPayload: Sending %d bytes", data.Size());
    DEBUGLOG("Sending: %s", ATG::BytesToHexStringDelim(data.payload.data(), data.payload.size(), ':').c_str());

    if (m_encryptSend)
    {
        EncryptData(data);
    }

    if (m_connectionState == ConnectionState::Negotiating)
    {
        m_retryPackets.emplace_back(data);
    }

    auto hr = m_socket->InternalSend(this, &data);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "InternalSend");
    }
}

void DtlsConnection::SendApplicationData(SocketPayload& data)
{
    auto record = std::make_shared<DtlsApplicationMessage>(data.payload, m_epoch, m_writeSequence++);
    record->Finalize();

    data = record->Payload();
    SendPayload(data);
}

DtlsConnection::ProcessResult DtlsConnection::ProcessData(const SocketPayload& inData, SocketPayload& outData)
{
    DEBUGLOG("DtlsConnection::ProcessData: Proccessing %d bytes", inData.Size());
    DEBUGLOG("Received: %s", ATG::BytesToHexStringDelim(inData.payload.data(), inData.payload.size(), ':').c_str());

    auto result = ProcessResult::Error;

    try
    {
        if (inData.Size() > 0)
        {
            auto packet = const_cast<SocketPayload&>(inData);
            auto header = DtlsRecordHeader(packet);
            packet.SetReadLocation(0);

            DEBUGLOG("Processing DTLS packet: Epoch: %d, Sequence: %llu", header.Epoch(), header.Sequence());

            // Make sure it's for the current epoch
            if (header.Epoch() < m_epoch)
            {
                DEBUGLOG("Message is from prior epoch");
                return ProcessResult::WaitingForData;
            }

            // Handle repeated or out of sequence records
            if (header.Sequence() < m_readSequence)
            {
                DEBUGLOG("Message has already been processed");
                return ProcessResult::WaitingForData;
            }
            else if (header.Sequence() > m_readSequence)
            {
                DEBUGLOG("Message is out of sequence, queuing");
                m_pendingPackets.emplace_back(packet);
                return ProcessResult::WaitingForData;
            }

            // Decrypt if necessary
            if (m_encryptRecv)
            {
                DecryptData(packet);
            }

            // Process the message
            result = ProcessDtlsPayload(packet, outData);

            if (result != ProcessResult::Error && !IsConnected())
            {
                auto packets = m_pendingPackets;
                m_pendingPackets.clear();

                // Check the pending queue
                for (auto& payload : packets)
                {
                    header = DtlsRecordHeader(payload);
                    payload.SetReadLocation(0);

                    if (m_encryptRecv)
                    {
                        DecryptData(payload);
                    }

                    if (header.Sequence() == m_readSequence)
                    {
                        DEBUGLOG("Processing queued message");
                        result = ProcessDtlsPayload(payload, outData);

                        if (result == ProcessResult::Error)
                        {
                            break;
                        }
                    }
                    else if (header.Sequence() > m_readSequence)
                    {
                        DEBUGLOG("Requing queued message");
                        m_pendingPackets.emplace_back(payload);
                    }
                    else
                    {
                        assert(false);
                    }
                }
            }

            // Remove retry packets that we've seen
            m_retryPackets.erase(
                std::remove_if(
                    std::begin(m_retryPackets),
                    std::end(m_retryPackets),
                    [&](SocketPayload& p)
                    {
                        auto h = DtlsRecordHeader(p);
                        p.SetReadLocation(0);
                        return h.Sequence() < m_readSequence;
                    }),
                std::end(m_retryPackets));
        }
    }
    catch (DtlsException& ex)
    {
        DEBUGLOG("Caught DtlsException: (0x%x) %s: %s", ex.hr(), ex.what(), GetErrorMessage(ex.hr()).c_str());
        result = ProcessResult::Error;
    }
    catch (std::exception& ex)
    {
        DEBUGLOG("Caught exception: %s", ex.what());
        result = ProcessResult::Error;
    }

    return result;
}

DtlsConnection::ProcessResult DtlsConnection::ProcessDtlsPayload(SocketPayload& packet, SocketPayload& outData)
{
    auto result = ProcessResult::Error;

    // Perform connection handshake if not connected yet
    if (!IsConnected())
    {
        // Read the TLS record type
        auto recordType = packet.Read<DtlsContentType>();

        // Rewind the buffer since the record/message classes expect to parse the type
        packet.SetReadLocation(packet.GetReadLocation() - 1);

        // Check for an alert/abort
        if (recordType == DtlsContentType::Alert)
        {
            auto tlsRecord = std::make_shared<DtlsAlertMessage>(packet);

            DEBUGLOG("DtlsConnection::ProcessData: Received TLS ALERT record, aborting connection.  Level: %d, Description: %d", tlsRecord->AlertLevel(), tlsRecord->AlertDescripiton());

            m_HandshakeState = HandshakeState::None;
            m_connectionState = ConnectionState::Failed;

            if (m_createConnectionOp != nullptr)
            {
                m_createConnectionOp->Complete(E_ABORT);
            }

            return ProcessResult::ConnectionClosed;
        }

        // Process message as part of the handshake
        Handshake(packet);

        // If the handshake is now complete invoke notification
        if (IsConnected())
        {
            DEBUGLOG("DtlsConnection::ProcessData: Connected");

            if (m_createConnectionOp != nullptr)
            {
                m_createConnectionOp->Complete(S_OK);
            }

            // The handshake timeout is no longer needed now that we are connected
            m_timeoutOp.Cancel();

            result = ProcessResult::ConnectionEstablished;
        }
        else
        {
            // Restart the timer
            auto hr = m_timeoutOp.Reset(c_handshakeTimeoutMS);

            DEBUGLOG("DtlsConnection::ProcessData: Timer reset: 0x%08X", hr);
            DEBUGLOG("DtlsConnection::ProcessData: Continue handshake sequence");

            result = ProcessResult::Handshaking;
        }
    }
    else
    {
        // Read the TLS record type
        auto recordType = packet.Read<DtlsContentType>();

        // Rewind the buffer since the record/message classes expect to parse the type
        packet.SetReadLocation(packet.GetReadLocation() - 1);

        // Check for an alert/abort
        if (recordType == DtlsContentType::Alert)
        {
            auto tlsRecord = std::make_shared<DtlsAlertMessage>(packet);

            DEBUGLOG("DtlsConnection::ProcessData: Received TLS ALERT record, aborting connection.  Level: %d, Description: %d", tlsRecord->AlertLevel(), tlsRecord->AlertDescripiton());

            m_HandshakeState = HandshakeState::None;
            m_connectionState = ConnectionState::Failed;

            if (m_createConnectionOp != nullptr)
            {
                m_createConnectionOp->Complete(E_ABORT);
            }

            return ProcessResult::ConnectionClosed;
        }

        if (recordType == DtlsContentType::Application)
        {
            auto record = std::make_shared<DtlsApplicationMessage>(packet);

            // Pass the decrypted data out
            outData.Write(record->MessageData());

            result = ProcessResult::Decrypted;
        }
        else
        {
            DEBUGLOG("DtlsConnection::ProcessData: Received unhandled TLS record type %d", recordType);
        }
    }

    m_readSequence++;

    return result;
}

void DtlsConnection::Handshake(SocketPayload& packet)
{
    // Read the TLS record type
    auto recordType = packet.Read<DtlsContentType>();

    // Rewind the buffer since the record/message classes expect to parse the type
    packet.SetReadLocation(packet.GetReadLocation() - 1);

    // We should receive a ChangeCipherSpec as part of the handshake
    if (recordType == DtlsContentType::ChangeCipherSpec)
    {
        if (m_HandshakeState != HandshakeState::ParseClientFinished &&
            m_HandshakeState != HandshakeState::ParseServerFinished)
        {
            throw DtlsException(E_FAIL, "Received ChangeCipherSpec at an unexpected time");
        }

        // Received packets will be encrypted from now on
        m_encryptRecv = true;

        DEBUGLOG("Received ChangeCipherSpec: Encryption enabled!");

        return;
    }

    // We only expect handshake messages until we're connected
    if (recordType != DtlsContentType::Handshake)
    {
        throw DtlsException(E_FAIL, "Received non-handshake record before connection established");
    }

    // Verify the sequence number(s)
    auto tlsRecord = std::make_shared<DtlsHandshakeMessage>(packet);

    for (auto& message : tlsRecord->Messages())
    {
        auto handshake = std::dynamic_pointer_cast<DtlsHandshake>(message);

        if (handshake->Sequence() == m_expectedHandshakeSequence)
        {
            m_expectedHandshakeSequence++;
        }
        else
        {
            throw DtlsException(E_FAIL, "Invalid handshake sequence");
        }
    }

    switch (m_HandshakeState)
    {
        // Server: Parse the client hello message
        case HandshakeState::ParseClientHello:
        {
            ParseClientHello(tlsRecord);
            SendServerHello();

            m_HandshakeState = HandshakeState::ParseClientKeyExchange;
            break;
        }

        // Client: Parse the server hello message
        case HandshakeState::ParseServerHello:
        {
            ParseServerHello(tlsRecord);
            SendClientKeyExchange();

            m_HandshakeState = HandshakeState::ParseServerFinished;
            break;
        }

        // Server: Parse the client key exchange
        case HandshakeState::ParseClientKeyExchange:
        {
            ParseClientKeyExchange(tlsRecord);

            m_HandshakeState = HandshakeState::ParseClientFinished;
            break;
        }

        // Server: Parse the Finished message.  This occurs after the ChangeCipherSpec and is sent encrypted.
        case HandshakeState::ParseClientFinished:
        {
            ParseClientFinished(tlsRecord);
            SendServerFinished();

            // We are connected now
            m_HandshakeState = HandshakeState::None;
            m_connectionState = ConnectionState::Established;

            // Handshake has finished, move to next epoch and reset sequence counters
            m_epoch++;
            m_writeSequence = 0;
            m_readSequence = (uint64_t)-1;
            break;
        }

        // Client: Parse the server finished message
        case HandshakeState::ParseServerFinished:
        {
            ParseServerFinished(tlsRecord);

            // We are connected now
            m_HandshakeState = HandshakeState::None;
            m_connectionState = ConnectionState::Established;

            // Handshake has finished, move to next epoch and reset sequence counters
            m_epoch++;
            m_writeSequence = 0;
            m_readSequence = (uint64_t)-1;
            break;
        }

        default:
        {
            throw DtlsException(E_FAIL, "Unexpected handshake state");
        }
    }
}

void DtlsConnection::EncryptData(SocketPayload& data)
{
    DEBUGLOG("DtlsConnection::EncryptData");

    // Get the DTLS header
    auto inputHeader = DtlsRecordHeader(data);
    auto inputBodySize = data.GetRemainingReadSize();

    if (inputBodySize > inputHeader.Length())
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid record");
    }

    // Get the record body
    auto inputBody = std::vector<uint8_t>(inputBodySize);
    data.Read(inputBody);

    // Generate the HMAC
    auto macHash = BCryptHashHandle{};
    macHash.set_data_size(m_hmacSha384ObjLen);

    // Create a hash object
    auto hr = BCryptCreateHash(BCRYPT_HMAC_SHA384_ALG_HANDLE, &macHash, macHash, m_hmacSha384ObjLen, m_writeMacBuffer.data(), (ULONG)m_writeMacBuffer.size(), 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptCreateHash");
    }

    // Hash the header
    hr = BCryptHashData(macHash, (PUCHAR)inputHeader.Header.data(), (ULONG)inputHeader.Header.size(), 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptHashData");
    }

    // Hash the content
    hr = BCryptHashData(macHash, (PUCHAR)inputBody.data(), (ULONG)inputBody.size(), 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptHashData");
    }

    // Generate the finalized hash buffer
    auto finishHash = std::vector<uint8_t>(m_hmacSha384HashLen);
    hr = BCryptFinishHash(macHash, (PUCHAR)finishHash.data(), m_hmacSha384HashLen, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptFinishHash");
    }

    // Generate padding
    auto neededPadding = m_aesBlockLen - (inputBodySize + m_hmacSha384HashLen) % m_aesBlockLen;
    auto outputPadding = std::vector<uint8_t>(neededPadding, (uint8_t)(neededPadding - 1));

    // Create the buffer to be encrypted
    auto outputBody = std::vector<uint8_t>();
    outputBody.reserve(inputBody.size() + finishHash.size() + outputPadding.size());
    outputBody.insert(std::end(outputBody), std::begin(inputBody), std::end(inputBody));
    outputBody.insert(std::end(outputBody), std::begin(finishHash), std::end(finishHash));
    outputBody.insert(std::end(outputBody), std::begin(outputPadding), std::end(outputPadding));

    // Generate a random number for the IV
    auto outputIV = std::vector<uint8_t>(m_aesBlockLen);
    hr = BCryptGenRandom(NULL, (PUCHAR)outputIV.data(), m_aesBlockLen, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGenRandom");
    }

    // Perform the encryption [input body + HMAC + padding]
    auto encryptIV = outputIV;
    DWORD bytesWritten{};
    hr = BCryptEncrypt(m_writeKey, (PUCHAR)outputBody.data(), (ULONG)outputBody.size(), NULL, (PUCHAR)encryptIV.data(), (ULONG)encryptIV.size(), (PUCHAR)outputBody.data(), (ULONG)outputBody.size(), &bytesWritten, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptEncrypt");
    }

    // Create the new encrypted DTLS record
    auto outputPayload = SocketPayload{};
    auto outputHeader = DtlsRecordHeader(inputHeader);
    outputHeader.SetLength((uint16_t)(m_aesBlockLen + inputBodySize + m_hmacSha384HashLen + neededPadding));
    outputPayload.Write(outputHeader.Header);
    outputPayload.Write(outputIV);
    outputPayload.Write(outputBody);

    data = outputPayload;
}

void DtlsConnection::DecryptData(SocketPayload& data)
{
    DEBUGLOG("DtlsConnection::DecryptData");

    // Get the DTLS header
    auto inputHeader = DtlsRecordHeader(data);
    auto inputBodySize = data.GetRemainingReadSize();

    if (inputBodySize > inputHeader.Length())
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid record");
    }

    // Get the IV
    auto inputIV = std::vector<uint8_t>(m_aesBlockLen);
    data.Read(inputIV);

    // Get the encrypted payload
    auto inputPayloadSize = inputBodySize - m_aesBlockLen;
    if (inputPayloadSize % m_aesBlockLen)
    {
        // Block size violation
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    auto inputPayload = std::vector<uint8_t>(inputPayloadSize);
    data.Read(inputPayload);

    // Decrypt the payload
    DWORD bytesWritten{};
    auto hr = BCryptDecrypt(m_readKey, (PUCHAR)inputPayload.data(), (ULONG)inputPayloadSize, NULL, (PUCHAR)inputIV.data(), m_aesBlockLen, (PUCHAR)inputPayload.data(), (ULONG)inputPayloadSize, &bytesWritten, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptDecrypt");
    }

    // Find the actual length of the record
    auto inputPaddingSize = inputPayload[inputPayloadSize - 1] + sizeof(uint8_t);
    auto inputPaddingOffset = inputPayloadSize - inputPaddingSize;
    auto outputBodySize = inputPaddingOffset - m_hmacSha384HashLen;

    // Generate the actual record header
    auto outputHeader = DtlsRecordHeader(inputHeader);
    outputHeader.SetLength((uint16_t)outputBodySize);

    // Generate and verify the HMAC
    auto macHash = BCryptHashHandle{};
    macHash.set_data_size(m_hmacSha384ObjLen);

    // Create a hash object
    hr = BCryptCreateHash(BCRYPT_HMAC_SHA384_ALG_HANDLE, &macHash, macHash, m_hmacSha384ObjLen, m_readMacBuffer.data(), (ULONG)m_readMacBuffer.size(), 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptCreateHash");
    }

    // Hash the header
    hr = BCryptHashData(macHash, (PUCHAR)outputHeader.Header.data(), (ULONG)outputHeader.Header.size(), 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptHashData");
    }

    // Perform a time-constant HMAC verification
    hr = BCryptHashData(macHash, (PUCHAR)inputPayload.data(), (ULONG)inputPayload.size(), BCRYPT_TLS_CBC_HMAC_VERIFY_FLAG);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptHashData");
    }

    // Drop the HMAC and padding off the payload
    inputPayload.resize(outputBodySize);

    // Create the unencrypted packet
    auto outputPayload = SocketPayload{};
    outputPayload.Write(outputHeader.Header);
    outputPayload.Write(inputPayload);

    data = outputPayload;
}

void DtlsConnection::GenerateMasterKey()
{
    DEBUGLOG("DtlsConnection::GenerateMasterKey: %s", m_isServer ? "SERVER" : "CLIENT");

    // Perform the ECDHE key exchange and compute the master secret.
    auto secret = BCryptSecretHandle{};
    auto hr = S_OK;

    if (m_isServer)
    {
        hr = BCryptSecretAgreement(m_serverKey, m_clientKey, &secret, 0);
    }
    else
    {
        hr = BCryptSecretAgreement(m_clientKey, m_serverKey, &secret, 0);
    }

    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptSecretAgreement");
    }

    // Get the buffer size needed
    DWORD preKeySize = 0;
    hr = BCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, NULL, 0, &preKeySize, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptDeriveKey");
    }

    // Derive the pre-master key from the secret
    auto preKeyBlob = std::vector<uint8_t>(preKeySize);
    hr = BCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, preKeyBlob.data(), preKeySize, &preKeySize, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptDeriveKey");
    }

    // Strip 'leading' zeroes
    for (; preKeySize > 0 && preKeyBlob[preKeySize - 1] == 0; --preKeySize);

    // Reverse the bytes
    for (auto front = preKeyBlob.data(), back = front + preKeySize - 1; front < back; ++front, --back)
    {
        *front = (uint8_t)(*front ^ *back);
        *back  = (uint8_t)(*front ^ *back);
        *front = (uint8_t)(*front ^ *back);
    }

    // Create the random seed as ClientRandom + ServerRandom
    auto randomSeed = std::vector<uint8_t>();
    randomSeed.reserve(c_RandomLen * 2);
    randomSeed.insert(std::end(randomSeed), std::begin(m_clientRandom), std::end(m_clientRandom));
    randomSeed.insert(std::end(randomSeed), std::begin(m_serverRandom), std::end(m_serverRandom));

    // Create the actual master key
    m_masterKeyMaterial.resize(c_MasterKeyLength);
    GenerateKeyMaterial(preKeyBlob, c_MasterSecret, randomSeed, m_masterKeyMaterial);
}

void DtlsConnection::GenerateSessionKeys()
{
    DEBUGLOG("DtlsConnection::GenerateSessionKeys:");

    // Create the random seed
    auto randomSeed = std::vector<uint8_t>();
    randomSeed.reserve(c_RandomLen * 2);
    randomSeed.insert(std::end(randomSeed), std::begin(m_clientRandom), std::end(m_clientRandom));
    randomSeed.insert(std::end(randomSeed), std::begin(m_serverRandom), std::end(m_serverRandom));

    // Generate enough key material for read/write/mac keys
    auto keyMaterial = std::vector<uint8_t>((BCRYPT_AES_ALGORITHM_BYTE_LEN * 2) + (BCRYPT_SHA384_ALGORITHM_LEN * 2));
    GenerateKeyMaterial(m_masterKeyMaterial, c_KeyExpansion, randomSeed, keyMaterial);

    // Partitions the material for key generation
    uint8_t *clientWriteMac, *serverWriteMac;
    uint8_t *clientWriteKey, *serverWriteKey;

    clientWriteMac = keyMaterial.data();
    serverWriteMac = clientWriteMac + BCRYPT_SHA384_ALGORITHM_LEN;
    clientWriteKey = serverWriteMac + BCRYPT_SHA384_ALGORITHM_LEN;
    serverWriteKey = clientWriteKey + BCRYPT_AES_ALGORITHM_BYTE_LEN;

    // Keep track of both sides' values
    uint8_t *readMac, *writeMac;
    uint8_t *readKey, *writeKey;

    if (m_isServer)
    {
        writeMac = serverWriteMac;
        readMac = clientWriteMac;
        writeKey = serverWriteKey;
        readKey = clientWriteKey;
    }
    else
    {
        writeMac = clientWriteMac;
        readMac = serverWriteMac;
        writeKey = clientWriteKey;
        readKey = serverWriteKey;
    }

    m_readMacBuffer = std::vector<uint8_t>(readMac, readMac + BCRYPT_SHA384_ALGORITHM_LEN);
    m_writeMacBuffer = std::vector<uint8_t>(writeMac, writeMac + BCRYPT_SHA384_ALGORITHM_LEN);

    // Generate the Read key
    m_readKeyMaterial.resize(m_aesObjLen);

    auto hr = BCryptGenerateSymmetricKey(BCRYPT_AES_CBC_ALG_HANDLE, &m_readKey, m_readKeyMaterial.data(), m_aesObjLen, readKey, BCRYPT_AES_ALGORITHM_BYTE_LEN, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGenerateSymmetricKey");
    }

    // Generate the Write key
    m_writeKeyMaterial.resize(m_aesObjLen);

    hr = BCryptGenerateSymmetricKey(BCRYPT_AES_CBC_ALG_HANDLE, &m_writeKey, m_writeKeyMaterial.data(), m_aesObjLen, writeKey, BCRYPT_AES_ALGORITHM_BYTE_LEN, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGenerateSymmetricKey");
    }
}

void DtlsConnection::ShutdownConnection()
{
    DEBUGLOG("DtlsConnection::ShutdownConnection");

    if (m_connectionState == ConnectionState::Established)
    {
        auto alertMsg = std::make_shared<DtlsAlertMessage>(DtlsAlertLevel::Warning, DtlsAlertDescription::CloseNotify, m_epoch, m_writeSequence++);
        alertMsg->Finalize();

        auto data = alertMsg->Payload();
        SendPayload(data);
    }
}

#pragma region Validation

void DtlsConnection::ValidateCertificate(PCCERT_CONTEXT certContext)
{
    DEBUGLOG("DtlsConnection::ValidateCertificate:");

    std::string expectedFingerprintString, expectedSubjectNameString;
    ATG::SplitString(m_expectedIdentityString, ":", expectedFingerprintString, expectedSubjectNameString);

    // Check the certificate's validity period against the current system time
    LONG comp = CertVerifyTimeValidity(nullptr, certContext->pCertInfo);
    if (comp != 0)
    {
        throw DtlsException(comp < 0 ? HRESULT_FROM_WIN32(ERROR_TIME_SKEW) : CERT_E_EXPIRED, "Certificate time invalid");
    }

    // Get the CN from the cert
    auto nameLen = CertGetNameStringA(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
    if (nameLen == 1)
    {
        throw DtlsException(HRESULT_FROM_WIN32(GetLastError()), "CertGetNameStringA");
    }

    auto certName = std::string((size_t)nameLen, 0);
    nameLen = CertGetNameStringA(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, certName.data(), nameLen);
    if (nameLen == 1)
    {
        throw DtlsException(HRESULT_FROM_WIN32(GetLastError()), "CertGetNameStringA");
    }

    // Verify the name matches our expected subject name
    auto certSubjectName = ATG::BytesToHexString((uint8_t*)certName.data(), certName.size() - 1);
    if (certSubjectName != expectedSubjectNameString)
    {
        throw DtlsException(TRUST_E_SUBJECT_NOT_TRUSTED, "Subject name mismatch");
    }

    // Compare cert hashes
    DWORD computedHashSize = DtlsSocket::c_maxCertificateFingerprintSize;
    BYTE computedHashBuffer[DtlsSocket::c_maxCertificateFingerprintSize];
    auto result = CryptHashCertificate2(BCRYPT_SHA256_ALGORITHM, 0, nullptr, certContext->pbCertEncoded, certContext->cbCertEncoded, computedHashBuffer, &computedHashSize);
    if (!result)
    {
        throw DtlsException(HRESULT_FROM_WIN32(GetLastError()), "CryptHashCertificate2");
    }

    auto hashString = ATG::BytesToHexString(computedHashBuffer, computedHashSize);
    if (hashString != expectedFingerprintString)
    {
        throw DtlsException(TRUST_E_SUBJECT_NOT_TRUSTED, "Certificte hash mismatch");
    }
}

#pragma endregion

#pragma region Client Handshake

void DtlsConnection::StartClientHandshake()
{
    DEBUGLOG("DtlsConnection::StartClientHandshake:");

    auto hr = m_timeoutOp.Reset(c_handshakeTimeoutMS);
    DEBUGLOG("DtlsConnection::StartClientHandshake: Timer reset: 0x%08X", hr);

    // Create the handshake hash object
    m_handshakeHash.set_data_size(m_sha384ObjLen);

    hr = BCryptCreateHash(BCRYPT_SHA384_ALG_HANDLE, &m_handshakeHash, m_handshakeHash, m_sha384ObjLen, NULL, 0, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptCreateHash");
    }

    // Send the ClientHello message.
    SendClientHello();
}

void DtlsConnection::SendClientHello()
{
    DEBUGLOG("DtlsConnection::SendClientHello:");

    // Generate the Client Random number
    m_clientRandom.resize(c_RandomLen);

    auto hr = BCryptGenRandom(NULL, m_clientRandom.data(), (ULONG)m_clientRandom.size(), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGenRandom");
    }

    // Create a new DTLS Handshake record
    auto clientHello = std::make_shared<DtlsHandshakeMessage>(m_epoch, m_writeSequence++);

    // Add the ClientHello message
    auto message = std::dynamic_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeClientHello>(m_handshakeSequence++, m_clientRandom));
    message->HashMessage(m_handshakeHash);

    clientHello->AddMessage(message);
    clientHello->Finalize();

    // Send the data
    auto data = clientHello->Payload();
    SendPayload(data);
}

void DtlsConnection::ParseServerHello(std::shared_ptr<DtlsHandshakeMessage> tlsRecord)
{
    DEBUGLOG("DtlsConnection::ParseServerHello:");

    auto messages = tlsRecord->Messages();

    if (messages.size() != 4)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    // First message should be the ServerHello
    auto serverHello = std::dynamic_pointer_cast<DtlsHandshakeServerHello>(messages[0]);
    if (serverHello == nullptr)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    serverHello->HashMessage(m_handshakeHash);

    m_serverRandom = serverHello->ServerRandom();
    m_sessionId = serverHello->SessionId();

    // Next message is the server Certificate
    auto serverCert = std::dynamic_pointer_cast<DtlsHandshakeCertificate>(messages[1]);
    if (serverCert == nullptr)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    serverCert->HashMessage(m_handshakeHash);

    m_serverCert = CertDuplicateCertificateContext(serverCert->CertificateHandle());
    if (m_serverCert == NULL)
    {
        throw DtlsException(HRESULT_FROM_WIN32(GetLastError()), "CertDuplicateCertificateContext");
    }

    // Verify certificate
    ValidateCertificate(m_serverCert);

    // Next message is the ServerKeyExchange
    auto serverKeyExch = std::dynamic_pointer_cast<DtlsHandshakeServerKeyExchange>(messages[2]);
    if (serverKeyExch == nullptr)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    serverKeyExch->HashMessage(m_handshakeHash);

    // Import the server's public key
    auto serverKeyBlob = serverKeyExch->ServerKey();
    auto fullKeyBlob = std::vector<uint8_t>(sizeof(BCRYPT_ECCKEY_BLOB));

    fullKeyBlob.insert(std::end(fullKeyBlob), std::begin(serverKeyBlob), std::end(serverKeyBlob));

    // Need to pre-pend a header struct and fill it in for the API
    auto keyHeader = (BCRYPT_ECCKEY_BLOB*)fullKeyBlob.data();

    keyHeader->cbKey = c_EccKeyLen / 8;
    keyHeader->dwMagic = BCRYPT_ECDH_PUBLIC_P384_MAGIC;

    auto hr = BCryptImportKeyPair(BCRYPT_ECDH_P384_ALG_HANDLE, NULL, BCRYPT_ECCPUBLIC_BLOB, &m_serverKey, fullKeyBlob.data(), (ULONG)fullKeyBlob.size(), 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptImportKeyPair");
    }

    // Use the certificate's public key to verify the server hash
    auto publicKey = BCryptKeyHandle{};
    auto result = CryptImportPublicKeyInfoEx2(X509_ASN_ENCODING, &m_serverCert->pCertInfo->SubjectPublicKeyInfo, 0, NULL, &publicKey);
    if (!result)
    {
        throw DtlsException(HRESULT_FROM_WIN32(GetLastError()), "CryptImportPublicKeyInfoEx2");
    }

    auto serverSig = serverKeyExch->Signature();
    auto excgHash = ComputeServerExchangeHashes();
    auto padInfo = BCRYPT_PKCS1_PADDING_INFO{};

    // Verify signature
    hr = BCryptVerifySignature(publicKey, &padInfo, excgHash.data(), (DWORD)excgHash.size(), serverSig.data(), (DWORD)serverSig.size(), BCRYPT_PAD_PKCS1);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptVerifySignature");
    }

    // Last message is the ServerHelloDone
    auto serverDone = std::dynamic_pointer_cast<DtlsHandshakeServerHelloDone>(messages[3]);
    if (serverDone == nullptr)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    serverDone->HashMessage(m_handshakeHash);
}

void DtlsConnection::SendClientKeyExchange()
{
    DEBUGLOG("DtlsConnection::SendClientKeyExchange:");

    // Create the client's private key
    m_clientKey = CreateEphemeralKey();

    // Extract the key info less the header
    auto keyLen = (c_EccKeyLen / 8) * 2;
    auto exportedKey = std::vector<uint8_t>(m_clientKey.data().data() + sizeof(BCRYPT_ECCKEY_BLOB), m_clientKey.data().data() + sizeof(BCRYPT_ECCKEY_BLOB) + keyLen);

    // Create a new TLS Handshake record
    auto clientKeyExch = std::make_shared<DtlsHandshakeMessage>(m_epoch, m_writeSequence++);

    // Add the ServerKeyExchange message
    auto message = std::dynamic_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeClientKeyExchange>(m_handshakeSequence++, exportedKey));
    message->HashMessage(m_handshakeHash);

    clientKeyExch->AddMessage(message);

    // Also send our certificate for validation
    message = std::dynamic_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeCertificate>(m_handshakeSequence++, m_clientCert));
    message->HashMessage(m_handshakeHash);

    clientKeyExch->AddMessage(message);
    clientKeyExch->Finalize();

    // Send the record to the server
    auto data = clientKeyExch->Payload();
    SendPayload(data);

    // Generate the master key
    GenerateMasterKey();

    // Generate the session keys
    GenerateSessionKeys();

    // Create a new TLS record for the ChangeCipherSpec message
    // This is not a handshake message and therefore not added to the hash
    auto changeCipher = std::make_shared<DtlsChangeCipherSpecMessage>(m_epoch, m_writeSequence++);
    changeCipher->Finalize();

    // Send the record
    data = changeCipher->Payload();
    SendPayload(data);

    // Now that we've sent ChangeCipherSpec, we will send encrypted packets
    m_encryptSend = true;
    DEBUGLOG("Encryption enabled!");

    // Calculate the verify data
    auto verifyData = ComputeFinishedHash(c_ClientFinished);

    // Create the final handshake message
    auto finishedMsg = std::make_shared<DtlsHandshakeMessage>(m_epoch, m_writeSequence++);

    message = std::dynamic_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeFinished>(m_handshakeSequence++, verifyData));
    message->Finalize();

    finishedMsg->AddMessage(message);
    finishedMsg->Finalize();

    auto tlsRecord = finishedMsg->Payload();
    SendPayload(tlsRecord);
}

void DtlsConnection::ParseServerFinished(std::shared_ptr<DtlsHandshakeMessage> tlsRecord)
{
    DEBUGLOG("DtlsConnection::ParseServerFinished:");

    auto messages = tlsRecord->Messages();

    if (messages.size() != 1)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    // The message should be a Finished message
    auto finishedMsg = std::dynamic_pointer_cast<DtlsHandshakeFinished>(messages[0]);
    if (finishedMsg == nullptr)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    // Verify finished message hash value.
    auto handshakeBuf = ComputeFinishedHash(c_ServerFinished);
    if (memcmp(finishedMsg->VerifyData().data(), handshakeBuf.data(), c_FinishHashLen) != 0)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }
}

#pragma endregion

#pragma region Server Handshake

void DtlsConnection::StartServerHandshake(const SocketPayload& clientHello)
{
    DEBUGLOG("DtlsConnection::StartServerHandshake: Starting server handshake");

    SocketPayload data{};

    HRESULT hr = m_timeoutOp.Reset(c_handshakeTimeoutMS);

    DEBUGLOG("DtlsConnection::StartServerHandshake: Timer reset: 0x%08X", hr);

    auto result = ProcessData(clientHello, data);

    Assert(result == ProcessResult::Handshaking);
}

void DtlsConnection::ParseClientHello(std::shared_ptr<DtlsHandshakeMessage> tlsRecord)
{
    DEBUGLOG("DtlsConnection::ParseClientHello:");

    // Create the handshake hash object
    m_handshakeHash.set_data_size(m_sha384ObjLen);

    auto hr = BCryptCreateHash(BCRYPT_SHA384_ALG_HANDLE, &m_handshakeHash, m_handshakeHash, m_sha384ObjLen, NULL, 0, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptCreateHash");
    }

    // Get the list of handshake messages
    auto messages = tlsRecord->Messages();

    if (messages.size() != 1)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    // Message should be ClientHello
    auto clientHello = std::dynamic_pointer_cast<DtlsHandshakeClientHello>(messages[0]);
    if (clientHello == nullptr)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    m_clientRandom = clientHello->ClientRandom();
    m_sessionId = clientHello->SessionId();

    clientHello->HashMessage(m_handshakeHash);
}

std::vector<uint8_t> DtlsConnection::ComputeServerExchangeHashes()
{
    DEBUGLOG("DtlsConnection::ComputeServerExchangeHashes:");

    // Hash the client+server random values with MD5 and SHA1
    auto md5Hash = BCryptHashHandle{};
    md5Hash.set_data_size(m_md5ObjLen);

    auto hr = BCryptCreateHash(BCRYPT_MD5_ALG_HANDLE, &md5Hash, md5Hash, m_md5ObjLen, NULL, 0, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptCreateHash");
    }

    hr = BCryptHashData(md5Hash, m_clientRandom.data(), c_RandomLen, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptHashData");
    }

    hr = BCryptHashData(md5Hash, m_serverRandom.data(), c_RandomLen, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptHashData");
    }

    auto md5Value = std::vector<uint8_t>(c_MD5HashLen);
    hr = BCryptFinishHash(md5Hash, md5Value.data(), c_MD5HashLen, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptFinishHash");
    }

    auto sha1Hash = BCryptHashHandle{};
    sha1Hash.set_data_size(m_sha1ObjLen);

    hr = BCryptCreateHash(BCRYPT_SHA1_ALG_HANDLE, &sha1Hash, sha1Hash, m_sha1ObjLen, NULL, 0, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptCreateHash");
    }

    hr = BCryptHashData(sha1Hash, m_clientRandom.data(), c_RandomLen, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptHashData");
    }

    hr = BCryptHashData(sha1Hash, m_serverRandom.data(), c_RandomLen, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptHashData");
    }

    auto sha1Value = std::vector<uint8_t>(c_SHA1HashLen);
    hr = BCryptFinishHash(sha1Hash, sha1Value.data(), c_SHA1HashLen, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptFinishHash");
    }

    auto finalHash = std::vector<uint8_t>();
    finalHash.reserve(c_MD5HashLen + c_SHA1HashLen);
    finalHash.insert(std::end(finalHash), std::begin(md5Value), std::end(md5Value));
    finalHash.insert(std::end(finalHash), std::begin(sha1Value), std::end(sha1Value));

    return finalHash;
}

void DtlsConnection::SendServerHello()
{
    DEBUGLOG("DtlsConnection::SendServerHello:");

    // Generate our server random number
    m_serverRandom.resize(c_RandomLen);

    auto hr = BCryptGenRandom(NULL, m_serverRandom.data(), (ULONG)m_serverRandom.size(), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGenRandom");
    }

    // Generate a session id
    m_sessionId.resize(c_RandomLen);

    hr = BCryptGenRandom(NULL, m_sessionId.data(), (ULONG)m_sessionId.size(), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGenRandom");
    }

    // Create the server's private key
    m_serverKey = CreateEphemeralKey();

    // Extract the key info less the header
    auto keyLen = (c_EccKeyLen / 8) * 2;
    auto exportedKey = std::vector<uint8_t>(m_serverKey.data().data() + sizeof(BCRYPT_ECCKEY_BLOB), m_serverKey.data().data() + sizeof(BCRYPT_ECCKEY_BLOB) + keyLen);

    // Create hashes of client+server random numbers
    auto exchangeHash = ComputeServerExchangeHashes();

    // Sign the hashes with our private key
    DWORD sigLen{};
    BCRYPT_PKCS1_PADDING_INFO padInfo{};
    hr = NCryptSignHash(m_serverCertKey, &padInfo, exchangeHash.data(), (DWORD)exchangeHash.size(), nullptr, 0, &sigLen, NCRYPT_PAD_PKCS1_FLAG);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "NCryptSignHash");
    }

    auto signature = std::vector<uint8_t>(sigLen);
    hr = NCryptSignHash(m_serverCertKey, &padInfo, exchangeHash.data(), (DWORD)exchangeHash.size(), signature.data(), sigLen, &sigLen, NCRYPT_PAD_PKCS1_FLAG);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "NCryptSignHash");
    }

    // Create a new TLS Handshake record
    auto serverHello = std::make_shared<DtlsHandshakeMessage>(m_epoch, (uint16_t)m_writeSequence++);

    // Add the ServerHello message
    auto message = std::dynamic_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeServerHello>(m_handshakeSequence++, m_serverRandom, m_sessionId));
    message->HashMessage(m_handshakeHash);

    serverHello->AddMessage(message);

    // Add the server Certificate message
    message = std::dynamic_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeCertificate>(m_handshakeSequence++, m_serverCert));
    message->HashMessage(m_handshakeHash);

    serverHello->AddMessage(message);

    // Add the ServerKeyExchange message
    message = std::dynamic_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeServerKeyExchange>(m_handshakeSequence++, exportedKey, signature));
    message->HashMessage(m_handshakeHash);

    serverHello->AddMessage(message);

    // Add the ServerHelloDone message
    message = std::dynamic_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeServerHelloDone>(m_handshakeSequence++));
    message->HashMessage(m_handshakeHash);

    serverHello->AddMessage(message);
    serverHello->Finalize();

    // Send the data
    auto data = serverHello->Payload();
    SendPayload(data);
}

void DtlsConnection::ParseClientKeyExchange(std::shared_ptr<DtlsHandshakeMessage> tlsRecord)
{
    DEBUGLOG("DtlsConnection::ParseClientKeyExchange:");

    auto messages = tlsRecord->Messages();

    if (messages.size() != 2)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    // First message is the ClientKeyExchange
    auto clientKeyExch = std::dynamic_pointer_cast<DtlsHandshakeClientKeyExchange>(messages[0]);
    if (clientKeyExch == nullptr)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    clientKeyExch->HashMessage(m_handshakeHash);

    // Import the client's public key
    auto clientKeyBlob = clientKeyExch->ClientKey();
    auto fullKeyBlob = std::vector<uint8_t>(sizeof(BCRYPT_ECCKEY_BLOB));

    fullKeyBlob.insert(std::end(fullKeyBlob), std::begin(clientKeyBlob), std::end(clientKeyBlob));

    auto keyHeader = (BCRYPT_ECCKEY_BLOB*)fullKeyBlob.data();

    keyHeader->cbKey = c_EccKeyLen / 8;
    keyHeader->dwMagic = BCRYPT_ECDH_PUBLIC_P384_MAGIC;

    auto hr = BCryptImportKeyPair(BCRYPT_ECDH_P384_ALG_HANDLE, NULL, BCRYPT_ECCPUBLIC_BLOB, &m_clientKey, fullKeyBlob.data(), (ULONG)fullKeyBlob.size(), 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptImportKeyPair");
    }

    // Next message should be the client certificate
    auto clientCert = std::dynamic_pointer_cast<DtlsHandshakeCertificate>(messages[1]);
    if (clientCert == nullptr)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    clientCert->HashMessage(m_handshakeHash);

    m_clientCert = CertDuplicateCertificateContext(clientCert->CertificateHandle());
    if (m_clientCert == NULL)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    // Verify certificate
    ValidateCertificate(m_clientCert);

    // Generate the master key
    GenerateMasterKey();

    // Generate the session keys
    GenerateSessionKeys();
}

void DtlsConnection::ParseClientFinished(std::shared_ptr<DtlsHandshakeMessage> tlsRecord)
{
    // Parse the Finished message.
    auto messages = tlsRecord->Messages();

    if (messages.size() != 1)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    // The message should be a Finished message
    auto finishedMsg = std::dynamic_pointer_cast<DtlsHandshakeFinished>(messages[0]);
    if (finishedMsg == nullptr)
    {
        throw DtlsException(NTE_BAD_DATA, "Invalid packet data");
    }

    // Verify finished message hash value.
    auto handshakeBuf = ComputeFinishedHash(c_ClientFinished);
    if (memcmp(finishedMsg->VerifyData().data(), handshakeBuf.data(), c_FinishHashLen) != 0)
    {
        throw DtlsException(E_FAIL, "Invalid handshake hash");
    }
}

void DtlsConnection::SendServerFinished()
{
    DEBUGLOG("DtlsConnection::SendServerFinished:");

    // Create a new TLS record for the ChangeCipherSpec message
    // This is not a handshake message and therefore not added to the hash
    auto changeCipher = std::make_shared<DtlsChangeCipherSpecMessage>(m_epoch, m_writeSequence++);
    changeCipher->Finalize();

    // Send the record
    auto data = changeCipher->Payload();
    SendPayload(data);

    // Now that we've sent ChangeCipherSpec, we send/recv encrypted packets
    m_encryptSend = true;
    DEBUGLOG("Encryption enabled!");

    // Calculate the verify data
    auto verifyData = ComputeFinishedHash(c_ServerFinished);

    // Create the final handshake message
    auto message = std::dynamic_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeFinished>(m_handshakeSequence++, verifyData));
    message->Finalize();

    auto finishedMsg = std::make_shared<DtlsHandshakeMessage>(m_epoch, m_writeSequence++);
    finishedMsg->AddMessage(message);
    finishedMsg->Finalize();

    data = finishedMsg->Payload();
    SendPayload(data);
}

void DtlsConnection::QueryBCryptObjectSizes(void)
{
    DWORD resultSize{};
    HRESULT hr{};

    hr = BCryptGetProperty(BCRYPT_HMAC_SHA384_ALG_HANDLE, BCRYPT_OBJECT_LENGTH, (PBYTE)&m_hmacSha384ObjLen, sizeof(DWORD), &resultSize, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGetProperty");
    }

    hr = BCryptGetProperty(BCRYPT_HMAC_SHA384_ALG_HANDLE, BCRYPT_HASH_LENGTH, (PBYTE)&m_hmacSha384HashLen, sizeof(DWORD), &resultSize, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGetProperty");
    }

    hr = BCryptGetProperty(BCRYPT_AES_CBC_ALG_HANDLE, BCRYPT_BLOCK_LENGTH, (PBYTE)&m_aesBlockLen, sizeof(DWORD), &resultSize, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGetProperty");
    }

    hr = BCryptGetProperty(BCRYPT_AES_CBC_ALG_HANDLE, BCRYPT_OBJECT_LENGTH, (PBYTE)&m_aesObjLen, sizeof(DWORD), &resultSize, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGetProperty");
    }

    hr = BCryptGetProperty(BCRYPT_MD5_ALG_HANDLE, BCRYPT_OBJECT_LENGTH, (PBYTE)&m_md5ObjLen, sizeof(DWORD), &resultSize, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGetProperty");
    }

    hr = BCryptGetProperty(BCRYPT_SHA1_ALG_HANDLE, BCRYPT_OBJECT_LENGTH, (PBYTE)&m_sha1ObjLen, sizeof(DWORD), &resultSize, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGetProperty");
    }

    hr = BCryptGetProperty(BCRYPT_SHA384_ALG_HANDLE, BCRYPT_OBJECT_LENGTH, (PBYTE)&m_sha384ObjLen, sizeof(DWORD), &resultSize, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGetProperty");
    }

    DEBUGLOG("BCrypt sizes:\n\tm_hmacSha384ObjLen: %d\n\tm_hmacSha384HashLen: %d\n\tm_aesBlockLen: %d\n\tm_aesObjLen: %d\n\tm_md5ObjLen: %d\n\tm_sha1ObjLen: %d\n\tm_sha384ObjLen: %d", m_hmacSha384ObjLen, m_hmacSha384HashLen, m_aesBlockLen, m_aesObjLen, m_md5ObjLen, m_sha1ObjLen, m_sha384ObjLen);
}

BCryptKeyHandle DtlsConnection::CreateEphemeralKey()
{
    BCryptKeyHandle key{};

    // Create a new key pair
    auto hr = BCryptGenerateKeyPair(BCRYPT_ECDH_P384_ALG_HANDLE, &key, 0, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGenerateKeyPair");
    }

    // Finalize the key
    hr = BCryptFinalizeKeyPair(key, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptFinalizeKeyPair");
    }

    // Get the key blob size
    DWORD blobLen{};
    hr = BCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, 0, &blobLen, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptExportKey");
    }

    key.set_data_size(blobLen);

    // Export the key blob
    hr = BCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, key, blobLen, &blobLen, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptExportKey");
    }

    return key;
}

void DtlsConnection::GenerateKeyMaterial(std::vector<uint8_t>& secret, const std::string_view label, std::vector<uint8_t>& seed, std::vector<uint8_t>& key)
{
    auto keyHandle = BCryptKeyHandle{};
    auto hr = BCryptGenerateSymmetricKey(BCRYPT_TLS1_2_KDF_ALG_HANDLE, &keyHandle, NULL, 0, secret.data(), (ULONG)secret.size(), 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptGenerateSymmetricKey");
    }

    BCryptBufferDesc parameterList{};
    BCryptBuffer parameters[3]{};

    // Setup extra parameters for BCrypt
    parameterList.ulVersion = BCRYPTBUFFER_VERSION;
    parameterList.cBuffers = 3;
    parameterList.pBuffers = parameters;

    // Tell BCrypt we want the SHA384 KDF alorithm
    parameters[0].BufferType = KDF_HASH_ALGORITHM;
    parameters[0].cbBuffer = (ULONG)((wcslen(BCRYPT_SHA384_ALGORITHM) + 1) * sizeof(WCHAR));
    parameters[0].pvBuffer = (PBYTE)BCRYPT_SHA384_ALGORITHM;

    // The pseudo-random function label
    parameters[1].BufferType = KDF_TLS_PRF_LABEL;
    parameters[1].cbBuffer = (ULONG)label.size();
    parameters[1].pvBuffer = (PVOID)label.data();

    // Specify the seed value to use
    parameters[2].BufferType = KDF_TLS_PRF_SEED;
    parameters[2].cbBuffer = (ULONG)seed.size();
    parameters[2].pvBuffer = seed.data();

    DWORD cbResult{};
    hr = BCryptKeyDerivation(keyHandle, &parameterList, key.data(), (ULONG)key.size(), &cbResult, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptKeyDerivation");
    }
}

std::vector<uint8_t> DtlsConnection::ComputeFinishedHash(std::string_view label)
{
    // Duplicate the handshake hash object
    BCryptHashHandle hash = m_handshakeHash;
    std::vector<uint8_t> finishHash(BCRYPT_SHA384_ALGORITHM_LEN);

    // Finish the hash
    auto hr = BCryptFinishHash(hash, finishHash.data(), BCRYPT_SHA384_ALGORITHM_LEN, 0);
    if (FAILED(hr))
    {
        throw DtlsException(hr, "BCryptFinishHash");
    }

    std::vector<uint8_t> data(c_FinishHashLen);

    // Use the PRF to generate material based on the Master key
    GenerateKeyMaterial(m_masterKeyMaterial, label, finishHash, data);

    return data;
}

#pragma endregion
