#pragma once

namespace ATG
{
    // Info taken from https://en.wikipedia.org/wiki/Transport_Layer_Security
    // TLS 1.2  - https://datatracker.ietf.org/doc/html/rfc5246/
    // DTLS 1.2 - https://datatracker.ietf.org/doc/html/rfc6347

#define DTLS1_2_PROTOCOL_VERSION                0xfefd
#define TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384   0xC028
#define TLS_ECC_P384_CURVE_KEY_TYPE             24
#define BCRYPT_AES_ALGORITHM_BYTE_LEN           32
#define BCRYPT_AES_ALGORITHM_BLOCK_LEN          16
#define BCRYPT_SHA384_ALGORITHM_LEN             48

    enum class DtlsContentType : uint8_t
    {
        Unknown = 0,
        ChangeCipherSpec = 0x14,
        Alert = 0x15,
        Handshake = 0x16,
        Application = 0x17,
        Heartbeat = 0x18
    };

    enum class DtlsHandshakeMessageType : uint8_t
    {
        HelloRequest = 0,
        ClientHello = 1,
        ServerHello = 2,
        NewSessionTicket = 4,
        EncryptedExtensions = 8,
        Certificate = 11,
        ServerKeyExchange = 12,
        CertificateRequest = 13,
        ServerHelloDone = 14,
        CertificateVerify = 15,
        ClientKeyExchange = 16,
        Finished = 20,
        Unknown = 255
    };

    enum class DtlsAlertLevel : uint8_t
    {
        Warning = 1,
        Fatal = 2
    };

    enum class DtlsAlertDescription : uint8_t
    {
        CloseNotify = 0,
        UnexpectedMessage = 10,
        BadRecordMAC = 20,
        DecryptionFailed = 21,
        RecordOverflow = 22,
        DecompressionFailure = 30,
        HandshakeFailure = 40,
        NoCertificate = 41,
        BadCertificate = 42,
        UnsupportedCertificate = 43,
        CertificateRevoked = 44,
        CertificateExpired = 45,
        CertificateUnknown = 46,
        IllegalParameter = 47,
        UnknownCA = 48,
        AccessDenied = 49,
        DecodeError = 50,
        DecryptError = 51,
        ExportRestriction = 60,
        ProtocolVersion = 70,
        InsufficientSecurity = 71,
        InternalError = 80,
        InappropriateFallback = 86,
        UserCanceled = 90,
        NoRenegotiation = 100,
        UnsupportedExtension = 110,
        CertificateUnobtainable = 111,
        UnrecognizedName = 112,
        BadCertificateStatusResponse = 113,
        BadCertificateHashValue = 114,
        UnknownPSKIdentity = 115,
        NoApplicationProtocol = 120
    };

    enum class DtlsChangeCipherSpecProtocol : uint8_t
    {
        ProtocolOne = 1
    };

    constexpr uint16_t c_ProtocolVersion = DTLS1_2_PROTOCOL_VERSION;
    constexpr uint16_t c_CipherSuite = TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384;
    constexpr uint8_t  c_NamedCurve = TLS_ECC_P384_CURVE_KEY_TYPE;
    constexpr uint16_t c_EccKeyLen = 384;
    constexpr uint8_t  c_RandomLen = 32;
    constexpr uint8_t  c_SHA1HashLen = 20;
    constexpr uint8_t  c_MD5HashLen = 16;

    ///////////////////////////////////////////////////////////////////////////
    //
    struct DtlsRecordHeader
    {
        DtlsRecordHeader() = default;

        DtlsRecordHeader(uint16_t epoch, uint64_t sequence, DtlsContentType type, uint16_t length)
        {
            Header[0] = (uint8_t)type;
            Header[1] = (uint8_t)((c_ProtocolVersion >> 8) & 0xFF);
            Header[2] = (uint8_t)(c_ProtocolVersion & 0xFF);
            Header[3] = (uint8_t)((epoch >> 8) & 0x00FF);
            Header[4] = (uint8_t)(epoch & 0x00FF);
            Header[5] = (uint8_t)((sequence >> 40) & 0xFF);
            Header[6] = (uint8_t)((sequence >> 32) & 0xFF);
            Header[7] = (uint8_t)((sequence >> 24) & 0xFF);
            Header[8] = (uint8_t)((sequence >> 16) & 0xFF);
            Header[9] = (uint8_t)((sequence >> 8) & 0xFF);
            Header[10] = (uint8_t)(sequence & 0xFF);
            Header[11] = (uint8_t)((length >> 8) & 0xFF);
            Header[12] = (uint8_t)(length & 0xFF);
        }

        DtlsRecordHeader(SocketPayload& data)
        {
            data.Read(Header);
        }

        void SetLength(uint16_t length)
        {
            Header[11] = (uint8_t)((length >> 8) & 0xFF);
            Header[12] = (uint8_t)(length & 0xFF);
        }

        // All values are stored in network byte order so these
        // helpers perform the byte swaps same as htons()/ntohs()
        DtlsContentType Type() { return (DtlsContentType)Header[0]; }
        uint16_t Version() { return (uint16_t)((Header[1] << 8) | Header[2]); }
        uint16_t Epoch() { return (uint16_t)((Header[3] << 8) | Header[4]); }
        uint64_t Sequence() { return (uint64_t)(((uint64_t)Header[5] << 40) | ((uint64_t)Header[6] << 32) | ((uint64_t)Header[7] << 24) | ((uint64_t)Header[8] << 16) | ((uint64_t)Header[9] << 8) | (uint64_t)Header[10]); }
        uint16_t Length() { return (uint16_t)((Header[11] << 8) | Header[12]); }

        std::array<uint8_t, 13> Header{};
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    struct DtlsHandshakeHeader
    {
        DtlsHandshakeHeader(DtlsHandshakeMessageType type, uint32_t length, uint32_t sequence)
        {
            Header[0] = (uint8_t)type;
            Header[1] = (uint8_t)((length >> 16) & 0xFF);
            Header[2] = (uint8_t)((length >> 8) & 0xFF);
            Header[3] = (uint8_t)(length & 0xFF);
            Header[4] = (uint8_t)((sequence >> 8) & 0xFF);
            Header[5] = (uint8_t)(sequence & 0xFF);
        }

        DtlsHandshakeHeader(SocketPayload& data)
        {
            data.Read(Header);
        }

        void SetLength(uint32_t length)
        {
            Header[1] = (uint8_t)((length >> 16) & 0xFF);
            Header[2] = (uint8_t)((length >> 8) & 0xFF);
            Header[3] = (uint8_t)(length & 0xFF);
        }

        // All values are stored in network byte order so these
        // helpers perform the byte swaps same as htons()/ntohs()
        DtlsHandshakeMessageType Type() { return (DtlsHandshakeMessageType)Header[0]; }
        uint32_t Length() { return (uint32_t)((Header[1] << 16) | (Header[2] << 8) | Header[3]); }
        uint16_t Sequence() { return (uint16_t)((Header[4] << 8) | Header[5]); }

        std::array<uint8_t, 6> Header;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsMessage
    {
    public:
        virtual ~DtlsMessage() = default;

        virtual void Finalize(void) = 0;

        SocketPayload& Payload(void) { return m_payload; }

        void HashMessage(BCryptHashHandle& hash);

    protected:
        SocketPayload m_payload{};
    };


    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsRecord : public DtlsMessage
    {
    public:
        DtlsRecord(DtlsContentType type, uint16_t epoch, uint64_t sequence, uint16_t length) :
            m_recordHeader(epoch, sequence, type, length)
        {
        }

        DtlsRecord(SocketPayload& packet);

        DtlsContentType Type() { return m_recordHeader.Type(); }
        uint16_t Length() { return m_recordHeader.Length(); }
        uint16_t Epoch() { return m_recordHeader.Epoch(); }
        uint64_t Sequence() { return m_recordHeader.Sequence(); }

    protected:
        DtlsRecordHeader m_recordHeader{};
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsAlertMessage : public DtlsRecord
    {
    public:
        DtlsAlertMessage(DtlsAlertLevel level, DtlsAlertDescription description, uint16_t epoch, uint64_t sequence) :
            DtlsRecord(DtlsContentType::Alert, epoch, sequence, 0),
            m_alertLevel(level),
            m_alertDescription(description)
        {
        }

        DtlsAlertMessage(SocketPayload& packet);

        DtlsAlertLevel AlertLevel(void) { return m_alertLevel; }
        DtlsAlertDescription AlertDescripiton(void) { return m_alertDescription; }

        void Finalize(void) override;

    private:
        DtlsAlertLevel m_alertLevel;
        DtlsAlertDescription m_alertDescription;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsChangeCipherSpecMessage : public DtlsRecord
    {
    public:
        DtlsChangeCipherSpecMessage(uint16_t epoch, uint64_t sequence) :
            DtlsRecord(DtlsContentType::ChangeCipherSpec, epoch, sequence, 0),
            m_protocol(DtlsChangeCipherSpecProtocol::ProtocolOne)
        {
        }

        DtlsChangeCipherSpecMessage(SocketPayload& packet);

        DtlsChangeCipherSpecProtocol Protocol(void) { return m_protocol; }

        void Finalize(void) override;

    private:
        DtlsChangeCipherSpecProtocol m_protocol;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsApplicationMessage : public DtlsRecord
    {
    public:
        DtlsApplicationMessage(const std::vector<uint8_t>& data, uint16_t epoch, uint64_t sequence);
        DtlsApplicationMessage(const uint8_t* data, const size_t length, uint16_t epoch, uint64_t sequence);

        DtlsApplicationMessage(SocketPayload& packet);

        std::vector<uint8_t>& MessageData(void) { return m_messageData; }

        void Finalize(void) override;

    private:
        std::vector<uint8_t> m_messageData;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsHandshakeMessage : public DtlsRecord
    {
    public:
        DtlsHandshakeMessage(uint16_t epoch, uint64_t sequence) :
            DtlsRecord(DtlsContentType::Handshake, epoch, sequence, 0)
        {
        }

        DtlsHandshakeMessage(SocketPayload& packet);

        void AddMessage(std::shared_ptr<DtlsMessage> message) { m_handshakeMessages.push_back(message); }
        std::vector<std::shared_ptr<DtlsMessage>>& Messages(void) { return m_handshakeMessages; }

        void Finalize(void) override;

    private:
        std::vector<std::shared_ptr<DtlsMessage>> m_handshakeMessages;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsHandshake : public DtlsMessage
    {
    public:
        DtlsHandshake(uint16_t sequence, DtlsHandshakeMessageType type) :
            m_header(type, 0, sequence)
        {
        }

        DtlsHandshake(SocketPayload& data) :
            m_header(data)
        {
        }

        uint16_t Sequence() { return m_header.Sequence(); }
        uint32_t Length() { return m_header.Length(); }
        DtlsHandshakeMessageType Type() { return m_header.Type(); }

    protected:
        DtlsHandshakeHeader m_header;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsHandshakeClientHello : public DtlsHandshake
    {
    public:
        DtlsHandshakeClientHello(uint16_t sequence, const std::vector<uint8_t>& rand) :
            DtlsHandshake(sequence, DtlsHandshakeMessageType::ClientHello),
            m_clientRandom(rand)
        {
        }

        DtlsHandshakeClientHello(SocketPayload& packet);

        std::vector<uint8_t>& ClientRandom(void) { return m_clientRandom; }
        std::vector<uint8_t>& SessionId(void) { return m_sessionId; }

        void Finalize(void) override;

    private:
        std::vector<uint8_t> m_clientRandom;
        std::vector<uint8_t> m_sessionId;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsHandshakeCertificate : public DtlsHandshake
    {
    public:
        DtlsHandshakeCertificate(uint16_t sequence, PCCERT_CONTEXT cert) :
            DtlsHandshake(sequence, DtlsHandshakeMessageType::Certificate),
            m_certificate(cert)
        {
        }

        DtlsHandshakeCertificate(SocketPayload& packet);

        ~DtlsHandshakeCertificate()
        {
            if (m_certificate)
            {
                CertFreeCertificateContext(m_certificate);
            }
        }

        void Finalize(void) override;

        std::vector<uint8_t>& Certificate(void) { return m_certificateBytes; }
        PCCERT_CONTEXT CertificateHandle(void) { return m_certificate; }

    private:
        PCCERT_CONTEXT m_certificate{};
        std::vector<uint8_t> m_certificateBytes;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsHandshakeServerHello : public DtlsHandshake
    {
    public:
        DtlsHandshakeServerHello(uint16_t sequence, const std::vector<uint8_t>& rand, const std::vector<uint8_t>& session) :
            DtlsHandshake(sequence, DtlsHandshakeMessageType::ServerHello),
            m_serverRandom(rand),
            m_sessionId(session)
        {
        }

        DtlsHandshakeServerHello(SocketPayload& packet);

        std::vector<uint8_t>& ServerRandom(void) { return m_serverRandom; }
        std::vector<uint8_t>& SessionId(void) { return m_sessionId; }

        void Finalize(void) override;

    private:
        std::vector<uint8_t> m_serverRandom;
        std::vector<uint8_t> m_sessionId;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsHandshakeServerKeyExchange : public DtlsHandshake
    {
    public:
        DtlsHandshakeServerKeyExchange(uint16_t sequence, const std::vector<uint8_t>& key, const std::vector<uint8_t>& sig) :
            DtlsHandshake(sequence, DtlsHandshakeMessageType::ServerKeyExchange),
            m_serverKey(key),
            m_signature(sig)
        {
        }

        DtlsHandshakeServerKeyExchange(SocketPayload& packet);

        std::vector<uint8_t>& ServerKey(void) { return m_serverKey; }
        std::vector<uint8_t>& Signature(void) { return m_signature; }

        void Finalize(void) override;

    private:
        std::vector<uint8_t> m_serverKey;
        std::vector<uint8_t> m_signature;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsHandshakeServerHelloDone : public DtlsHandshake
    {
    public:
        DtlsHandshakeServerHelloDone(uint16_t sequence) :
            DtlsHandshake(sequence, DtlsHandshakeMessageType::ServerHelloDone)
        {
        }

        DtlsHandshakeServerHelloDone(SocketPayload& packet);

        void Finalize(void) override;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsHandshakeClientKeyExchange : public DtlsHandshake
    {
    public:
        DtlsHandshakeClientKeyExchange(uint16_t sequence, const std::vector<uint8_t>& key) :
            DtlsHandshake(sequence, DtlsHandshakeMessageType::ClientKeyExchange),
            m_clientKey(key)
        {
        }

        DtlsHandshakeClientKeyExchange(SocketPayload& packet);

        std::vector<uint8_t>& ClientKey(void) { return m_clientKey; }

        void Finalize(void) override;

    private:
        std::vector<uint8_t> m_clientKey;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    class DtlsHandshakeFinished : public DtlsHandshake
    {
    public:
        DtlsHandshakeFinished(uint16_t sequence, const std::vector<uint8_t>& verify) :
            DtlsHandshake(sequence, DtlsHandshakeMessageType::Finished),
            m_verifyData(verify)
        {
        }

        DtlsHandshakeFinished(SocketPayload& packet);

        std::vector<uint8_t>& VerifyData(void) { return m_verifyData; }

        void Finalize(void) override;

    private:
        std::vector<uint8_t> m_verifyData;
    };
}
