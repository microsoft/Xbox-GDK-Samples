#include "pch.h"

#include "DtlsConnection.h"
#include "DtlsSocket.h"

using namespace ATG;

// Global options
uint16_t g_portNumber = 0;
bool g_debugOutput = false;
bool g_serverRunning = false;
std::string g_certFile;
std::string g_keyFile;
std::string g_expectedIdentityString;

// Global components
XTaskQueueHandle g_taskQueue{};
std::unique_ptr<DtlsSocket> g_serverSocket;
DtlsConnectionHandle g_serverConnection{};

namespace ATG
{
    bool GetLocalNetworkAddress(SOCKADDR_IN& outAddr);
}

void Usage(void)
{
    std::cout << "Usage: DtlsEchoServer.exe [-p <port>] [-d] <certificate> <privatekey>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << " -p = Port to bind to (default: 0)" << std::endl;
    std::cout << " -d = Enable debug output" << std::endl;
    std::cout << " -? = This help text" << std::endl;
}

void ProcessCommandLine(int argc, char** argv)
{
    int currentArg = 0;

    try
    {
        // Loop over the arguments, skipping the first (the exe name)
        while (++currentArg != argc)
        {
            std::string argValue(argv[currentArg]);

            if (argValue == "-p")
            {
                if (++currentArg == argc)
                {
                    throw std::invalid_argument("p");
                }

                g_portNumber = static_cast<uint16_t>(std::atoi(argv[currentArg]));

                std::cout << "Using port number: " << g_portNumber << std::endl;
            }
            else if (argValue == "-d")
            {
                g_debugOutput = true;

                std::cout << "Debug logging enabled" << std::endl;
            }
            else
            {
                // If it's not an option assume it's the cert file name
                g_certFile = argv[currentArg];

                // The next argument needs to be the key file name
                if (currentArg + 1 == argc)
                {
                    continue;
                }

                g_keyFile = argv[++currentArg];

                std::cout << "Using cert file: " << g_certFile << std::endl;
                std::cout << "Using key file: " << g_keyFile << std::endl;
            }
        }

        if (g_certFile.empty())
        {
            std::cerr << "No certificate file specified." << std::endl;
            throw std::invalid_argument("cert");
        }

        if (g_keyFile.empty())
        {
            std::cerr << "No private key file specified." << std::endl;
            throw std::invalid_argument("key");
        }
    }
    catch (...)
    {
        // Any errors parsing the command line will just print usage and exit
        Usage();
        exit(1);
    }
}

void PrintLocalAddress()
{
    SOCKADDR localAddress{};
    SOCKADDR_IN* addrIn = reinterpret_cast<SOCKADDR_IN*>(&localAddress);

    ATG::GetLocalNetworkAddress(*addrIn);
    addrIn->sin_port = htons(g_portNumber);

    std::cout << "\nLocal Address: " << AddressToString(localAddress) << "\n\n";
}

void PrintLocalIdentityString()
{
    uint32_t fingersize = g_serverSocket->GetFingerprintSize();
    std::vector<uint8_t> fingerprint(fingersize);
    g_serverSocket->GetFingerprint(fingerprint.data(), fingersize, &fingersize);

    uint32_t namesize = g_serverSocket->GetSubjectNameSize();
    std::vector<uint8_t> subject(namesize);
    g_serverSocket->GetSubjectName(subject.data(), namesize, &namesize);

    std::cout << "Local Identity: " << ATG::BytesToHexString(fingerprint.data(), fingersize);
    std::cout << ":" << ATG::BytesToHexString(subject.data(), namesize) << "\n\n";
}

void RequestExpectedIdentityString()
{

    std::cout << "Enter the Identity string for the expected connection: ";
    std::cin >> g_expectedIdentityString;

    std::string expectedFingerprintString, subjectname;
    ATG::SplitString(g_expectedIdentityString, ":", expectedFingerprintString, subjectname);

    std::cout << "\nExpected Fingerprint: " << expectedFingerprintString << "\n\n";
    std::cout << "Expected SubjectName: " << subjectname << "\n\n";
}

int main(int argc, char** argv)
{
    WSAData wsa{};

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::cerr << "Failed to initialize WinSock." << std::endl;
        exit(1);
    }

    // Get any command line settings
    ProcessCommandLine(argc, argv);

    // Establish a task queue
    auto hr = XTaskQueueCreate(XTaskQueueDispatchMode::SerializedThreadPool, XTaskQueueDispatchMode::Manual, &g_taskQueue);

    if (FAILED(hr))
    {
        std::cerr << "Unable to create task queue." << std::endl;
        exit(1);
    }

    // Create a DTLS socket
    DtlsSocket* socket{};

    hr = DtlsSocket::Create(g_portNumber, g_taskQueue, g_certFile, g_keyFile, &socket);

    if (FAILED(hr))
    {
        std::cerr << "Unable to create DTLS socket." << std::endl;
        exit(1);
    }

    // Move the socket to our unique pointer
    g_serverSocket.reset(socket);

    // Print the local address for copy pasting
    PrintLocalAddress();

    // Print the local identity string for copy pasting
    PrintLocalIdentityString();

    // Allow the user to paste the expected identity string
    RequestExpectedIdentityString();

    // Establish handler for incoming connections
    g_serverSocket->AcceptConnections([](DtlsSocket* socket, const SOCKADDR* source, const SocketPayload* data, void*)
    {
        std::string addressString = AddressToString(*source);

        if (g_serverConnection != nullptr)
        {
            return;
        }

        std::cout << "Incoming connections from " << AddressToString(*source) << ". Establishing..." << std::endl;

        auto async = std::make_unique<XAsyncBlock>();
        async->queue = g_taskQueue;
        async->callback = [](XAsyncBlock* async)
        {
            std::unique_ptr<XAsyncBlock> asyncPtr{ async };

            auto hr = DtlsSocket::AcceptConnectionAsyncResult(async, &g_serverConnection);

            if (SUCCEEDED(hr))
            {
                std::cout << "Connection established." << std::endl;
            }
            else
            {
                std::cout << "Connection failed." << std::endl;
            }
        };

        // Attempt to connect securely
        auto hr = socket->AcceptConnectionAsync(source, g_expectedIdentityString, data, async.get());

        if (SUCCEEDED(hr))
        {
            async.release();
        }
    }, nullptr);

    // Setup a (Ctrl-C) handler to quit the server
    SetConsoleCtrlHandler(
        [](DWORD dwCtrlType) -> BOOL
        {
            if (dwCtrlType == CTRL_C_EVENT)
            {
                std::cout << std::endl << "Ctrl-C received, stopping server." << std::endl;
                g_serverRunning = false;

                return TRUE;
            }

            return FALSE;
        },
        true);

    std::cout << "Server accepting connections.  Ctrl-C to exit." << std::endl;

    g_serverRunning = true;

    // Main server loop
    while (g_serverRunning)
    {
        DtlsConnectionHandle source{};
        SocketPayload data{};

        XTaskQueueDispatch(g_taskQueue, XTaskQueuePort::Completion, 0);

        while (SUCCEEDED(g_serverSocket->RecvFrom(&source, &data)))
        {
            auto async = std::make_unique<XAsyncBlock>();
            async->queue = g_taskQueue;
            async->callback = [](XAsyncBlock* async)
            {
                std::unique_ptr<XAsyncBlock> asyncPtr{ async };
            };

            std::cout << "Received: " << std::string_view(reinterpret_cast<const char*>(data.payload.data()), data.Size()) << std::endl;

            // Echo back the received data
            hr = g_serverSocket->SendToAsync(source, &data, async.get());

            if (SUCCEEDED(hr))
            {
                async.release();
            }
        }
    }

    if (g_serverConnection != nullptr && g_serverConnection->IsConnected())
    {
        g_serverSocket->CloseConnection(g_serverConnection);
    }

    g_serverSocket.reset();

    WSACleanup();

    return 0;
}
