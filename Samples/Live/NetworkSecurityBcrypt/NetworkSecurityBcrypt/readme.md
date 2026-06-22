  ![](./media/image1.png)

#   Networking Security CNG (BCrypt) Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# **NOTICE: These samples are provided as exemplary references only. We do not guarantee or warrant that these will be sufficient to achieve your networking security goals, now or in the future. Make sure to assess your implementations independently.**

# Description

This sample demonstrates the best practices for helping secure UDP
traffic using a DTLS implementation using the CNG (Crypto: Next
Generation) API.

![Graphical user interface, text, website Description automatically generated](./media/image3.png)

# Building the sample

This sample supports Xbox One, Xbox Series Consoles, and GDK Desktop.
Select the config in the dropdown to build.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Running the sample

In order to run this sample, the included DTLSEchoServer project must
also be built and run. The server also requires an x509 certificate and
RSA private key.

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

This sample demonstrates how to use the Microsoft CNG (Crypto: Next
Generation) APIs to handle pre-shared certificates and private keys,
generating self-signed certificates, and implementing a secured
connection using DTLS with TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384.

The underlying UDP socket is using Overlapped I/O and the Windows Thread
Pool to minimize the delay between the data being received by the OS and
delivered to the title.

Finally, in production code you should be using a key service or lobby
service (such as MPSD) to help share the key material securely between
the game instances.

# Known issues

The sample only partially implements the DTLS 1.2 protocol and is
designed as a sample of the CNG API usage, and not a production ready
DTLS server. It is not complete, it is not robust and does not a
reliable UDP networking layer.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update history

**Initial Release:** October 2021

June 2022 -- Updated for March 2022 GDK (and newer) compatibility
