  ![](./media/image1.png)

#   Meshlet Converter App

*\* This tool is compatible with PC.*

# Description

The meshlet converter is a command line tool for use on PC. The Visual
Studio solution has three projects:

-   ConverterApp -- a executable command line tool that generates
    meshlet data using DirectXMesh

-   Runtime -- a static library project which contains a runtime version
    of the meshlet data structures

The ConverterApp project is a command line tool which can be used to
generate meshlet data from an FBX file, OBJ file, or SDKMesh file. The
tool leverages DirectXMesh integration of meshlet generation to generate
meshlets from vertex & primitive data read from the input FBX file.

The Runtime project provides self-contained runtime meshlet code which
demonstrates how meshlets would be deserialized and uploaded at runtime.
It is not a standalone demonstration but could be easily integrated into
an existing codebase.

# Setup

To compile the tool requires that the FBX SDK 2019.2 is installed. Once
installed configure an environment variable named 'FBX_SDK' to point at
the installation directory (commonly *C:\\Program
Files\\Autodesk\\FBX\\FBX SDK\\2019.2*).

# Using the sample

The command line tool only has a few options:

-   -h -- Display the help message

-   -v \<int\> - Specifies the max vertex count of a meshlet. Must be
    between 32 and 256, inclusively. Default is 128

-   -p \<int\> - Specifies the max primitive count of a meshlet. Must be
    between 32 and 256, inclusively. Default is 128

-   -s \<float\> - Specifies a global scaling factor for scene geometry.
    Default is 1.0

-   -fz -- Flips the Z axis of scene geometry. Default is false

-   -ft -- Flips the triangle winding order of the scene geometry.
    Default is false

-   -i -- Forces vertex indices to 32-bits, even if 16-bits would
    suffice. Default is false

-   -t - Triangulates scene meshes file using the FbxGeometryConverter
    functionality. Default is false

-   \<file list\> - List of relative file paths to process. Must provide
    at least one.

An example usage may be:

ConverterApp.exe -v 256 -p 256 -f Path/To/MyFile1.fbx
Path/To/MyFile2.fbx

# Implementation notes

The command line tool does not modify or export mesh vertex data.
Automatic FBX SDK triangulation can be specified on the command line.

Since FBX files may contain multiple meshes the exported files may pack
multiple sets of meshlets. There currently is no scheme to index
different meshlets by mesh name but may be added in a later iteration.
The meshes are processed and exported according to in-order,
breadth-first traversal of the FBX node tree.

# Usage Note

Care must be taken to ensure there is no reordering of index or vertex
data during conversion of the mesh to the engine runtime format. Since
vertex data is not exported with the command line tool any reordering
will invalidate the meshlet data.

# Update history

12/2/2019 -- Sample creation.

2/20/2019 -- Rewrote meshlet generator to support different
vertex/primitive counts, more coherent spatial and orientation
properties.

4/11/2020 -- Replaced meshlet generation interface with a thinner
DirectXMesh-like interface.

10/17/2022 -- Added support for reading from an SDKMesh file.
