  ![](./media/image1.png)

#   SimpleSFS Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

Sampler Feedback is a Direct3D feature for capturing and recording
texture sampling information and locations during rendering. When
combining sampler feedback with tiled textures, we get an easy mechanism
to manage tile residency when implementing partially resident textures
(PRT) for texture streaming. This sample shows a simple implementation
of Sampler Feedback for Streaming (SFS), showing how to use a
SamplerFeedback map with a MinMip map. DirectStorage is used to load and
hardware decompress tiles directly into graphics memory.

The sample renders a tiled texture on a quad with a camera that can move
over the quad like a very simple landscape. As the camera moves around,
the UI will indicate which tiles were requested to be resident in
memory, and which tiles are currently resident in memory.

Note: Sampler feedback is not supported on Xbox One, therefore this is a
Xbox Series X|S only sample.

![](./media/image3.png)

# Building the sample

This sample only supports Xbox Series X|S, so the active solution
platform will be Gaming.Xbox.Scarlett.x64

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Move camera                            |  Left Thumb Stick            |
| Exit                                   |  View Button                 |

# Implementation notes

The following diagram shows the data flow in an SFS implementation. A
*MinMip map* is a map that holds information about the minimum mip
level, i.e. the mip with the highest detail, that is currently resident.
As the streaming system loads or unloads tiles, it updates the MinMip
map on the CPU. A *feedback map* is a map that holds information about
the minimum mip level that was requested to be resident during scene
rendering. The feedback map is updated on the GPU during scene rendering
and the streaming system can generate new streaming requests based on
this information. For more detailed information, refer to the GDK
documentation and the Xfest 2019 presentation "Texture Streaming on
Scarlett".

![](./media/image4.png)

**Tiled Texture**

The tiled texture used in this sample was created the using xbtc.exe
tool included with the GDK. The tool takes as input a DDS file and
generates a compressed file. When using the option "-tilemode SFS", the
compressed file will contain the 64KB tiles for a tiled texture. The
method `TiledTexture::Create()` in TiledTexture.cpp shows how to consume a
.xbtc file. After loading this file, a list of tiles is created, with
each tile holding information about where on disk the data should be
loaded and once loaded, where in the memory heap it resides. PIX can
visualize tiled textures, showing which tiles are committed and which
are reserved.

**Feedback Map**

Two formats for feedback maps exist, MinMip and RegionUsed. This sample
implements a MinMip feedback, i.e. it's created using
`DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE`. To create a UAV for the
feedback map, use the API `CreateSamplerFeedbackUnorderedAccessView()`.
Before scene rendering, the feedback map must be cleared. It cannot be
cleared to the value zero, since that would mean that mip level 0 has
been requested during scene rendering. Therefore, the sample clears the
map to the value of -1, which indicates that no mip was requested. To
read the values on the CPU, the texture first needs to be transcoded
using `ResolveSubresourceRegion()` with the flag
`D3D12_RESOLVE_MODE_DECODE_SAMPLER_FEEDBACK`. For Xbox Series X|S, values
in the feedback map are 5.3 fixed point. The sample implements three
functions to convert values between feedback values and mip values,
`MipValueToFeedbackValue()`, `FeedbackValueToMipValue()` and
`DecodeFeedbackMapValue()`. Streaming requests are sorted based on the
fractional portion of the feedback values. PIX can visualize feedback
maps.

**MinMip Map**

The MinMip map is created as a placed texture with format R8. To create
a UAV, use the API `CreateMinMipShaderResourceViewX()`. The values in the
MinMip map are expected to be encoded as 5.3 fixed point, i.e. if the
streaming system loads in mip 2, the map should contain the value of (2
\<\< 3). Pix can visualize MinMip maps

**Streaming**

This is a simple sample, so streaming is implemented to happen
synchronously and with simple streaming logic. After scene rendering,
feedback map values are used to generate streaming requests in the
function GenerateStreamingRequests(). The function
ProcessStreamingRequests() processes the streaming requests, using
DirectStorage to load and decompress the requested tile directly into
graphics memory.

# Update history

04/27/2020 -- Sample creation.

10/15/2021 -- Added 1440p support.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
