  ![](./media/image1.png)

#   Networking Security OpenSSL Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# **NOTICE: These samples are provided as exemplary references only. We do not guarantee or warrant that these will be sufficient to achieve your networking security goals, now or in the future. Make sure to assess your implementations independently.**

# Description

This sample demonstrates the best practices for helping secure UDP
traffic using OpenSSL's DTLS implementation.

![Graphical user interface, text, website Description automatically generated](./media/image3.png)

# Building the sample

This sample supports Xbox One, Xbox Series Consoles, and GDK Desktop.
Select the config in the dropdown to build.

*For more information, see* __Running samples__, *in the GDK documentation.*

This sample does not ship with a pre-compiled version of OpenSSL. You
will need to acquire the latest version from one of the following
sources and add it to the project:

For PC you can download the latest binaries from:

-   <https://www.openssl.org/>

-   <https://github.com/openssl/openssl>

An alternative would be to use a package manager such as
[vcpkg](aka.ms/vcpkg) or nuget.

For Xbox Console you will need to acquire the source from github and
build a custom version with the following configuration:

```perl
perl Configure VC-WIN64A-masm no-ui-console no-dso no-stdio -D"WINAPI_FAMILY=WINAPI_FAMILY_GAMES" -D"_WIN32_WINNT=0x0A00" -DOPENSSL_SYS_WIN_CORE
```

# Running the sample

The sample can either be run as a client or a host and use either a
pre-shared key or a certificate. The sample is built to only demonstrate
one connection at a time between a client and host. To start as a host,
press the "Accept Connections" button. When using either a pre-shared
key or certificate, the sample will print out the key or fingerprint of
the certificate. After clicking "Accept" you will see a window pop up
asking for the IP address of the client you expect to connection. On
console this is required to allow packets from the client through the
console's firewall.

![Graphical user interface Description automatically generated](./media/image4.png)

To connect to the host, you will click the "Open Connection" button. The
first pop up window will be for the IP address and port of the host. The
second will be for the fingerprint or pre-shared key that the host is
using.

Once the connection has been completed, pressing "Send Message" will
allow you to input a string that will be send between the 2 sample
instances. If the traffic was viewed in a tool such as Wireshark it
would be encrypted and unreadable.

# Implementation notes

The sample uses OpenSSL's memory BIO and not a DGRAM BIO. This was done
to allow the socket to be reused for multiple connections (not
demonstrated in this sample). To utilize this data must be moved between
the socket and the BIO and is not done automatically as it would with
the DGRAM BIO.

The underlying UDP socket is using Overlapped I/O and the Windows Thread
Pool to minimize the delay between the data being received by the OS and
delivered to the title.

Finally, in production code you should be using a key service or lobby
service (such as MPSD) to help share the key material securely between
the game instances.

# Known issues

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update history

**Initial Release:** August 2021

June 2022 -- Updated for March 2022 GDK (and newer) compatibility
