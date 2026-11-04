<!-----
---
page_type: sample
languages:
- cpp
products:
- gdk
urlFragment: "hlslcompile"
extendedZipContent:
- path: LICENSE
  target: LICENSE
- path: Kits
  target: Kits
- path: Media
  target: Media
description: "The sample compiles the same pixel shader in a number of different ways, to illustrate different options for PC-side asset build. It also shows how to precompile pipeline state offline with the ACO shader compiler for Xbox Series X|S."
---
----->

  ![](./media/image1.png)

#   HlslCompile Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022). The ACO shader compiler example requires the Microsoft Game Development Kit (October 2026) or later.*

# Description

The sample compiles the same pixel shader in a number of different ways,
to illustrate different options for PC-side asset build.

The sample builds shaders using two different compiler interfaces:

-   `Dxc.exe` -- command line interface for the new shader compiler front
    end

-   `DxCompiler_x[s].dll` callable interface for the new shader compiler
    front end

There are separate copies of both the exe and the dll for Xbox One and
for Xbox Series X|S. (The Xbox Series X|S copy of the dll is
`DxCompiler_xs.dll`.)

Shader symbols provide important information for PIX, in the same way
that C++ symbols provide context for Visual Studio and other tools. The
shader compiler interfaces support several options for storage of
symbols:

-   Embedded in the binary -- this method is the simplest, but it is
    generally too costly in terms of runtime memory usage. Embedded
    symbols are deprecated for the new shader compiler front end.

-   Stripped to a manually chosen file name -- for example, the name may
    be chosen by the caller to be a recognizable variant of the source
    file name.

-   Stripped to an automatically chosen file name -- the name will be
    chosen based on a hash of the compiled shader. This method is
    recommended, since PIX can compute the same shader hash without a
    hint.

## ACO shader compiler (Xbox Series X|S only)

On Xbox Series X|S the sample also renders one triangle (`AcoManualPdb`)
with shaders compiled by the ACO back end (`aco_xbox.dll`). ACO requires
the Microsoft Game Development Kit (October 2026) or later and isn't
supported on Xbox One. With an older GDK, or on Xbox One, the sample
builds and runs without this row. The check is `ACO_EXAMPLE_ENABLED` in
`HlslCompile.h` and `AcoExampleEnabled` in `HlslCompile.vcxproj`.

Graphics shaders compiled by ACO are consumed on the console through a
pipeline state that is serialized offline on the PC and deserialized at
runtime. The whole flow is kept separate from the other examples so it
can be read on its own:

| Step | File | What it does |
|------|------|--------------|
| Build tool | `MyAcoCompiler\MyAcoCompiler.cpp` | `compile` mode: compiles HLSL with `DxCompiler_xs.dll` using `-requireacoprecompile`, strips symbols to a PDB. `pso` mode: calls `AcoOfflineSerializePipelineStateXEx` to serialize a graphics pipeline state into `*.bin` files. |
| Build wiring | `HlslCompile_Aco.targets` | Imported by `HlslCompile.vcxproj` when `AcoExampleEnabled` is true. Runs `MyAcoCompiler.exe compile <profile> <hlsl> <cso> <pdb>` per shader, then `MyAcoCompiler.exe pso <vs.cso> <ps.cso> <outputBase>`. |
| Runtime | `AcoPipelineState.h/.cpp` | Loads the `*.bin` files, places the shader code in `XMemAlloc` graphics-shader memory, and calls `DeserializeGraphicsPipelineStateX`. |

Notes:

-   The PC-side `d3d12_xs.dll` can't deserialize Xbox root signatures,
    so `MyAcoCompiler` builds the root signature description in code. It
    must match the `[RootSignature]` declared in the shaders.

-   The serialized pipeline state is tied to the GDK version that
    produced it. Rebuild the assets whenever the GDK changes.

-   ACO symbols are written to the PDB passed to the `compile` step. PIX
    shows the ACO disassembly for the shader, but can't currently map
    it back to HLSL source lines.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using an Xbox Series X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

When you build and run the Solution, Visual Studio will 

- Build the MyHlslCompiler project (and MyAcoCompiler for Xbox Series X|S)
- Build the HlslCompile project 
  - As part of that build, compile certain shaders using MyHlslCompiler.exe
  - For Xbox Series X|S, compile the ACO shaders and serialize the ACO
    pipeline state using MyAcoCompiler.exe
- Run the HlslCompile executable on the console using the compilation results

# Using the sample

The sample is non-interactive. Each row in the screen image below
contains a triangle. Each triangle is rendered using a copy of the same
pixel shader, with each copy compiled in a different manner. The size of
each pixel shader binary is listed in cyan (the numbers in the
screenshot may be out of date). The remainder of each line's text
describes how the shader was compiled, and how symbols are stored.

To verify that symbols were generated correctly, we recommend that you
take a PIX GPU capture of the sample and attempt to retrieve symbols
within PIX for the pixel shader of each triangle. In some cases, PIX
will automatically retrieve the proper symbols, while in other cases,
manual action on the user's part is required.

![](./media/image3.png)

If you want to debug the MyHlslCompiler project on the PC, you need to point the 
Path environment variable to the GDK binaries, as shown below:

![MyHlslCompiler Settings](./media/MyHlslCompiler-settings.png)

# Known issues

None.

# Update history

Initial release April 2019

Updated for Microsoft GDK November 2019

October 2026: Added the ACO shader compiler example for Xbox Series X|S

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
