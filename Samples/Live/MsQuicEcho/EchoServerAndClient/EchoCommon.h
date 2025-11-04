#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
//
// The conformant preprocessor along with the newest SDK throws this warning for
// a macro in C mode. As users might run into this exact bug, exclude this
// warning here. This is not an MsQuic bug but a Windows SDK bug.
//
#pragma warning(disable:5105)
#include <share.h>
#endif

#include <msquic.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

    // Shared variables (defined in EchoBase.c)

    extern const QUIC_BUFFER Alpn;
    extern const uint64_t IdleTimeoutMs;
    extern const QUIC_API_TABLE* MsQuic;
    extern HQUIC Registration;
    extern HQUIC Configuration;
    extern BOOLEAN MsQuicRunning;


    // Function declarations (implemented in EchoBase.c)

    void PrintUsage(void);

    BOOLEAN GetFlag(
        _In_ int argc,
        _In_reads_(argc) _Null_terminated_ char* argv[],
        _In_z_ const char* name
    );

    _Ret_maybenull_ _Null_terminated_ const char* GetValue(
        _In_ int argc,
        _In_reads_(argc) _Null_terminated_ char* argv[],
        _In_z_ const char* name
    );

    uint16_t StrToUInt16(
        _In_z_ const char* Str
    );

    uint8_t DecodeHexChar(
        _In_ char c
    );

    uint32_t DecodeHexBuffer(
        _In_z_ const char* HexBuffer,
        _In_ uint32_t OutBufferLen,
        _Out_writes_to_(OutBufferLen, return)
        uint8_t* OutBuffer
    );

    void EncodeHexBuffer(
        _In_reads_(BufferLen) uint8_t* Buffer,
        _In_ uint8_t BufferLen,
        _Out_writes_bytes_(2 * BufferLen) char* HexString
    );

    void WriteSslKeyLogFile(
        _In_z_ const char* FileName,
        _In_ QUIC_TLS_SECRETS* TlsSecrets
    );


    // Function declarations for EchoServerAndClient.c, EchoClient.c, and EchoServer.c

    int QUIC_MAIN_EXPORT echoBegin(
        int argc,
        char* argv[]
    );

    void RunClient(
        _In_ int argc,
        _In_reads_(argc) _Null_terminated_ char* argv[]
    );

    void RunServer(
        _In_ int argc,
        _In_reads_(argc) _Null_terminated_ char* argv[]
    );

#ifdef __cplusplus
}
#endif
