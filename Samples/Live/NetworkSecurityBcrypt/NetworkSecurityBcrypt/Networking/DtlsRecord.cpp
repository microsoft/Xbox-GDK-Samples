#include "pch.h"
#include "SocketPayload.h"
#include "DtlsRecord.h"

#include <WinSock2.h>
#include <WS2tcpip.h>

using namespace ATG;

void DtlsMessage::HashMessage(BCryptHashHandle& hash)
{
    // Add the message data to a hash
    Finalize();

    auto result = BCryptHashData(hash, (PUCHAR)m_payload.payload.data(), (ULONG)m_payload.payload.size(), 0);
    if (result != ERROR_SUCCESS)
    {
        throw std::exception("Failed to hash message");
    }
}

DtlsRecord::DtlsRecord(SocketPayload& packet)
{
    m_recordHeader = DtlsRecordHeader(packet);
}


///////////////////////////////////////////////////////////////////////////
//
DtlsAlertMessage::DtlsAlertMessage(SocketPayload& packet) :
    DtlsRecord(packet)
{
    // Validate message length, which is always two for an Alert
    if (Length() != 2)
    {
        throw std::exception("Invalid alert message");
    }

    packet.Read(m_alertLevel);
    packet.Read(m_alertDescription);
}

void DtlsAlertMessage::Finalize(void)
{
    m_recordHeader.SetLength(2);

    m_payload = SocketPayload{};
    m_payload.Write(m_recordHeader.Header);
    m_payload.Write(m_alertLevel);
    m_payload.Write(m_alertDescription);
}

///////////////////////////////////////////////////////////////////////////
//
DtlsChangeCipherSpecMessage::DtlsChangeCipherSpecMessage(SocketPayload& packet) :
    DtlsRecord(packet)
{
    if (Length() != 1)
    {
        throw std::exception("Invalid ChangeCipherSpec message");
    }

    // The protocol must be '1'
    m_protocol = packet.Read<DtlsChangeCipherSpecProtocol>();
    if (m_protocol != DtlsChangeCipherSpecProtocol::ProtocolOne)
    {
        throw std::exception("Invalid ChangeCipherSpec message");
    }
}

void DtlsChangeCipherSpecMessage::Finalize(void)
{
    m_recordHeader.SetLength(1);

    m_payload = SocketPayload{};
    m_payload.Write(m_recordHeader.Header);
    m_payload.Write<DtlsChangeCipherSpecProtocol>(DtlsChangeCipherSpecProtocol::ProtocolOne);
}

///////////////////////////////////////////////////////////////////////////
//
DtlsApplicationMessage::DtlsApplicationMessage(const std::vector<uint8_t>& data, uint16_t epoch, uint64_t sequence) :
    DtlsRecord(DtlsContentType::Application, epoch, sequence, (uint16_t)data.size())
{
    m_messageData = data;
}

DtlsApplicationMessage::DtlsApplicationMessage(const uint8_t* data, const size_t length, uint16_t epoch, uint64_t sequence) :
    DtlsRecord(DtlsContentType::Application, epoch, sequence, (uint16_t)length)
{
    m_messageData = std::vector<uint8_t>(data, data + length);
}

DtlsApplicationMessage::DtlsApplicationMessage(SocketPayload& packet) :
    DtlsRecord(packet)
{
    m_messageData.resize(Length());
    packet.Read(m_messageData);
}

void DtlsApplicationMessage::Finalize(void)
{
    m_recordHeader.SetLength((uint16_t)m_messageData.size());

    m_payload = SocketPayload{};
    m_payload.Write(m_recordHeader.Header);
    m_payload.Write(m_messageData);
}


///////////////////////////////////////////////////////////////////////////
//
DtlsHandshakeMessage::DtlsHandshakeMessage(SocketPayload& packet) :
    DtlsRecord(packet)
{
    if (packet.Size() < Length())
    {
        throw std::exception("Invalid packet size");
    }

    while (packet.GetRemainingReadSize() > 0)
    {
        auto messageType = packet.Read<DtlsHandshakeMessageType>();

        packet.SetReadLocation(packet.GetReadLocation() - 1);

        switch (messageType)
        {
        case DtlsHandshakeMessageType::Certificate:
            AddMessage(std::static_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeCertificate>(packet)));
            break;

        case DtlsHandshakeMessageType::ClientHello:
            AddMessage(std::static_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeClientHello>(packet)));
            break;

        case DtlsHandshakeMessageType::ClientKeyExchange:
            AddMessage(std::static_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeClientKeyExchange>(packet)));
            break;

        case DtlsHandshakeMessageType::Finished:
            AddMessage(std::static_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeFinished>(packet)));
            break;

        case DtlsHandshakeMessageType::ServerHello:
            AddMessage(std::static_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeServerHello>(packet)));
            break;

        case DtlsHandshakeMessageType::ServerHelloDone:
            AddMessage(std::static_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeServerHelloDone>(packet)));
            break;

        case DtlsHandshakeMessageType::ServerKeyExchange:
            AddMessage(std::static_pointer_cast<DtlsMessage>(std::make_shared<DtlsHandshakeServerKeyExchange>(packet)));
            break;

        case DtlsHandshakeMessageType::CertificateRequest:
        case DtlsHandshakeMessageType::CertificateVerify:
        case DtlsHandshakeMessageType::EncryptedExtensions:
        case DtlsHandshakeMessageType::HelloRequest:
        case DtlsHandshakeMessageType::NewSessionTicket:
        case DtlsHandshakeMessageType::Unknown:
        default:
            assert(false);
            throw std::exception("Unknown message type");
            break;
        }
    }
}

void DtlsHandshakeMessage::Finalize(void)
{
    size_t totalLen = 0;

    // Get the full size of the record
    for (const auto& message : m_handshakeMessages)
    {
        totalLen += message->Payload().Size();
    }

    m_recordHeader.SetLength((uint16_t)totalLen);

    m_payload = SocketPayload{};
    m_payload.Write(m_recordHeader.Header);

    // Append all handshake messages
    for (const auto& message : m_handshakeMessages)
    {
        m_payload.Write(message->Payload().payload);
    }
}

///////////////////////////////////////////////////////////////////////////
//
DtlsHandshakeClientHello::DtlsHandshakeClientHello(SocketPayload& packet) :
    DtlsHandshake(packet)
{
    // Read the 32 bytes of client random values
    m_clientRandom.resize(c_RandomLen);
    packet.Read(m_clientRandom);

    // Check for a session id
    auto sessionIdLen = packet.Read<uint8_t>();
    if (sessionIdLen != 0)
    {
        m_sessionId.resize(sessionIdLen);
        packet.Read(m_sessionId);
    }

    // There should be only one cipher entry (two bytes)
    auto cipherListLen = ntohs(packet.Read<uint16_t>());
    if (cipherListLen != 2)
    {
        throw std::exception("Unexpected cipher list length");
    }

    // Verify the cipher suite
    auto cipherId = ntohs(packet.Read<uint16_t>());
    if (cipherId != c_CipherSuite)
    {
        throw std::exception("Unexpected cipher suite");
    }

    // There should be one compression type
    auto compressionListLen = packet.Read<uint8_t>();
    if (compressionListLen != 1)
    {
        throw std::exception("Unexpected compression list length");
    }

    // The type should be 0 for 'no compression'
    auto compressionType = packet.Read<uint8_t>();
    if (compressionType != 0)
    {
        throw std::exception("Unexpected compression type");
    }
}

void DtlsHandshakeClientHello::Finalize(void)
{
    m_header.SetLength(39);

    m_payload = SocketPayload{};
    m_payload.Write(m_header.Header);
    m_payload.Append(m_clientRandom);                       // Random bytes
    m_payload.Write<uint8_t>(0);                            // Session ID length
    m_payload.Write<uint8_t>(0);                            // Cipher suite list length (MSD)
    m_payload.Write<uint8_t>(2);                            // Cipher suite list length (LSD)
    m_payload.Write<uint16_t>(htons(c_CipherSuite));        // Selected cipher suite
    m_payload.Write<uint8_t>(1);                            // Compression suite list length
    m_payload.Write<uint8_t>(0);                            // No compression
}

///////////////////////////////////////////////////////////////////////////
//
DtlsHandshakeCertificate::DtlsHandshakeCertificate(SocketPayload& packet) :
    DtlsHandshake(packet)
{
    m_certificateBytes.resize(Length());

    packet.Read(m_certificateBytes);

    m_certificate = CertCreateCertificateContext(X509_ASN_ENCODING, m_certificateBytes.data(), (DWORD)m_certificateBytes.size());
}

void DtlsHandshakeCertificate::Finalize(void)
{
    m_header.SetLength(m_certificate->cbCertEncoded);

    m_payload = SocketPayload{};
    m_payload.Write(m_header.Header);
    m_payload.Fill(m_certificate->pbCertEncoded, m_certificate->cbCertEncoded);
}

///////////////////////////////////////////////////////////////////////////
//
DtlsHandshakeServerHello::DtlsHandshakeServerHello(SocketPayload& packet) :
    DtlsHandshake(packet)
{
    // Read the server random bytes
    m_serverRandom.resize(c_RandomLen);
    packet.Read(m_serverRandom);

    // Get the session id length
    auto sessionIdLen = packet.Read<uint8_t>();
    if (sessionIdLen > 0)
    {
        m_sessionId.resize(sessionIdLen);
        packet.Read(m_sessionId);
    }

    auto cipherSuite = ntohs(packet.Read<uint16_t>());
    if (cipherSuite != c_CipherSuite)
    {
        throw std::exception("Invalid cipher suite");
    }

    packet.IncrementReadLocation(1);
}

void DtlsHandshakeServerHello::Finalize(void)
{
    m_header.SetLength(70);

    m_payload = SocketPayload{};
    m_payload.Write(m_header.Header);
    m_payload.Append(m_serverRandom);
    m_payload.Write<uint8_t>(c_RandomLen);
    m_payload.Append(m_sessionId);
    m_payload.Write<uint16_t>(htons(c_CipherSuite));
    m_payload.Write<uint8_t>(0);
}

///////////////////////////////////////////////////////////////////////////
//
DtlsHandshakeServerKeyExchange::DtlsHandshakeServerKeyExchange(SocketPayload& packet) :
    DtlsHandshake(packet)
{
    // Verify parameter type
    auto paramType = packet.Read<uint8_t>();
    if (paramType != 3)
    {
        throw std::exception("Invalid message");
    }

    // Verify parameter
    auto namedCurve = packet.Read<uint8_t>();
    if (namedCurve != c_NamedCurve)
    {
        throw std::exception("Invalid message");
    }

    // Read the server public key
    size_t keyLen = (c_EccKeyLen / 8) * 2;

    m_serverKey.resize(keyLen);
    packet.Read(m_serverKey);

    // Read the hash signature
    auto sigLen = ntohs(packet.Read<uint16_t>());

    m_signature.resize(sigLen);
    packet.Read(m_signature);
}

void DtlsHandshakeServerKeyExchange::Finalize(void)
{
    m_header.SetLength((uint32_t)(m_serverKey.size() + m_signature.size() + 4));

    m_payload = SocketPayload{};
    m_payload.Write(m_header.Header);
    m_payload.Write<uint8_t>(3);                    // always use a named curve.
    m_payload.Write<uint8_t>(c_NamedCurve);         // named curve.
    m_payload.Write(m_serverKey);
    m_payload.Write<uint16_t>(htons((uint16_t)m_signature.size()));
    m_payload.Write(m_signature);
}

///////////////////////////////////////////////////////////////////////////
//
DtlsHandshakeServerHelloDone::DtlsHandshakeServerHelloDone(SocketPayload& packet) :
    DtlsHandshake(packet)
{
}

void DtlsHandshakeServerHelloDone::Finalize(void)
{
    m_payload = SocketPayload{};
    m_payload.Write(m_header.Header);
}

///////////////////////////////////////////////////////////////////////////
//
DtlsHandshakeClientKeyExchange::DtlsHandshakeClientKeyExchange(SocketPayload& packet) :
    DtlsHandshake(packet)
{
    // Read the client public key
    m_clientKey.resize(Length());
    packet.Read(m_clientKey);
}

void DtlsHandshakeClientKeyExchange::Finalize(void)
{
    m_header.SetLength((uint32_t)m_clientKey.size());

    m_payload = SocketPayload{};
    m_payload.Write(m_header.Header);
    m_payload.Write(m_clientKey);
}

///////////////////////////////////////////////////////////////////////////
//
DtlsHandshakeFinished::DtlsHandshakeFinished(SocketPayload& packet) :
    DtlsHandshake(packet)
{
    // Read the final hash value
    m_verifyData.resize(Length());
    packet.Read(m_verifyData);
}

void DtlsHandshakeFinished::Finalize(void)
{
    m_header.SetLength((uint32_t)m_verifyData.size());

    m_payload = SocketPayload{};
    m_payload.Write(m_header.Header);
    m_payload.Write(m_verifyData);
}
