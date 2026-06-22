  ![](./media/image1.png)

#   OfflineRT Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022).*

# 

# Description

This sample demonstrates how to use the offline acceleration-structure
(BVH) builder and raytracing pipeline state object (RTPSO) serialization
features available in the Microsoft Game Development Kit for the Xbox
Series X|S platform.

A title can integrate these features into existing content and shader
build pipelines to produce asset data in Xbox-native formats to prevent
costly shader compiler invocations at runtime and to increase the BVH
quality and reduce scratch memory requirements.

The sample also demonstrates how to build BVH hierarchy for a 3D mesh
externally (please refer to `bvhtoy.cpp` source code), how to convert such
hierarchy into a set of `D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE`
nodes representing a BLAS (please refer to `bvhtoyxbox.cpp`), and how to convert
such BLAS via `BuildRaytracingAccelerationStructureFromCanonicalBVH` API into
the format the graphics driver understands and can serialize.

# Building the Sample

The sample consists of two separate solutions: RTBuilder and OfflineRT.
RTBuilder is a PC application that contains the code necessary to build
offline BVHs and RTPSOs and OfflineRT is a Xbox Series X|S application
that can consume the offline-generated data and visualize it.

When you build and run the Solution, Visual Studio will 

- Build the RTBuilder project
- Run the RTBuilder project on the PC to generate offline RTPSOs
- Build the OfflineRT project 
- Run the OfflineRT executable on the console to consume offline RTPSOs

When you run RTBuilder, you will see console output like this:

![Text Description automatically generated](./media/image3.png)

When the tool finishes, you should see the following generated files in
the sample's Build directory:

![Graphical user interface, text, application Description automatically generated](./media/image4.png)

You can now proceed to compile and run the OfflineRT sample on your Xbox
Series X|S development kit.

# Using the sample

![](./media/image5.png)

The sample raytraces a single model using a simple RTPSO. You can switch
between red runtime (.sdkmesh file) and green offline (.mdat file)
generated model BVHs and cycle through runtime, offline collection-based
and fully offline RTPSOs using the Gamepad:

| Action                                       |  Gamepad               |
|----------------------------------------------|-----------------------|
| Switch model (BVH)                           |  Dpad left/ right      |
| Switch RTPSO                                 |  Dpad up/ down         |
| Orbit camera                                 |  Right thumbstick      |
| Reset camera  |  Right thumbstick button                |
| Zoom/ roll camera                            |  Left thumbstick       |
| Exit                                         |  View Button           |

The sample shows some basic stats for the selected BVH and RTPSO. Notice
that the offline-generated BVH is significantly smaller in memory and
also doesn't require any scratch space at runtime. Importantly, it also
traces faster. For offline RTPSOs, you can see that virtually the full
creation time cost can be shifted to the builder.

If you want to debug the RTBuilder project on the PC, you need to point the 
Path environment variable to the GDK binaries, as shown below:

![RTBuilder Settings](./media/RTBuilder-settings.png)

# Implementation notes

The RTBuilder application turns OBJ models and HLSL shader libraries
into Xbox-native BVHs and RTPSOs by leveraging functionality present in
the PC-version of the Xbox Series X|S graphics driver (UMD). The
driver's PC API is available in
`%GXDKLatest%\toolKit\include\Scarlett\d3d12_xs.h` and the binaries
needed at runtime can be found in `%GXDKLatest%\bin\Scarlett`. The
sample deploys the binaries to the output directory using a custom build
step.

While offline BVHs are produced with the same
`ID3D12GraphicsCommandList6::BuildRaytracingAccelerationStructure` API as
you would use to produce runtime BVHs, it is important to notice that
these operations happen on the CPU (immediately when you call them) and
not on the GPU at ExecuteCommandLists time. Along the same lines, all
`D3D12_GPU_VIRTUAL_ADDRESS` parameters passed as inputs and outputs are
interpreted as regular CPU virtual memory addresses. Internally, the PC
UMD will utilize the [Intel Embree](https://www.embree.org/) builder
instead of the GPU compute-based solution used in the driver at runtime
to produce the BVH. The Embree builder produces a higher quality BVH
which typically leads to a 5% - 10% (potentially even higher depending
on model data) speedup in traversal time.

Although the performance delta between runtime and offline built BVHs is
small on this sample's content, typical game content will show larger
performance wins. The Intel Embree offline builder employs \'triangle
splitting\' techniques to reduce the effective surface area of long,
thin triangles, while the runtime builder does not. The offline builder
also produces smaller (memory-wise) BVH structures because it can
produce quad leaves to a higher degree than the runtime builder.
Finally, no scratch memory is needed for the build at runtime which can
be particularly beneficial for Xbox Series S consoles.

After BVH construction, the structure is serialized using the
`CopyRaytracingAccelerationStructure` API with copy mode
`D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE` and written
out to disk.

In the OfflineRT sample (see AddModelFromMDat function), the
acceleration structure is deserialized again on the GPU using the
`CopyRaytracingAccelerationStructure` API with copy mode
`D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE`. We advise
performing deserialization operations on an async compute pipe in
parallel with other graphics work.

Offline RTPSOs are generated in three steps in RTBuilder:

-   HLSL compilation to DXIL libraries (`lib_6_6` target) using the Xbox
    Series X|S shader compiler (DXC)

-   Creation of RTPSO using Scarlett PC UMD

-   Serialization of the RTPSO using Scarlett PC UMD

The initial compilation is handled by the *HLSL Compiler* custom tool in
Visual studio. If you right-click on any of the .hlsl files in the
project's *Assets* folder, you can see the parameters used for this
process. The intermediate DXIL libraries are saved in the sample's
`Build\Int` directory. The second and third steps are done when running
RTBuilder. The sample has support for both collection and fully-linked
RTPSO creation and serialization. In both cases, the PC UMD will fully
compile Xbox-native shaders when `CreateStateObject` is called provided
ShaderConfig, PipelineConfig and root signature(s) are known (see the
[Collection state
object](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#collection-state-object)
section in the DXR Specification). Important also to notice, is that the
serialization process (initiated with the SerializeStateObjectX call)
will strip all DXIL and other metadata from the state objects, which
makes any runtime sub-object association impossible. It is, however,
still possible to look up Shader Identifiers for Shader Binding Tables
since shaders aren't sub-objects.

The OfflineRT sample demonstrates 3 different ways of constructing
RTPSOs:

- Runtime creation from DXIL library (see
  `AddPipelineFromEmbeddedDXILLib` function): This method has maximum
  flexibility since it allows full sub-object association behavior but
  incurs the full compilation and linking cost at runtime.
- Runtime creation based on offline collections (see
  `AddPipelineFromSerializedCollections`): State object creation is fast
  because all shaders are fully precompiled and internally linked. The
  method maintains a degree of flexibility by allowing RTPSOs to be
  "composed" from multiple (potentially shared) collections.
- Runtime deserialization (see `AddPipelineFromSerializedRTPSO`
    function): State object creation is fast because everything is
    precompiled and linked. No real flexibility.

# Known issues

Related Scarlett graphics driver bugs:

- Bug 33668549: Graphics: XDXR does not correctly AddRef collections
    referenced from RTPSOs via
    `D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION`

# Update history

- June 2021: Initial release.
- November 2023: Reconfigured to automatically generate RTPSOs.
- March 2024: Adds an example of building BVH externally and converting it into
  the format used by the driver via `BuildRaytracingAccelerationStructureFromCanonicalBVH` API.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
