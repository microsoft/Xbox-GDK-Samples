  ![](./media/image1.png)

#   Networking Security OpenSSL Linux Server Sample

### **NOTICE: These samples are provided as exemplary references only. We do not guarantee or warrant that these will be sufficient to achieve your networking security goals, now or in the future. Make sure to assess your implementations independently.**

# Description

This sample demonstrates best practices for helping secure UDP traffic
using OpenSSL's DTLS implementation and is designed to work in
conjunction with the Networking Security OpenSSL sample.

# ![Text Description automatically generated](./media/image3.png)

# Building the sample

The development environment used to produce the sample was:

-   Windows Subsystem for Linux v2 on Windows 10

-   Ubuntu 20.04 LTS

-   Visual Studio Code

Apart from the base install, Ubuntu was further configured with:

`sudo apt install build-essential clang gcc gdb openssl-dev`

The sample includes the VS Code project configuration in the .vscode
directory which may need to be modified to suit local system
configurations.

The project may also be built with the following assuming the
configuration above:

`clang++ -std=c++17 -g ./\*.cpp -o ./NetworkSecurityOpenSSL_Linux -l ssl -l crypto -pthread`

# Running the sample

In order to run this sample, the Network Security OpenSSL sample must
also be built and run. The server also requires an x509 certificate and
the related private key.

Normally a production title would use a secure key service to exchange
certificate information for security validation, however for the
purposes of the sample these values will need to be copy/pasted between
the client and server.

Upon launching the client it will generate and display an Identity
string. This value is needed to launch the server; it will be prompted
for during startup. The server will also generate an Identity string.

When Open Connection is selected in the client, it will prompt for the
server address and Identity string. Once these values are entered the
client will follow the best practices for security while connecting to
the server and the client can send arbitrary messages to the server
which will echo them back.

# Implementation notes

The DTLS implementation in this sample is the same as the one in the
Network Security OpenSSL sample ported to Linux.

The differences from the client version are:

-   Winsock API was replaced with generic BSD sockets

-   Windows Thread Pool and Overlapped I/O replaced with std::thread and
    a select() loop

The sample also includes an implementation of XAsync and XTaskQueue
built on the STL for portability.

Finally, in production code you should be using a key service or lobby
service (such as MPSD) to help share the key material securely between
the game instances.

# Known issues

# Update history

**Initial Release:** August 2021
