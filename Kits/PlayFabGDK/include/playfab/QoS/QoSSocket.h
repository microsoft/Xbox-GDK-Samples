#pragma once

#if defined (PLAYFAB_PLATFORM_WINDOWS) || defined (PLAYFAB_PLATFORM_XBOX)
#include <playfab/QoS/XPlatSocket.h>
#include <playfab/QoS/PingResult.h>

namespace PlayFab
{
    namespace QoS
    {
        constexpr int BUFLEN = 512; // Length of the message send and received

        constexpr unsigned int MSGHEADER = 255; // Header of the message send over UDP
                                       // The message must start with 2 bytes of all bits set to 1

        constexpr char BUFFER_VALUE = '1'; // Clear the buffer with this value before receiving the reply.

        constexpr int PORT = 3075; // Port the QoS server listen on

        /// <summary>
        /// A wrapper on top of the XPlatSocket class to abstract out the logic
        /// for pinging the Azure QoS servers.
        /// </summary>
        class QoSSocket
        {
        public:
            QoSSocket();

            QoSSocket(const QoSSocket&) = delete;
            QoSSocket(QoSSocket&&) = delete;
            QoSSocket& operator=(const QoSSocket&) = delete;
            QoSSocket& operator=(QoSSocket&&) = delete;

            // Configure the socket with the following :
            //	1. Timeout for the call
            //	2. Set the port to 3075
            //	3. Set the mode to blocking vs non-blocking/async (using the constexpr BLOCKING_ASYNC_MODE above)
            int ConfigureSocket(int timeoutMs);

            // Set the address that the socket would ping
            int SetAddress(const char* socketAddr);

            // Get the ping relay latency for the server set.
            PingResult GetQoSServerLatencyMs();

        private:
            std::unique_ptr<XPlatSocket> xPlatSocket;

            // Message sent. Should start with 2 bytes set to 255 each (all bits set to 1)
            const char message[BUFLEN]{ static_cast<char>(MSGHEADER), static_cast<char>(MSGHEADER) };
            char buf[BUFLEN];
            const std::chrono::milliseconds threadWaitTimespan = std::chrono::milliseconds(THREAD_WAIT_MS);
        };
    }
}
#endif // defined (PLAYFAB_PLATFORM_WINDOWS) || defined (PLAYFAB_PLATFORM_XBOX)