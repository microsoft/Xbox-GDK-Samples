  ![](./media/image1.png)

#   SimpleHttp Sample

*This sample is compatible with the Microsoft GDK (Desktop) and GDKX
(Xbox) (March 2022)*

# Description

This sample demonstrates using XCurl to make HTTP requests including
adding the user token and signature to the headers for authenticated
Xbox Live calls.

# Building the sample

The sample should not require any specific changes to build and should
run without any modifications if using the XDKS.1 sandbox.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

When the sample is run, you can send requests over HTTPS to services
that require authentication with XSTS tokens.

For HTTPS calls, you can select to call a standard Xbox Live endpoint to
get information about the current user (Profile Service) with the XBL
Service HTTP Request button. This connects to the service and properly
adds the XSTS token as the Authorization header and adds the Signature
header.

To simulate a call to a custom game service you can use the Game Service
HTTP Request which also appends needed XSTS token auth for a Game
Service. This by default calls to the running sample version of the Game
Service Sample and will reply back with all of the claims within the
user's X-Token used to auth with the service. Other service
functionality including b2b commerce URIs can also be used with this
sample by overriding the button's target URL with the other options
commented out in the code. For more information about configuring your
own custom Game Service see the Game Service sample and configuration
guide.

## Main Screen

![Graphical user interface, text, website Description automatically generated](./media/image3.png)

# Implementation notes

The XCurl usage is all found in HttpManager.h/.cpp. Here you'll find
demonstrations of:

-   Waiting for networking availability

-   Creating an HTTPS "GET" request from the web server

-   Making general HTTP queries

Please refer to XCurl documentation for detailed API notes and usage.

# Update history

Initial release April, 2021

June 2022 -- March 2022 GDK (and newer) compatibility

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
