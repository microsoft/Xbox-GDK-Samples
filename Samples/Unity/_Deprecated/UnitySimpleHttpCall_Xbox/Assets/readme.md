  ![](./media/image1.png)

#   Unity Simple Http Call for Xbox

*This sample is compatible with:*

-   *Microsoft Game Development Kit with Xbox Extensions (June 2022 Update 4 220604) & Later*

-   *Unity Editor 2021.3.20f1 & Later*

-   *Unity GameCore v0.7.0 & Later*

-   *Unity GXDKInput v0.1.5 & Later*

# 

# Description

The Unity Simple Http Call for Xbox sample demonstrates the usage of the
Xbox Live Http Calls using the Unity game engine. You can send & receive
a message as a string or byte array to a pre-configured Xbox Http end
point. Upon successful sending, the end point will provide an additional
response.

![](./media/image3.png)

# Noteable Code Files

**XboxManager.cs**: Contains initialization of the Xbox GDK & Xbox Live Services APIs,
along with signing in a user & querying various information, such as Sandbox
& Title ID.

**XboxHttpCall.cs**: Contains APIs for sending an http request through 
Xbox Live Services.

# Building the Sample

**IMPORTANT:** The sample **requires** the GameCore & GXDKInput packages
provided by Unity, which can be found in the [Unity
Forums](https://forum.unity.com/). However, these packages are currently
under **NDA** status. You will need to join
[ID@Xbox](https://www.xbox.com/en-us/Developers/id) to gain access.
These **must** be added, via Unity's **Package Manager**:

com.unity.gamecore-0.7.0

com.unity.inputsystem.gxdk-0.1.5-preview

![Graphical user interface, text, application Description automatically generated](./media/image4.png)

You will also need to switch to the target Xbox platform in Unity's
**Build Settings** page. Afterwards, you can build & deploy through Xbox
Manager to your target console.

Game Core -- Xbox One

Game Core -- Xbox Series

![Graphical user interface Description automatically generated](./media/image5.png)

*For more information, see* __Running samples__, *in the GDK documentation.*

# Running the Sample

You will need an Xbox Live test account signed in to execute updates and
retrieval commands.

The console's sandbox **must** be set to XDKS.1.

There are two available Http Call commands in this sample:

-   *Send Request as Bytes* -- sends a http GET Request with a string
    body converted into bytes to an Xbox Live end point. A subsequent
    response is distributed from Xbox Live.

-   *Send Request as String* -- sends a http GET request with a string
    body to an Xbox Live end point. A subsequent response is distributed
    from Xbox Live

**IMPORTANT:** The Http Call provides an additional pre-configured
response handler to a successful Http Call. This is demonstrated inside
of the *SDK.XBLXblHttpCallPerformAsync* callback in *XboxHttpCall.cs*.

*Additional Commands:*

-   *'Sign In'* -- opens the Xbox user selection.

-   '*Clear Logs'* -- Clears the console of all existing logs.

-   '*Close' --* Closes the sample.

# Known Issues

The sample was developed & tested against the packages & versions in
this document. It is highly recommended to use the most recent releases
for the GameCore plugin & GXDK Input package released by Unity.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History
5/18/2023 - Changed editor version to one that is supported by 220604 or later.
