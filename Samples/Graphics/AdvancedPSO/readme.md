  ![](./media/image1.png)

# AdvancedPSO Sample

*This sample is compatible with the March 2022 GDK.*

# Description

This sample demonstrates Xbox specific Pipeline State Object (PSO)
generation methods which improve the memory usage and runtime
performance relative to the stock DirectX 12 method
(CreateGraphicsPipelineState). The sample is configured as two Visual
Studio solutions: PSOGen which runs on PC, and AdvancedPSO which runs on
Xbox.

The PSOGen solution demonstrates offline PSO generation. This runs on a
PC and generates a set of serialized PSO components for given shader
combinations. The source code demonstrates how to use the PSO
serialization API. (SerializeGraphicsPipelineStateX)

The AdvancedPSO solution runs on the console. The PSO set is loaded on
demand at runtime and deserialized (DeserializeGraphicsPipelineStateX).
The set is the minimal amount of data needed for all combinations, using
a process known as de-duplication.

The AdvancedPSO sample also demonstrates *deriving* PSOs. This is an
extremely performant and memory efficient way to make new PSOs that
differ by certain attributes defined by the API
(CreateDerivedGraphicsPipelineState).

![Sample Screenshot](./media/image3.png)

# Building the Sample

The AdvancedPSO project consumes the precompiled PSO microcode produced
by PSOGen. The shader compiler is subject to changes between GDK
versions, and therefore so is its output. For this reason the
AdvancedPSO project requires that the GDK versions between the
precompiled PSO's and its build environment match.

If the sample detects a versioning failure it will emit a runtime error.
If compiling against a different GDK simply build and run the PSOGen
tool to update the deserialized PSOs, then re-run the AdvancedPSO
sample.

When you build and run the Solution, Visual Studio will 

- Build the PSOGen project
- Run the PSOGen project on the PC to generate offline PSOs
- Build the AdvancedPSO project 
- Run the AdvancedPSO executable on the console to consume offline PSOs

# Using the sample

To use the sample simply run the AdvancedPSO sample. It will show the
metrics for each PSO creation method onscreen. To generate different
shader combinations or edit shader source, re-run the PSOGen executable,
then run the AdvancedPSO sample.

If you want to debug the PSOGen project on the PC, you need to point the 
Path environment variable to the GDK binaries, as shown below:

![PSOGen Settings](./media/PSOGen-settings.png)

# Implementation notes

The PSOGen project must link with a custom version of the D3D12.x\[s\]
UMD driver built for PC, d3d12_x\[s\].h, .lib, and .dll. This can be
found within the GDK install directory at `<GDK root\>\bin\XboxOne` or
`<GDK root\>\bin\Scarlett`, respectively. This library links with
several other custom-built libraries within that directory. These must
be on the path for **D3D12CreateDevice** to return successfully. The
PSOGen project is configured with a custom build step to clone these
libraries to the project directory. Note that there are different sets
of libraries for Xbox One & Xbox Series X\|S.

| Xbox One                          |  Scarlett (Xbox Series X\|S)        |
|-----------------------------------|-----------------------------------|
| d3d12_x.dll                       |  d3d12_xs.dll                      |
| xgs12_pc_x.dll                    |  xgs12_pc_xs.dll                   |
| xg.dll                            |  xg_xs.dll                         |
| dxcompiler_x.dll                  |  dxcompiler_xs.dll                 |
| sc_dll.dll                        |  xbsc_xs.dll                       |
| scdxil.dll                        |  newbe_xs.dll                      |

The precompiled shader blobs and serialized pipeline state packets are
not interchangeable between Xbox One and Xbox Series X|S. The PSOGen
outputs the compiled PSOs to a platform-specific directory set by
project configuration, where it's then deployed to the corresponding
target platform by the AdvancedPSO project.

The PSOSet class is used to generate and load a set of PSO components.
This implementation is not particularly optimal as it is foremost
demonstrative of the necessary steps. It stores the components as
individual files, where in fact they are typically archived into blob
storage. Also, there is no attempt to pre-load shader combinations, or
use multi-threading for serializing or de-serializing.

The sample uses XMemAlloc hooks (in the Minitracker class) to measure
the driver memory usage of the various creation methods. The driver will
pre-allocate extra internal storage when the first PSO is created. The
sample excludes these allocations from the statistics using ObjectId
attributes. The semantics of the ObjectId attributes aren't exposed to
public headers and are subject to change. They may be exported in a
future GDK release.

# Update history

12/14/2020 -- Ported from Xbox One XDK to GDK.
11/28/2023 -- Solution reconfigured to automatically generate PSOs
